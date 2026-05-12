"""Background thread that owns the serial port connection."""

import json
import threading

import serial
from PyQt6.QtCore import QThread, pyqtSignal


class SerialWorker(QThread):
    """Reads JSON state packets from the ESP32 and emits them as Qt signals.

    The thread also exposes ``send_command`` which is safe to call from any
    thread (including the GUI thread).
    """

    state_received = pyqtSignal(dict)
    connection_lost = pyqtSignal(str)

    def __init__(self, port: str, baud: int = 115200, parent=None):
        super().__init__(parent)
        self._port = port
        self._baud = baud
        self._ser: serial.Serial | None = None
        self._stop_event = threading.Event()
        self._write_lock = threading.Lock()

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def send_command(self, cmd: str) -> None:
        """Send a plain-text command line to the firmware (e.g. 'd0' or 'd1')."""
        with self._write_lock:
            if self._ser and self._ser.is_open:
                try:
                    self._ser.write((cmd + "\n").encode())
                except serial.SerialException:
                    pass

    def stop(self) -> None:
        self._stop_event.set()

    # ------------------------------------------------------------------
    # QThread entry point
    # ------------------------------------------------------------------

    def run(self) -> None:
        try:
            self._ser = serial.Serial(self._port, self._baud, timeout=1)
        except serial.SerialException as exc:
            self.connection_lost.emit(str(exc))
            return

        try:
            while not self._stop_event.is_set():
                try:
                    raw = self._ser.readline()
                except serial.SerialException as exc:
                    self.connection_lost.emit(str(exc))
                    return

                if not raw:
                    continue

                line = raw.decode(errors="replace").strip()
                if not line.startswith("{"):
                    continue

                try:
                    packet = json.loads(line)
                except json.JSONDecodeError:
                    continue

                self.state_received.emit(packet)
        finally:
            try:
                self._ser.close()
            except Exception:
                pass
