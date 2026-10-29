#!/usr/bin/env python3
"""Optional Windows/Linux host tool; pip install pyserial.
Examples:
  python tools/serial_capture.py COM6
  python tools/serial_capture.py /dev/ttyUSB0
"""
import sys
try:
    import serial
except ImportError as exc:
    raise SystemExit('Install pyserial: python -m pip install pyserial') from exc

if len(sys.argv) != 2:
    raise SystemExit('Usage: python tools/serial_capture.py <COM6|/dev/ttyUSB0>')
with serial.Serial(sys.argv[1], 115200, timeout=0.5) as port:
    print('Serial open. Ctrl+C to quit. Type a command then Enter.')
    import threading
    running = True
    def reader():
        while running:
            data = port.read(1000)
            if data:
                sys.stdout.write(data.decode('utf-8', 'replace'))
                sys.stdout.flush()
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    try:
        while True:
            command = input()
            port.write((command + '\r').encode('utf-8'))
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        running = False
