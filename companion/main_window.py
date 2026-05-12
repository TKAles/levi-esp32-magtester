"""Main application window."""

from __future__ import annotations

import json
import time

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont, QKeySequence, QShortcut
from PyQt6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPlainTextEdit,
    QPushButton,
    QSizePolicy,
    QSplitter,
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

_LOG_MAX_LINES = 500


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

        # Horizontal splitter: mode views on the left, protocol log on the right
        self._splitter = QSplitter(Qt.Orientation.Horizontal)
        self._splitter.setHandleWidth(4)
        self._splitter.setStyleSheet(
            "QSplitter::handle { background: #21262d; }"
        )

        self._stack = self._build_content_stack()
        self._splitter.addWidget(self._stack)

        self._log_panel = self._build_log_panel()
        self._splitter.addWidget(self._log_panel)
        self._log_panel.setVisible(False)

        # Give all initial space to the mode views
        self._splitter.setStretchFactor(0, 1)
        self._splitter.setStretchFactor(1, 0)

        root.addWidget(self._splitter, stretch=1)
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
        stack = QStackedWidget()

        self._test_view    = TestView()
        self._learn_view   = LearnView()
        self._verify_view  = VerifyView()
        self._options_view = OptionsView()

        stack.addWidget(self._test_view)
        stack.addWidget(self._learn_view)
        stack.addWidget(self._verify_view)
        stack.addWidget(self._options_view)

        return stack

    def _build_log_panel(self) -> QFrame:
        panel = QFrame()
        panel.setObjectName("log_panel")
        panel.setMinimumWidth(320)
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Panel header bar
        header = QFrame()
        header.setObjectName("log_panel_header")
        header.setFixedHeight(36)
        hdr_layout = QHBoxLayout(header)
        hdr_layout.setContentsMargins(12, 0, 8, 0)
        hdr_layout.setSpacing(8)

        title = QLabel("PROTOCOL LOG")
        title.setObjectName("log_panel_title")
        hdr_layout.addWidget(title)
        hdr_layout.addStretch()

        self._log_pause_btn = QPushButton("⏸ Pause")
        self._log_pause_btn.setObjectName("btn_log_action")
        self._log_pause_btn.setFixedHeight(24)
        self._log_pause_btn.setCheckable(True)
        self._log_pause_btn.toggled.connect(self._on_log_pause_toggled)
        hdr_layout.addWidget(self._log_pause_btn)

        clear_btn = QPushButton("✕ Clear")
        clear_btn.setObjectName("btn_log_action")
        clear_btn.setFixedHeight(24)
        clear_btn.clicked.connect(self._clear_log)
        hdr_layout.addWidget(clear_btn)

        layout.addWidget(header)

        # Log text area
        self._log_edit = QPlainTextEdit()
        self._log_edit.setReadOnly(True)
        self._log_edit.setObjectName("log_edit")
        self._log_edit.setMaximumBlockCount(_LOG_MAX_LINES)
        self._log_edit.setFont(QFont("Consolas", 10))
        self._log_edit.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
        layout.addWidget(self._log_edit)

        self._log_paused = False
        return panel

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

        layout.addSpacing(16)

        self._log_toggle_btn = QPushButton("◧  Protocol Log")
        self._log_toggle_btn.setObjectName("btn_secondary")
        self._log_toggle_btn.setFixedHeight(36)
        self._log_toggle_btn.setCheckable(True)
        self._log_toggle_btn.toggled.connect(self._toggle_log_panel)
        layout.addWidget(self._log_toggle_btn)

        return frame

    # ------------------------------------------------------------------
    # Signal wiring
    # ------------------------------------------------------------------

    def _connect_signals(self) -> None:
        self._worker.state_received.connect(self._on_state)
        self._worker.raw_line_received.connect(self._on_raw_line)
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

        view = self._stack.currentWidget()
        if hasattr(view, "update_state"):
            view.update_state(state)

        self._pkt_label.setText(f"Packets: {self._packet_count:,}")

    def _on_raw_line(self, line: str) -> None:
        if self._log_paused or not self._log_panel.isVisible():
            return
        # Pretty-print the JSON for readability
        try:
            pretty = json.dumps(json.loads(line), separators=(", ", ":"))
        except Exception:
            pretty = line
        self._log_edit.appendPlainText(pretty)

    def _on_disconnect(self, reason: str) -> None:
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
                lbl.setStyleSheet(
                    "background: #1f6feb; color: #ffffff; border-radius: 4px; "
                    "font-weight: bold; padding: 0 12px;"
                )
            else:
                lbl.setStyleSheet(
                    "background: #21262d; color: #8b949e; border-radius: 4px; "
                    "padding: 0 12px;"
                )

    # ------------------------------------------------------------------
    # Log panel controls
    # ------------------------------------------------------------------

    def _toggle_log_panel(self, checked: bool) -> None:
        self._log_panel.setVisible(checked)
        if checked:
            total = self._splitter.width()
            self._splitter.setSizes([total - 420, 420])
        self._log_toggle_btn.setText("◨  Hide Log" if checked else "◧  Protocol Log")

    def _on_log_pause_toggled(self, paused: bool) -> None:
        self._log_paused = paused
        self._log_pause_btn.setText("▶ Resume" if paused else "⏸ Pause")

    def _clear_log(self) -> None:
        self._log_edit.clear()

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
