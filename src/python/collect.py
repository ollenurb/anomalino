"""
collect.py — Serial IMU data collector for anomaly-detector firmware.

Reads lines from an Arduino over a serial port, parses the 6-DOF IMU values,
and writes them to a timestamped CSV file for later analysis.

Usage example:
    python collect.py --port /dev/ttyACM0 --label walking --max-samples 500
    python collect.py --port /dev/ttyACM0 --output-dir ./data --baudrate 9600
"""

import argparse
import csv
import os
import re
import sys
import time
from datetime import datetime

import serial

# Matches: [float, float, float, float, float, float]
# Tolerates optional spaces around brackets, commas, and signs.
_LINE_RE = re.compile(
    r"\[\s*"
    r"([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*,\s*"
    r"([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*,\s*"
    r"([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*,\s*"
    r"([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*,\s*"
    r"([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*,\s*"
    r"([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*"
    r"\]"
)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Collect IMU data from an Arduino over serial and save to CSV."
    )
    parser.add_argument(
        "--port",
        required=True,
        help="Serial port, e.g. /dev/ttyACM0 or COM3",
    )
    parser.add_argument(
        "--output-dir",
        default="./data",
        help="Directory where CSV files are saved (default: ./data)",
    )
    parser.add_argument(
        "--label",
        default="normal",
        help="Short tag describing the acquisition campaign (default: normal)",
    )
    parser.add_argument(
        "--max-samples",
        type=int,
        default=None,
        help="Stop after this many samples (default: unlimited)",
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        default=9600,
        help="Serial baudrate (default: 9600)",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{args.label}_{timestamp_str}.csv"
    filepath = os.path.join(args.output_dir, filename)

    max_label = str(args.max_samples) if args.max_samples is not None else "INF"
    sample_count = 0

    try:
        with (
            serial.Serial(args.port, args.baudrate, timeout=1) as ser,
            open(filepath, "w", newline="") as csvfile,
        ):
            writer = csv.writer(csvfile)
            writer.writerow(["timestamp", "x", "y", "z", "roll", "pitch", "yaw"])

            print(f"Collecting data → {filepath}")
            print("Press Ctrl+C to stop.\n")

            while True:
                if args.max_samples is not None and sample_count >= args.max_samples:
                    break

                raw = ser.readline()
                if not raw:
                    continue

                try:
                    line = raw.decode("utf-8", errors="replace").strip()
                except Exception as exc:
                    print(f"\nWarning: could not decode bytes: {exc}", file=sys.stderr)
                    continue

                match = _LINE_RE.search(line)
                if not match:
                    if line:  # avoid spamming on empty / timeout lines
                        print(
                            f"\nWarning: malformed line skipped: {line!r}",
                            file=sys.stderr,
                        )
                    continue

                ts = time.time()
                x, y, z, roll, pitch, yaw = (float(v) for v in match.groups())

                writer.writerow([ts, x, y, z, roll, pitch, yaw])
                csvfile.flush()

                sample_count += 1
                sys.stdout.write(
                    f"\r  Samples: {sample_count} / {max_label}   File: {filename}  "
                )
                sys.stdout.flush()
    except KeyboardInterrupt:
        pass  # handled gracefully below
    except serial.SerialException as exc:
        print(f"\nSerial error: {exc}", file=sys.stderr)
        sys.exit(1)

    print(f"\nDone. {sample_count} sample(s) saved to {filepath}")


if __name__ == "__main__":
    main()
