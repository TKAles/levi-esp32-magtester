"""Port selection dialog shown at startup."""

from __future__ import annotations

import serial.tools.list_ports
from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QPushButton,
    QVBoxLayout,
    QWidget,
)


class ConnectDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Connect to MagTester")
        self.setMinimumWidth(480)
        self.setMinimumHeight(340)

        layout = QVBoxLayout(self)
        layout.setSpacing(16)
        layout.setContentsMargins(24, 24, 24, 24)

        # Title
        title = QLabel("Select Serial Port")
        title.setObjectName("dialog_title")
        layout.addWidget(title)

        # Port list + refresh button
        list_row = QHBoxLayout()
        self._port_list = QListWidget()
        self._port_list.itemDoubleClicked.connect(self._accept)
        list_row.addWidget(self._port_list)

        refresh_btn = QPushButton("⟳ Refresh")
        refresh_btn.setObjectName("btn_secondary")
        refresh_btn.setFixedWidth(100)
        refresh_btn.clicked.connect(self._populate_ports)
        list_row.addWidget(refresh_btn, alignment=Qt.AlignmentFlag.AlignTop)
        layout.addLayout(list_row)

        # Baud rate
        baud_row = QHBoxLayout()
        baud_row.addWidget(QLabel("Baud rate:"))
        self._baud_combo = QComboBox()
        for rate in ("115200", "230400", "460800", "921600"):
            self._baud_combo.addItem(rate)
        self._baud_combo.setCurrentText("115200")
        baud_row.addWidget(self._baud_combo)
        baud_row.addStretch()
        layout.addLayout(baud_row)

        # Buttons
        btn_box = QDialogButtonBox()
        self._connect_btn = btn_box.addButton(
            "Connect", QDialogButtonBox.ButtonRole.AcceptRole
        )
        self._connect_btn.setObjectName("btn_primary")
        btn_box.addButton("Cancel", QDialogButtonBox.ButtonRole.RejectRole)
        btn_box.accepted.connect(self._accept)
        btn_box.rejected.connect(self.reject)
        layout.addWidget(btn_box)

        self._populate_ports()

    # ------------------------------------------------------------------

    def _populate_ports(self) -> None:
        self._port_list.clear()
        ports = sorted(serial.tools.list_ports.comports(), key=lambda p: p.device)
        for port in ports:
            desc = f"{port.device}  —  {port.description}"
            item = QListWidgetItem(desc)
            item.setData(Qt.ItemDataRole.UserRole, port.device)
            self._port_list.addItem(item)
        if ports:
            self._port_list.setCurrentRow(0)

    def _accept(self) -> None:
        if self._port_list.currentItem() is None:
            return
        self.accept()

    # ------------------------------------------------------------------
    # Result accessors
    # ------------------------------------------------------------------

    @property
    def selected_port(self) -> str | None:
        item = self._port_list.currentItem()
        if item is None:
            return None
        return item.data(Qt.ItemDataRole.UserRole)

    @property
    def selected_baud(self) -> int:
        return int(self._baud_combo.currentText())
