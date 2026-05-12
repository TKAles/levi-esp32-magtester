"""Background thread that owns the serial port connection."""

import json
import queue
import threading

import serial
from PyQt6.QtCore import QThread, pyqtSignal


class SerialWorker(QThread):
    """Reads JSON state packets from the ESP32 and emits them as Qt signals.

    ``send_command`` is safe to call from any thread — it enqueues the
    command and the worker thread performs the actual write, keeping the
    GUI thread unblocked.
    """

    state_received = pyqtSignal(dict)
    connection_lost = pyqtSignal(str)

    def __init__(self, port: str, baud: int = 115200, parent=None):
        super().__init__(parent)
        self._port = port
        self._baud = baud
        self._ser: serial.Serial | None = None
        self._stop_event = threading.Event()
        self._cmd_queue: queue.Queue[str] = queue.Queue()

    # ------------------------------------------------------------------
    # Public API (safe to call from any thread)
    # ------------------------------------------------------------------

    def send_command(self, cmd: str) -> None:
        """Enqueue a plain-text command (e.g. 'd0' or 'd1') for sending."""
        self._cmd_queue.put_nowait(cmd)

    def stop(self) -> None:
        self._stop_event.set()

    # ------------------------------------------------------------------
    # QThread entry point — all serial I/O happens here
    # ------------------------------------------------------------------

    def run(self) -> None:
        try:
            self._ser = serial.Serial(
                self._port,
                self._baud,
                timeout=0.05,       # short read timeout keeps the loop responsive
                write_timeout=1.0,
            )
        except serial.SerialException as exc:
            self.connection_lost.emit(str(exc))
            return

        line_buf = bytearray()

        try:
            while not self._stop_event.is_set():
                # --- flush outgoing command queue ---
                try:
                    while True:
                        cmd = self._cmd_queue.get_nowait()
                        try:
                            self._ser.write((cmd + "\n").encode())
                        except serial.SerialException:
                            pass
                except queue.Empty:
                    pass

                # --- read whatever bytes are available ---
                try:
                    # read up to 256 bytes; in_waiting may be 0 between packets
                    waiting = self._ser.in_waiting
                    chunk = self._ser.read(max(waiting, 1))
                except serial.SerialException as exc:
                    self.connection_lost.emit(str(exc))
                    return

                for byte in chunk:
                    if byte == ord('\n'):
                        line = line_buf.decode(errors="replace").strip()
                        line_buf.clear()
                        if line.startswith("{"):
                            try:
                                packet = json.loads(line)
                                self.state_received.emit(packet)
                            except json.JSONDecodeError:
                                pass
                    elif byte != ord('\r'):
                        line_buf.append(byte)

        finally:
            try:
                self._ser.close()
            except Exception:
                pass
