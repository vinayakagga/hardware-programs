import argparse, os, time
import serial

BAUD=460800

def drain(ser):
    while ser.in_waiting:
        s=ser.readline().decode("utf-8","replace").strip()
        if s: print("ARDUINO:",s)

def upload(ser, path, remote):
    size=os.path.getsize(path)
    print(f"\nUploading {path} -> {remote} ({size} bytes)")
    ser.write(f"PUT {remote} {size}\n".encode("ascii"))
    ser.flush()

    deadline=time.time()+8
    while time.time()<deadline:
        line=ser.readline().decode("utf-8","replace").strip()
        if not line: continue
        print("ARDUINO:",line)
        if line=="READY": break
        if line.startswith("ERR"): raise RuntimeError(line)
    else:
        raise RuntimeError("Arduino did not answer READY")

    sent=0
    with open(path,"rb") as f:
        while True:
            b=f.read(4096)
            if not b: break
            ser.write(b)
            sent+=len(b)
            print(f"\r{sent}/{size} ({sent*100/size:5.1f}%)",end="",flush=True)
    ser.flush()
    print()

    deadline=time.time()+30
    while time.time()<deadline:
        line=ser.readline().decode("utf-8","replace").strip()
        if not line: continue
        print("ARDUINO:",line)
        if line.startswith("OK "): return
        if line.startswith("ERR"): raise RuntimeError(line)
    raise RuntimeError("Timed out waiting for OK")

def list_files(ser):
    ser.write(b"LIST\n"); ser.flush()
    while True:
        line=ser.readline().decode("utf-8","replace").strip()
        if line:
            print(line)
            if line=="LIST_END": break

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--port",required=True)
    p.add_argument("--folder")
    p.add_argument("--file",nargs=2,metavar=("LOCAL","REMOTE"))
    p.add_argument("--list",action="store_true")
    a=p.parse_args()

    ser=serial.Serial(a.port,BAUD,timeout=.5,write_timeout=5)
    time.sleep(1.5)
    drain(ser)

    try:
        if a.list: list_files(ser)

        if a.file:
            upload(ser,a.file[0],a.file[1])

        if a.folder:
            for name in sorted(os.listdir(a.folder)):
                path=os.path.join(a.folder,name)
                if os.path.isfile(path):
                    upload(ser,path,name)
    finally:
        ser.close()

if __name__=="__main__":
    main()
