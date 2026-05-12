"""Main application window."""

from __future__ import annotations

import time

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont, QKeySequence, QShortcut
from PyQt6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QSizePolicy,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from serial_worker import SerialWorker
from test_view import TestView
from learn_view import LearnView
from verify_view import VerifyView
from options_view import OptionsView

_MODE_NAMES = {0: "TEST", 1: "LEARN", 2: "VERIFY", 3: "OPTIONS"}
_MODE_KEYS  = list(_MODE_NAMES.keys())


class MainWindow(QMainWindow):
    def __init__(self, worker: SerialWorker, parent=None):
        super().__init__(parent)
        self._worker = worker
        self._last_packet_time: float = 0.0
        self._packet_count: int = 0
        self._current_mode: int = 0

        self.setWindowTitle("MagTester Companion")
        self._build_ui()
        self._connect_signals()

        # Staleness indicator timer
        self._stale_timer = QTimer(self)
        self._stale_timer.timeout.connect(self._update_staleness)
        self._stale_timer.start(500)

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_ui(self) -> None:
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setSpacing(0)
        root.setContentsMargins(0, 0, 0, 0)

        root.addWidget(self._build_header())
        root.addWidget(self._build_mode_bar())
        root.addWidget(self._build_content_stack(), stretch=1)
        root.addWidget(self._build_control_bar())

    def _build_header(self) -> QFrame:
        frame = QFrame()
        frame.setObjectName("header_frame")
        frame.setFixedHeight(56)
        layout = QHBoxLayout(frame)
        layout.setContentsMargins(20, 0, 20, 0)

        logo = QLabel("⬡  MAGTESTER COMPANION")
        logo.setObjectName("app_logo")
        layout.addWidget(logo)

        layout.addStretch()

        self._conn_dot = QLabel("●")
        self._conn_dot.setObjectName("conn_dot_green")
        layout.addWidget(self._conn_dot)

        self._conn_label = QLabel(f"Connected: {self._worker._port}  @  {self._worker._baud}")
        self._conn_label.setObjectName("conn_label")
        layout.addWidget(self._conn_label)

        return frame

    def _build_mode_bar(self) -> QFrame:
        frame = QFrame()
        frame.setObjectName("mode_bar")
        frame.setFixedHeight(44)
        layout = QHBoxLayout(frame)
        layout.setContentsMargins(20, 0, 20, 0)
        layout.setSpacing(8)

        self._mode_tabs: dict[int, QLabel] = {}
        for mode_id, name in _MODE_NAMES.items():
            lbl = QLabel(name)
            lbl.setObjectName("mode_tab")
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl.setFixedHeight(30)
            lbl.setMinimumWidth(100)
            self._mode_tabs[mode_id] = lbl
            layout.addWidget(lbl)

        layout.addStretch()

        self._pkt_label = QLabel("Waiting for device…")
        self._pkt_label.setObjectName("status_text")
        layout.addWidget(self._pkt_label)

        return frame

    def _build_content_stack(self) -> QStackedWidget:
        self._stack = QStackedWidget()

        self._test_view    = TestView()
        self._learn_view   = LearnView()
        self._verify_view  = VerifyView()
        self._options_view = OptionsView()

        self._stack.addWidget(self._test_view)     # index 0
        self._stack.addWidget(self._learn_view)    # index 1
        self._stack.addWidget(self._verify_view)   # index 2
        self._stack.addWidget(self._options_view)  # index 3

        return self._stack

    def _build_control_bar(self) -> QFrame:
        frame = QFrame()
        frame.setObjectName("control_bar")
        frame.setFixedHeight(72)
        layout = QHBoxLayout(frame)
        layout.setContentsMargins(24, 0, 24, 0)
        layout.setSpacing(16)

        self._d0_btn = QPushButton("D0  ·  MODE ↻")
        self._d0_btn.setObjectName("btn_d0")
        self._d0_btn.setFixedHeight(44)
        self._d0_btn.clicked.connect(self._send_d0)
        layout.addWidget(self._d0_btn)

        self._d1_btn = QPushButton("D1  ·  ACTION")
        self._d1_btn.setObjectName("btn_d1")
        self._d1_btn.setFixedHeight(44)
        self._d1_btn.clicked.connect(self._send_d1)
        layout.addWidget(self._d1_btn)

        layout.addStretch()

        self._stale_label = QLabel("")
        self._stale_label.setObjectName("status_text")
        layout.addWidget(self._stale_label)

        return frame

    # ------------------------------------------------------------------
    # Signal wiring
    # ------------------------------------------------------------------

    def _connect_signals(self) -> None:
        self._worker.state_received.connect(self._on_state)
        self._worker.connection_lost.connect(self._on_disconnect)

        QShortcut(QKeySequence("F11"), self).activated.connect(self._toggle_fullscreen)
        QShortcut(QKeySequence("Escape"), self).activated.connect(self._exit_fullscreen)

    # ------------------------------------------------------------------
    # Slots
    # ------------------------------------------------------------------

    def _on_state(self, state: dict) -> None:
        self._last_packet_time = time.monotonic()
        self._packet_count += 1

        mode = state.get("m", 0)
        if mode != self._current_mode:
            self._current_mode = mode
            self._stack.setCurrentIndex(mode)
            self._refresh_mode_tabs(mode)

        # Delegate to the active view
        view = self._stack.currentWidget()
        if hasattr(view, "update_state"):
            view.update_state(state)

        self._pkt_label.setText(f"Packets: {self._packet_count:,}")

    def _on_disconnect(self, reason: str) -> None:
        self._conn_dot.setObjectName("conn_dot_red")
        self._conn_dot.setStyleSheet("color: #f87171;")
        self._conn_label.setText(f"Disconnected — {reason}")
        self._d0_btn.setEnabled(False)
        self._d1_btn.setEnabled(False)

    def _update_staleness(self) -> None:
        if self._last_packet_time == 0.0:
            return
        age = time.monotonic() - self._last_packet_time
        if age < 1.0:
            self._stale_label.setText(f"Last update: {age * 1000:.0f} ms ago")
            self._stale_label.setStyleSheet("color: #4ade80;")
        elif age < 3.0:
            self._stale_label.setText(f"Last update: {age:.1f} s ago")
            self._stale_label.setStyleSheet("color: #fde047;")
        else:
            self._stale_label.setText(f"No data for {age:.1f} s")
            self._stale_label.setStyleSheet("color: #f87171;")

    def _refresh_mode_tabs(self, active: int) -> None:
        for mode_id, lbl in self._mode_tabs.items():
            if mode_id == active:
                lbl.setProperty("active", "true")
                lbl.setStyleSheet(
                    "background: #1f6feb; color: #ffffff; border-radius: 4px; "
                    "font-weight: bold; padding: 0 12px;"
                )
            else:
                lbl.setProperty("active", "false")
                lbl.setStyleSheet(
                    "background: #21262d; color: #8b949e; border-radius: 4px; "
                    "padding: 0 12px;"
                )

    # ------------------------------------------------------------------
    # Button actions
    # ------------------------------------------------------------------

    def _send_d0(self) -> None:
        self._worker.send_command("d0")

    def _send_d1(self) -> None:
        self._worker.send_command("d1")

    # ------------------------------------------------------------------
    # Full-screen toggle
    # ------------------------------------------------------------------

    def _toggle_fullscreen(self) -> None:
        if self.isFullScreen():
            self.showMaximized()
        else:
            self.showFullScreen()

    def _exit_fullscreen(self) -> None:
        if self.isFullScreen():
            self.showMaximized()
