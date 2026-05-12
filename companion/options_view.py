"""OPTIONS mode view."""

from __future__ import annotations

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

_SETTINGS = [
    ("HYSTERESIS", "hy",  "ADC counts  (0 – 50)"),
    ("NUM SAMPLES", "ns", "Averages per capture  (1 – 50)"),
]


class OptionsView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        root = QVBoxLayout(self)
        root.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.setSpacing(24)

        title = QLabel("OPTIONS")
        title.setObjectName("mode_title")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(title)

        card = QFrame()
        card.setObjectName("info_card")
        card.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        card_layout = QVBoxLayout(card)
        card_layout.setSpacing(0)
        card_layout.setContentsMargins(0, 0, 0, 0)

        self._rows: list[_SettingRow] = []
        for i, (name, key, desc) in enumerate(_SETTINGS):
            row = _SettingRow(i, name, desc)
            card_layout.addWidget(row)
            if i < len(_SETTINGS) - 1:
                sep = QFrame()
                sep.setFrameShape(QFrame.Shape.HLine)
                sep.setObjectName("separator")
                card_layout.addWidget(sep)
            self._rows.append(row)

        root.addWidget(card)

        hint = QLabel(
            "D0 → advance / exit  ·  D1 → increment selected value\n"
            "Values are saved to flash immediately"
        )
        hint.setObjectName("hint_label")
        hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(hint)

    # ------------------------------------------------------------------

    def update_state(self, state: dict) -> None:
        cursor = state.get("oc", 0)
        values = [state.get("hy", "—"), state.get("ns", "—")]
        for i, row in enumerate(self._rows):
            row.set_value(str(values[i]))
            row.set_active(i == cursor)


class _SettingRow(QWidget):
    def __init__(self, index: int, name: str, description: str, parent=None):
        super().__init__(parent)
        self._index = index
        layout = QHBoxLayout(self)
        layout.setContentsMargins(40, 20, 40, 20)
        layout.setSpacing(16)

        self._cursor = QLabel("▶")
        self._cursor.setObjectName("cursor_arrow")
        self._cursor.setFixedWidth(20)
        layout.addWidget(self._cursor)

        info = QVBoxLayout()
        self._name_label = QLabel(name)
        self._name_label.setObjectName("setting_name")
        info.addWidget(self._name_label)
        desc_label = QLabel(description)
        desc_label.setObjectName("setting_desc")
        info.addWidget(desc_label)
        layout.addLayout(info)

        layout.addStretch()

        self._value_label = QLabel("—")
        self._value_label.setObjectName("setting_value")
        layout.addWidget(self._value_label)

    def set_value(self, v: str) -> None:
        self._value_label.setText(v)

    def set_active(self, active: bool) -> None:
        self._cursor.setVisible(active)
        bg = "#1c2d3a" if active else "transparent"
        self.setStyleSheet(f"_SettingRow {{ background: {bg}; }}")
        self._name_label.setStyleSheet("color: #fde047;" if active else "color: #e6edf3;")
        self._value_label.setStyleSheet(
            "color: #fde047; font-size: 28px;" if active else "color: #e6edf3; font-size: 28px;"
        )
