#!/usr/bin/env python3
"""MagTester Companion — entry point."""

import sys

from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import QApplication, QMessageBox

from connect_dialog import ConnectDialog
from main_window import MainWindow
from serial_worker import SerialWorker

# ---------------------------------------------------------------------------
# Application-wide stylesheet
# ---------------------------------------------------------------------------

STYLESHEET = """
/* ── Base ─────────────────────────────────────────────────────────── */
QMainWindow, QWidget, QDialog {
    background-color: #0d1117;
    color: #e6edf3;
    font-family: "Segoe UI", "SF Pro Display", "Helvetica Neue", Arial, sans-serif;
    font-size: 14px;
}

/* ── Header bar ───────────────────────────────────────────────────── */
QFrame#header_frame {
    background-color: #010409;
    border-bottom: 1px solid #21262d;
}
QLabel#app_logo {
    color: #58a6ff;
    font-size: 18px;
    font-weight: bold;
    letter-spacing: 2px;
}
QLabel#conn_dot_green { color: #3fb950; font-size: 16px; }
QLabel#conn_dot_red   { color: #f85149; font-size: 16px; }
QLabel#conn_label     { color: #8b949e; font-size: 13px; }

/* ── Mode bar ─────────────────────────────────────────────────────── */
QFrame#mode_bar {
    background-color: #161b22;
    border-bottom: 1px solid #21262d;
}
QLabel#mode_tab {
    background: #21262d;
    color: #8b949e;
    border-radius: 4px;
    padding: 0 12px;
    font-size: 13px;
    font-weight: bold;
    letter-spacing: 1px;
}

/* ── Content area ─────────────────────────────────────────────────── */
QStackedWidget {
    background-color: #0d1117;
}

/* ── Control bar ──────────────────────────────────────────────────── */
QFrame#control_bar {
    background-color: #161b22;
    border-top: 1px solid #21262d;
}
QPushButton#btn_d0 {
    background-color: #1f6feb;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    font-size: 15px;
    font-weight: bold;
    min-width: 160px;
    padding: 0 24px;
}
QPushButton#btn_d0:hover   { background-color: #388bfd; }
QPushButton#btn_d0:pressed { background-color: #1158c7; }
QPushButton#btn_d0:disabled{ background-color: #21262d; color: #484f58; }

QPushButton#btn_d1 {
    background-color: #238636;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    font-size: 15px;
    font-weight: bold;
    min-width: 160px;
    padding: 0 24px;
}
QPushButton#btn_d1:hover   { background-color: #2ea043; }
QPushButton#btn_d1:pressed { background-color: #196127; }
QPushButton#btn_d1:disabled{ background-color: #21262d; color: #484f58; }

QLabel#status_text { color: #8b949e; font-size: 12px; }

/* ── Shared card / info panel ─────────────────────────────────────── */
QFrame#info_card {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 12px;
    max-width: 800px;
}
QFrame#separator {
    color: #30363d;
    background-color: #30363d;
    max-height: 1px;
}

/* ── Mode view typography ─────────────────────────────────────────── */
QLabel#mode_title {
    color: #58a6ff;
    font-size: 36px;
    font-weight: bold;
    letter-spacing: 4px;
}
QLabel#status_big {
    color: #58a6ff;
    font-size: 42px;
    font-weight: bold;
    letter-spacing: 2px;
}
QLabel#result_banner {
    font-size: 80px;
    font-weight: bold;
    letter-spacing: 6px;
    min-height: 110px;
}
QLabel#value_label {
    color: #e6edf3;
    font-size: 16px;
    font-weight: bold;
}
QLabel#hint_label {
    color: #484f58;
    font-size: 13px;
    font-style: italic;
}
QLabel#detail_text {
    color: #e6edf3;
    font-family: "Consolas", "Courier New", monospace;
    font-size: 13px;
    background: transparent;
    padding: 4px;
}

/* ── Options view ─────────────────────────────────────────────────── */
QLabel#setting_name {
    color: #e6edf3;
    font-size: 18px;
    font-weight: bold;
}
QLabel#setting_desc {
    color: #8b949e;
    font-size: 12px;
}
QLabel#setting_value {
    color: #e6edf3;
    font-size: 28px;
    font-weight: bold;
    min-width: 60px;
}
QLabel#cursor_arrow {
    color: #fde047;
    font-size: 18px;
}

/* ── Connect dialog ───────────────────────────────────────────────── */
QDialog {
    background-color: #0d1117;
}
QLabel#dialog_title {
    color: #58a6ff;
    font-size: 18px;
    font-weight: bold;
}
QListWidget {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 6px;
    color: #e6edf3;
    font-size: 13px;
    padding: 4px;
}
QListWidget::item { padding: 6px 8px; border-radius: 4px; }
QListWidget::item:selected { background-color: #1f6feb; color: #ffffff; }
QListWidget::item:hover    { background-color: #21262d; }
QComboBox {
    background-color: #21262d;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 4px 10px;
    min-width: 120px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background-color: #161b22;
    color: #e6edf3;
    border: 1px solid #30363d;
    selection-background-color: #1f6feb;
}
QPushButton {
    background-color: #21262d;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px 16px;
    font-size: 14px;
}
QPushButton:hover   { background-color: #30363d; }
QPushButton:pressed { background-color: #161b22; }
QPushButton#btn_primary {
    background-color: #1f6feb;
    border-color: #388bfd;
    color: #ffffff;
    font-weight: bold;
}
QPushButton#btn_primary:hover { background-color: #388bfd; }
QPushButton#btn_secondary {
    background-color: #21262d;
    color: #8b949e;
}
QScrollArea, QScrollArea > QWidget > QWidget {
    background: transparent;
}
QScrollBar:vertical {
    background: #161b22;
    width: 8px;
    border-radius: 4px;
}
QScrollBar::handle:vertical {
    background: #30363d;
    border-radius: 4px;
    min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
"""


def main() -> None:
    app = QApplication(sys.argv)
    app.setStyleSheet(STYLESHEET)
    app.setFont(QFont("Segoe UI", 10))

    # Port selection
    dialog = ConnectDialog()
    if dialog.exec() != ConnectDialog.DialogCode.Accepted:
        sys.exit(0)

    port = dialog.selected_port
    baud = dialog.selected_baud

    if not port:
        QMessageBox.critical(None, "No port selected", "Please select a serial port.")
        sys.exit(1)

    # Start serial worker
    worker = SerialWorker(port, baud)
    worker.start()

    # Show main window full-screen
    win = MainWindow(worker)
    win.showFullScreen()

    exit_code = app.exec()

    worker.stop()
    worker.wait(3000)

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
