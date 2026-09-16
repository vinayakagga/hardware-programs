/*
 * ESP32 RFID Attendance System with Admin Login
 * Complete implementation with paired entry/exit logs
 */

#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <time.h>

// ==============================
//  CONFIG
// ==============================

const char* ssid = "TP-Link_A289";
const char* password = "31768160";

// Admin credentials
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "admin123";

#define RST_PIN 3
#define SS_PIN  5
#define LED_PIN 2
#define BUZZER_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
AsyncWebServer server(80);

const char* CARDS_FILE  = "/cards.txt";
const char* LOGS_FILE   = "/logs.csv";
const char* STATUS_FILE = "/status.txt";

String lastUnknownUID = "";
String lastScannedUID = "";
unsigned long lastScanTime = 0;
const unsigned long SCAN_COOLDOWN = 5000;

// Session management
String sessionToken = "";
unsigned long sessionExpiry = 0;
const unsigned long SESSION_DURATION = 3600000; // 1 hour

// ==============================
//  TIME SYSTEM (non-blocking)
// ==============================

unsigned long lastTimeUpdate = 0;
String currentTimeString = "Loading...";
bool timeInitialized = false;

void updateTimeNonBlocking() {
  if (millis() - lastTimeUpdate < 1000) return;
  lastTimeUpdate = millis();
  
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  
  if (timeinfo.tm_year > 100) {
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    currentTimeString = String(buffer);
    timeInitialized = true;
  }
}

void setupNTP() {
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP configured");
}

// ==============================
//  SESSION MANAGEMENT
// ==============================

String generateToken() {
  String token = "";
  for (int i = 0; i < 32; i++) {
    token += String(random(0, 16), HEX);
  }
  return token;
}

bool isAuthenticated(AsyncWebServerRequest *request) {
  if (request->hasHeader("Cookie")) {
    String cookie = request->header("Cookie");
    int idx = cookie.indexOf("session=");
    if (idx != -1) {
      String token = cookie.substring(idx + 8, idx + 40);
      if (token == sessionToken && millis() < sessionExpiry) {
        sessionExpiry = millis() + SESSION_DURATION; // Extend session
        return true;
      }
    }
  }
  return false;
}

// ==============================
//  SPIFFS HELPERS
// ==============================

String readFileFast(const char* path) {
  File file = SPIFFS.open(path, "r");
  if (!file) return "";
  String content = file.readString();
  file.close();
  return content;
}

void writeFileFast(const char* path, String content) {
  File file = SPIFFS.open(path, "w");
  if (file) {
    file.print(content);
    file.close();
  }
}

void appendFileFast(const char* path, String content) {
  File file = SPIFFS.open(path, "a");
  if (file) {
    file.print(content);
    file.close();
  }
}

bool deleteCardByUID(String uid) {
  String cards = readFileFast(CARDS_FILE);
  String newCards = "";
  int start = 0;
  bool found = false;
  
  while (start < cards.length()) {
    int end = cards.indexOf('\n', start);
    if (end == -1) end = cards.length();
    
    String line = cards.substring(start, end);
    if (line.startsWith(uid + "|")) {
      found = true;
    } else if (line.length() > 0) {
      newCards += line + "\n";
    }
    start = end + 1;
  }
  
  if (found) {
    writeFileFast(CARDS_FILE, newCards);
    
    String status = readFileFast(STATUS_FILE);
    String newStatus = "";
    start = 0;
    while (start < status.length()) {
      int end = status.indexOf('\n', start);
      if (end == -1) end = status.length();
      String line = status.substring(start, end);
      if (!line.startsWith(uid + "|") && line.length() > 0) {
        newStatus += line + "\n";
      }
      start = end + 1;
    }
    writeFileFast(STATUS_FILE, newStatus);
  }
  
  return found;
}

String getCardInfo(String uid) {
  String cards = readFileFast(CARDS_FILE);
  int idx = cards.indexOf(uid + "|");
  if (idx == -1) return "";
  
  int end = cards.indexOf('\n', idx);
  if (end == -1) end = cards.length();
  
  return cards.substring(idx, end);
}

String getUserStatus(String uid) {
  String status = readFileFast(STATUS_FILE);
  int idx = status.indexOf(uid + "|");
  if (idx == -1) return "OUT";
  
  int start = idx + uid.length() + 1;
  int end = status.indexOf('\n', start);
  if (end == -1) end = status.length();
  
  return status.substring(start, end);
}

void setUserStatus(String uid, String newStatus) {
  String status = readFileFast(STATUS_FILE);
  String newStatusFile = "";
  bool found = false;
  int start = 0;
  
  while (start < status.length()) {
    int end = status.indexOf('\n', start);
    if (end == -1) end = status.length();
    
    String line = status.substring(start, end);
    if (line.startsWith(uid + "|")) {
      newStatusFile += uid + "|" + newStatus + "\n";
      found = true;
    } else if (line.length() > 0) {
      newStatusFile += line + "\n";
    }
    start = end + 1;
  }
  
  if (!found) {
    newStatusFile += uid + "|" + newStatus + "\n";
  }
  
  writeFileFast(STATUS_FILE, newStatusFile);
}

// ==============================
//  LOG PAIRING SYSTEM
// ==============================

void updatePairedLog(String uid, String name, String timestamp, String type) {
  String logs = readFileFast(LOGS_FILE);
  String newLogs = "";
  bool updated = false;
  int start = 0;
  
  while (start < logs.length()) {
    int end = logs.indexOf('\n', start);
    if (end == -1) end = logs.length();
    
    String line = logs.substring(start, end);
    
    if (line.length() > 0) {
      // Parse: UID|Name|EntryTime|ExitTime|Duration
      int p1 = line.indexOf('|');
      int p2 = line.indexOf('|', p1 + 1);
      int p3 = line.indexOf('|', p2 + 1);
      int p4 = line.indexOf('|', p3 + 1);
      
      if (p1 != -1) {
        String lineUID = line.substring(0, p1);
        String exitTime = (p3 != -1) ? line.substring(p3 + 1, p4 != -1 ? p4 : line.length()) : "";
        
        // If this UID has an open entry (no exit), update it
        if (lineUID == uid && exitTime == "" && type == "EXIT" && !updated) {
          String lineName = line.substring(p1 + 1, p2);
          String entryTime = line.substring(p2 + 1, p3);
          
          // Calculate duration
          long duration = 0; // In minutes (simplified)
          newLogs += uid + "|" + lineName + "|" + entryTime + "|" + timestamp + "|Calculated\n";
          updated = true;
          start = end + 1;
          continue;
        }
      }
    }
    
    if (line.length() > 0) {
      newLogs += line + "\n";
    }
    start = end + 1;
  }
  
  // If EXIT but no open entry found, or if ENTRY, add new line
  if (!updated) {
    if (type == "ENTRY") {
      newLogs += uid + "|" + name + "|" + timestamp + "||Ongoing\n";
    } else {
      newLogs += uid + "|" + name + "||" + timestamp + "|Exit Only\n";
    }
  }
  
  writeFileFast(LOGS_FILE, newLogs);
}

// ==============================
//  RFID HANDLING
// ==============================

void beep(int times, int duration = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    if (i < times - 1) delay(100);
  }
}

void handleRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  mfrc522.PICC_HaltA();
  
  if (uid == lastScannedUID && (millis() - lastScanTime) < SCAN_COOLDOWN) {
    beep(2, 50);
    return;
  }
  
  lastScannedUID = uid;
  lastScanTime = millis();
  
  String cardInfo = getCardInfo(uid);
  
  if (cardInfo == "") {
    lastUnknownUID = uid;
    beep(3, 150);
    Serial.println("Unknown UID: " + uid);
    return;
  }
  
  int p1 = cardInfo.indexOf('|');
  int p2 = cardInfo.indexOf('|', p1 + 1);
  String name = cardInfo.substring(p1 + 1, p2);
  
  String currentStatus = getUserStatus(uid);
  String newStatus = (currentStatus == "OUT") ? "IN" : "OUT";
  String logType = (newStatus == "IN") ? "ENTRY" : "EXIT";
  
  setUserStatus(uid, newStatus);
  updatePairedLog(uid, name, currentTimeString, logType);
  
  beep(1, 200);
  Serial.println(name + " - " + logType);
}

// ==============================
//  WEB SERVER
// ==============================

const char LOGIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Admin Login</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}
.login-card{background:#fff;border-radius:12px;padding:40px;box-shadow:0 10px 30px rgba(0,0,0,0.3);max-width:400px;width:90%}
h1{color:#333;margin-bottom:30px;text-align:center;font-size:28px}
.lock-icon{text-align:center;font-size:60px;margin-bottom:20px}
input{width:100%;padding:14px;margin-bottom:20px;border:2px solid #ddd;border-radius:6px;font-size:16px}
input:focus{border-color:#667eea;outline:none}
button{width:100%;padding:14px;background:#667eea;color:#fff;border:none;border-radius:6px;font-size:18px;cursor:pointer;transition:.3s}
button:hover{background:#5568d3}
.error{color:#dc3545;margin-bottom:15px;text-align:center;font-weight:600}
</style>
</head>
<body>
<div class="login-card">
<div class="lock-icon">🔐</div>
<h1>Admin Login</h1>
<div id="error" class="error"></div>
<form id="loginForm">
<input type="text" name="username" placeholder="Username" required autofocus>
<input type="password" name="password" placeholder="Password" required>
<button type="submit">Login</button>
</form>
</div>
<script>
document.getElementById('loginForm').onsubmit = async (e) => {
  e.preventDefault();
  const formData = new FormData(e.target);
  const params = new URLSearchParams(formData);
  
  const res = await fetch('/login?' + params);
  const result = await res.text();
  
  if (result === 'OK') {
    window.location.href = '/';
  } else {
    document.getElementById('error').textContent = 'Invalid credentials!';
  }
};
</script>
</body>
</html>
)rawliteral";

const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>RFID Attendance System</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px}
.container{max-width:1200px;margin:0 auto}
.card{background:#fff;border-radius:12px;padding:25px;margin-bottom:20px;box-shadow:0 10px 30px rgba(0,0,0,0.2)}
h1{color:#333;margin-bottom:10px;font-size:28px}
h2{color:#555;margin-bottom:15px;font-size:20px;border-bottom:2px solid #667eea;padding-bottom:8px}
.header{display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px}
.status{background:#e8f5e9;padding:15px;border-radius:8px;margin-bottom:20px}
.status-item{margin:8px 0;font-size:16px}
.status-item strong{color:#2e7d32}
.live-badge{background:#28a745;color:#fff;padding:5px 12px;border-radius:20px;font-size:14px;font-weight:600;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.7}}
form{display:grid;gap:12px}
input,select{padding:12px;border:2px solid #ddd;border-radius:6px;font-size:14px;width:100%}
input:focus,select:focus{border-color:#667eea;outline:none}
button{padding:12px 24px;background:#667eea;color:#fff;border:none;border-radius:6px;font-size:16px;cursor:pointer;transition:.3s}
button:hover{background:#5568d3}
button.danger{background:#dc3545}
button.danger:hover{background:#c82333}
button.success{background:#28a745}
button.success:hover{background:#218838}
button.warning{background:#ffc107;color:#000}
button.warning:hover{background:#e0a800}
.uid-display{background:#fff3cd;padding:15px;border-radius:8px;margin-bottom:15px;border:2px solid #ffc107}
.uid-value{font-size:20px;font-weight:bold;color:#856404;font-family:monospace}
table{width:100%;border-collapse:collapse;margin-top:15px;font-size:14px}
th,td{padding:10px;text-align:left;border-bottom:1px solid #ddd}
th{background:#667eea;color:#fff;font-weight:600;position:sticky;top:0}
tr:hover{background:#f5f5f5}
.btn-group{display:flex;gap:10px;margin-top:15px;flex-wrap:wrap}
.table-container{max-height:500px;overflow-y:auto;margin-top:15px}
.logout-btn{background:#dc3545;padding:8px 16px;border-radius:6px;color:#fff;text-decoration:none;font-size:14px}
.logout-btn:hover{background:#c82333}
.entry{color:#28a745;font-weight:600}
.exit{color:#dc3545;font-weight:600}
.ongoing{color:#ffc107;font-weight:600}
@media(max-width:600px){
.header{flex-direction:column;align-items:flex-start}
.btn-group{flex-direction:column}
table{font-size:12px}
th,td{padding:6px}
}
</style>
</head>
<body>
<div class="container">

<div class="card">
<div class="header">
<div>
<h1>🔐 RFID Attendance System</h1>
<span class="live-badge">● LIVE</span>
</div>
<a href="/logout" class="logout-btn">Logout</a>
</div>
<div class="status">
<div class="status-item"><strong>Time:</strong> <span id="time">Loading...</span></div>
<div class="status-item"><strong>Status:</strong> <span style="color:#28a745">Online</span></div>
<div class="status-item"><strong>Total Cards:</strong> <span id="totalCards">0</span></div>
</div>
</div>

<div class="card">
<h2>➕ Add New Card</h2>
<div class="uid-display">
<div style="margin-bottom:8px;color:#856404;font-weight:600">Scanned UID:</div>
<div class="uid-value" id="uidDisplay">Waiting for card...</div>
</div>
<form id="addForm">
<input type="text" id="uid" name="uid" placeholder="UID (auto-filled)" readonly required>
<input type="text" name="name" placeholder="Full Name" required>
<input type="text" name="course" placeholder="Course" required>
<input type="text" name="shift" placeholder="Shift (Morning/Evening)" required>
<input type="text" name="phone" placeholder="Phone Number" required>
<input type="text" name="enrollment" placeholder="Enrollment Number" required>
<button type="submit" class="success">Add Card</button>
</form>
</div>

<div class="card">
<h2>📋 Registered Cards</h2>
<div class="btn-group">
<button onclick="loadCards()" class="success">Refresh Cards</button>
<button onclick="deleteAllCards()" class="danger">Delete All Cards</button>
</div>
<div class="table-container">
<div id="cardsList">Loading...</div>
</div>
</div>

<div class="card">
<h2>📊 Attendance Logs (Live)</h2>
<div class="btn-group">
<button onclick="loadLogs()" class="success">Refresh Logs</button>
<button onclick="downloadLogs()">Download CSV</button>
<button onclick="clearLogs()" class="danger">Clear All Logs</button>
</div>
<div class="table-container">
<div id="logsList">Loading...</div>
</div>
</div>

</div>

<script>
let lastUID = '';
let autoRefresh = true;

// Auto-fetch UID every 500ms
setInterval(() => {
  fetch('/getUID').then(r => r.text()).then(uid => {
    if (uid && uid !== 'NONE' && uid !== lastUID) {
      lastUID = uid;
      document.getElementById('uid').value = uid;
      document.getElementById('uidDisplay').textContent = uid;
      document.getElementById('uidDisplay').style.color = '#28a745';
    }
  }).catch(e => console.error(e));
}, 500);

// Update time every second
setInterval(() => {
  fetch('/getTime').then(r => r.text()).then(time => {
    document.getElementById('time').textContent = time;
  }).catch(e => console.error(e));
}, 1000);

// Auto-refresh logs every 3 seconds
setInterval(() => {
  if (autoRefresh) {
    loadLogs();
    loadCards();
  }
}, 3000);

// Add card
document.getElementById('addForm').onsubmit = async (e) => {
  e.preventDefault();
  const formData = new FormData(e.target);
  const params = new URLSearchParams(formData);
  
  const res = await fetch('/addCard?' + params);
  const result = await res.text();
  
  alert(result);
  if (result.includes('Success')) {
    e.target.reset();
    lastUID = '';
    document.getElementById('uidDisplay').textContent = 'Waiting for card...';
    document.getElementById('uidDisplay').style.color = '#856404';
    loadCards();
  }
};

// Load cards
async function loadCards() {
  const res = await fetch('/getCards');
  const html = await res.text();
  document.getElementById('cardsList').innerHTML = html;
  
  // Update total count
  const matches = html.match(/<tr>/g);
  const count = matches ? matches.length - 1 : 0;
  document.getElementById('totalCards').textContent = count;
}

// Load logs
async function loadLogs() {
  const res = await fetch('/getLogs');
  const html = await res.text();
  document.getElementById('logsList').innerHTML = html;
}

// Delete card
async function deleteCard(uid) {
  if (!confirm('Delete this card?')) return;
  const res = await fetch('/deleteCard?uid=' + uid);
  alert(await res.text());
  loadCards();
}

// Delete all cards
async function deleteAllCards() {
  if (!confirm('Delete ALL cards? This cannot be undone!')) return;
  if (!confirm('Are you absolutely sure? This will delete all registered cards!')) return;
  const res = await fetch('/deleteAllCards');
  alert(await res.text());
  loadCards();
}

// Download logs
function downloadLogs() {
  window.location.href = '/downloadLogs';
}

// Clear logs
async function clearLogs() {
  if (!confirm('Clear all attendance logs? This cannot be undone!')) return;
  const res = await fetch('/clearLogs');
  alert(await res.text());
  loadLogs();
}

// Initial load
loadCards();
loadLogs();
</script>
</body>
</html>
)rawliteral";

void setupWebServer() {
  // Login page
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("username") && request->hasParam("password")) {
      String user = request->getParam("username")->value();
      String pass = request->getParam("password")->value();
      
      if (user == ADMIN_USER && pass == ADMIN_PASS) {
        sessionToken = generateToken();
        sessionExpiry = millis() + SESSION_DURATION;
        
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Set-Cookie", "session=" + sessionToken + "; Path=/; Max-Age=3600");
        request->send(response);
      } else {
        request->send(401, "text/plain", "Invalid");
      }
    } else {
      request->send_P(200, "text/html", LOGIN_PAGE);
    }
  });
  
  // Logout
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    sessionToken = "";
    sessionExpiry = 0;
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", "/login");
    response->addHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
    request->send(response);
  });
  
  // Main page (protected)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
      AsyncWebServerResponse *response = request->beginResponse(302);
      response->addHeader("Location", "/login");
      request->send(response);
      return;
    }
    request->send_P(200, "text/html", MAIN_PAGE);
  });
  
  // Protected endpoints
  server.on("/getUID", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    request->send(200, "text/plain", lastUnknownUID.length() > 0 ? lastUnknownUID : "NONE");
  });
  
  server.on("/getTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    request->send(200, "text/plain", currentTimeString);
  });
  
  server.on("/addCard", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    if (request->hasParam("uid") && request->hasParam("name")) {
      String uid = request->getParam("uid")->value();
      String name = request->getParam("name")->value();
      String course = request->getParam("course")->value();
      String shift = request->getParam("shift")->value();
      String phone = request->getParam("phone")->value();
      String enrollment = request->getParam("enrollment")->value();
      
      String cardData = uid + "|" + name + "|" + course + "|" + shift + "|" + phone + "|" + enrollment + "\n";
      appendFileFast(CARDS_FILE, cardData);
      
      lastUnknownUID = "";
      request->send(200, "text/plain", "Success! Card added for " + name);
    } else {
      request->send(400, "text/plain", "Error: Missing parameters");
    }
  });
  
  server.on("/getCards", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    String cards = readFileFast(CARDS_FILE);
    if (cards == "") {
      request->send(200, "text/html", "<p style='color:#999;text-align:center;padding:20px'>No cards registered yet.</p>");
      return;
    }
    
    String html = "<table><tr><th>UID</th><th>Name</th><th>Course</th><th>Shift</th><th>Phone</th><th>Enrollment</th><th>Action</th></tr>";
    int start = 0;
    while (start < cards.length()) {
      int end = cards.indexOf('\n', start);
      if (end == -1) end = cards.length();
      
      String line = cards.substring(start, end);
      if (line.length() > 0) {
        int p[6];
        p[0] = line.indexOf('|');
        for (int i = 1; i < 6; i++) {
          p[i] = line.indexOf('|', p[i-1] + 1);
        }
        
        String uid = line.substring(0, p[0]);
        String name = line.substring(p[0]+1, p[1]);
        String course = line.substring(p[1]+1, p[2]);
        String shift = line.substring(p[2]+1, p[3]);
        String phone = line.substring(p[3]+1, p[4]);
        String enrollment = line.substring(p[4]+1);
        
        html += "<tr><td>" + uid + "</td><td>" + name + "</td><td>" + course + "</td><td>" + shift + "</td><td>" + phone + "</td><td>" + enrollment + "</td>";
        html += "<td><button class='danger' onclick='deleteCard(\"" + uid + "\")'>Delete</button></td></tr>";
      }
      start = end + 1;
    }
    html += "</table>";
    request->send(200, "text/html", html);
  });
  
  server.on("/deleteCard", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    if (request->hasParam("uid")) {
      String uid = request->getParam("uid")->value();
      if (deleteCardByUID(uid)) {
        request->send(200, "text/plain", "Card deleted successfully!");
      } else {
        request->send(404, "text/plain", "Card not found!");
      }
    } else {
      request->send(400, "text/plain", "Error: No UID provided");
    }
  });
  
  server.on("/deleteAllCards", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    writeFileFast(CARDS_FILE, "");
    writeFileFast(STATUS_FILE, "");
    request->send(200, "text/plain", "All cards deleted successfully!");
  });
  
  server.on("/getLogs", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    String logs = readFileFast(LOGS_FILE);
    if (logs == "") {
      request->send(200, "text/html", "<p style='color:#999;text-align:center;padding:20px'>No logs yet.</p>");
      return;
    }
    
    String html = "<table><tr><th>UID</th><th>Name</th><th>Entry Time</th><th>Exit Time</th><th>Status</th></tr>";
    int start = 0;
    int count = 0;
    
    while (start < logs.length() && count < 200) {
      int end = logs.indexOf('\n', start);
      if (end == -1) end = logs.length();
      
      String line = logs.substring(start, end);
      if (line.length() > 0) {
        // Parse: UID|Name|EntryTime|ExitTime|Status
        int p1 = line.indexOf('|');
        int p2 = line.indexOf('|', p1 + 1);
        int p3 = line.indexOf('|', p2 + 1);
        int p4 = line.indexOf('|', p3 + 1);
        
        if (p1 != -1 && p2 != -1) {
          String uid = line.substring(0, p1);
          String name = line.substring(p1 + 1, p2);
          String entryTime = line.substring(p2 + 1, p3);
          String exitTime = (p3 != -1 && p4 != -1) ? line.substring(p3 + 1, p4) : "";
          String status = (p4 != -1) ? line.substring(p4 + 1) : "Unknown";
          
          String statusClass = "";
          if (status == "Ongoing") statusClass = "ongoing";
          else if (status == "Calculated") statusClass = "exit";
          else if (status == "Exit Only") statusClass = "entry";
          
          html += "<tr><td>" + uid + "</td><td>" + name + "</td><td>" + entryTime + "</td><td>" + (exitTime.length() > 0 ? exitTime : "-") + "</td>";
          html += "<td class='" + statusClass + "'>" + status + "</td></tr>";
          count++;
        }
      }
      start = end + 1;
    }
    html += "</table>";
    request->send(200, "text/html", html);
  });
  
  server.on("/downloadLogs", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    String logs = readFileFast(LOGS_FILE);
    if (logs == "") logs = "UID,Name,Entry Time,Exit Time,Status\n";
    
    // Convert to CSV format
    String csv = "UID,Name,Entry Time,Exit Time,Status\n";
    int start = 0;
    while (start < logs.length()) {
      int end = logs.indexOf('\n', start);
      if (end == -1) end = logs.length();
      
      String line = logs.substring(start, end);
      if (line.length() > 0) {
        line.replace('|', ',');
        csv += line + "\n";
      }
      start = end + 1;
    }
    
    request->send(200, "text/csv", csv);
  });
  
  server.on("/clearLogs", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) return request->send(401, "text/plain", "Unauthorized");
    
    writeFileFast(LOGS_FILE, "");
    request->send(200, "text/plain", "All logs cleared successfully!");
  });
  
  server.begin();
  Serial.println("Web server started!");
}

// ==============================
//  SETUP
// ==============================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS Mounted");
  
  // Initialize files
  if (!SPIFFS.exists(CARDS_FILE)) writeFileFast(CARDS_FILE, "");
  if (!SPIFFS.exists(STATUS_FILE)) writeFileFast(STATUS_FILE, "");
  if (!SPIFFS.exists(LOGS_FILE)) writeFileFast(LOGS_FILE, "");
  
  // WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed!");
  }
  
  // NTP
  setupNTP();
  
  // RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Ready");
  
  // Web Server
  setupWebServer();
  
  beep(2, 100);
  Serial.println("=== SYSTEM READY ===");
  Serial.println("Login: admin / admin123");
}

// ==============================
//  LOOP
// ==============================

void loop() {
  updateTimeNonBlocking();
  handleRFID();
  delay(10);
}
