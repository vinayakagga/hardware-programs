import argparse
import os
import time
import serial


BAUD = 460800
CHUNK_SIZE = 512


def read_line(ser, timeout=10):

    deadline = time.time() + timeout

    while time.time() < deadline:

        line = ser.readline()

        if line:

            return line.decode(
                "utf-8",
                "replace"
            ).strip()

    return ""


def wait_for(ser, expected):

    while True:

        line = read_line(
            ser,
            timeout=15
        )

        if not line:

            raise RuntimeError(
                f"Timeout waiting for {expected}"
            )

        print("ARDUINO:", line)

        if line == expected:
            return

        if line.startswith("ERR"):

            raise RuntimeError(line)


def upload(
    ser,
    local_file,
    remote_file
):

    size = os.path.getsize(
        local_file
    )

    print()
    print(
        f"Uploading {local_file}"
    )

    print(
        f"  SD:   {remote_file}"
    )

    print(
        f"  Size: {size} bytes"
    )

    # --------------------------------------------------------
    # Start upload
    # --------------------------------------------------------

    command = (
        f"PUT {remote_file} {size}\n"
    )

    ser.write(
        command.encode("ascii")
    )

    ser.flush()

    wait_for(
        ser,
        "READY"
    )

    # --------------------------------------------------------
    # Chunked upload with ACK
    # --------------------------------------------------------

    sent = 0

    with open(
        local_file,
        "rb"
    ) as f:

        while sent < size:

            data = f.read(
                CHUNK_SIZE
            )

            if not data:
                break

            ser.write(data)

            ser.flush()

            # Arduino must acknowledge that it has
            # consumed and written this chunk.
            wait_for(
                ser,
                "ACK"
            )

            sent += len(data)

            percent = (
                sent * 100 / size
            )

            print(
                f"\r"
                f"{sent}/{size} "
                f"({percent:5.1f}%)",
                end="",
                flush=True
            )

    print()

    # --------------------------------------------------------
    # Final response
    # --------------------------------------------------------

    while True:

        line = read_line(
            ser,
            timeout=30
        )

        if not line:

            raise RuntimeError(
                "Timeout waiting for final OK"
            )

        print(
            "ARDUINO:",
            line
        )

        if line.startswith("OK "):

            actual = int(
                line.split()[1]
            )

            if actual != size:

                raise RuntimeError(
                    f"Size mismatch: "
                    f"{actual} != {size}"
                )

            return

        if line.startswith("ERR"):

            raise RuntimeError(line)


def list_files(ser):

    ser.write(
        b"LIST\n"
    )

    ser.flush()

    while True:

        line = read_line(
            ser,
            timeout=10
        )

        if not line:

            raise RuntimeError(
                "LIST timeout"
            )

        print(line)

        if line == "LIST_END":
            break


def main():

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--port",
        required=True
    )

    parser.add_argument(
        "--file",
        nargs=2,
        metavar=("LOCAL", "REMOTE")
    )

    parser.add_argument(
        "--list",
        action="store_true"
    )

    args = parser.parse_args()

    ser = serial.Serial(
        args.port,
        BAUD,
        timeout=0.2,
        write_timeout=10
    )

    # Allow UNO USB serial to settle.
    time.sleep(2)

    try:

        if args.list:

            list_files(ser)

        if args.file:

            upload(
                ser,
                args.file[0],
                args.file[1]
            )

    finally:

        ser.close()


if __name__ == "__main__":
    main()