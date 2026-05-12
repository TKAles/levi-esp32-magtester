"""LEARN mode view."""

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

_LEARN_SUB_LABELS = {
    0: ("READY", "#58a6ff"),
    1: ("LEARNING…", "#fde047"),
    2: ("DONE — SAVED", "#4ade80"),
    3: ("ERROR — NVS WRITE FAILED", "#f87171"),
}


class LearnView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        root = QVBoxLayout(self)
        root.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.setSpacing(24)

        # Mode title
        title = QLabel("LEARN MODE")
        title.setObjectName("mode_title")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(title)

        # Status card
        card = QFrame()
        card.setObjectName("info_card")
        card.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        card_layout = QVBoxLayout(card)
        card_layout.setSpacing(18)
        card_layout.setContentsMargins(40, 32, 40, 32)

        self._status_label = QLabel("READY")
        self._status_label.setObjectName("status_big")
        self._status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        card_layout.addWidget(self._status_label)

        sep = QFrame()
        sep.setFrameShape(QFrame.Shape.HLine)
        sep.setObjectName("separator")
        card_layout.addWidget(sep)

        # Baseline data indicator
        row1 = QHBoxLayout()
        row1.addWidget(QLabel("Baseline data:"))
        self._data_label = QLabel("—")
        self._data_label.setObjectName("value_label")
        row1.addStretch()
        row1.addWidget(self._data_label)
        card_layout.addLayout(row1)

        # Num samples
        row2 = QHBoxLayout()
        row2.addWidget(QLabel("Samples per capture:"))
        self._samples_label = QLabel("—")
        self._samples_label.setObjectName("value_label")
        row2.addStretch()
        row2.addWidget(self._samples_label)
        card_layout.addLayout(row2)

        root.addWidget(card)

        # Hint
        hint = QLabel("Press  D1  to capture a new baseline")
        hint.setObjectName("hint_label")
        hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(hint)

    # ------------------------------------------------------------------

    def update_state(self, state: dict) -> None:
        ls = state.get("ls", 0)
        label_text, color = _LEARN_SUB_LABELS.get(ls, ("READY", "#58a6ff"))
        self._status_label.setText(label_text)
        self._status_label.setStyleSheet(f"color: {color};")

        has_data = state.get("hd", 0)
        if has_data:
            self._data_label.setText("✓  Stored")
            self._data_label.setStyleSheet("color: #4ade80;")
        else:
            self._data_label.setText("✗  None")
            self._data_label.setStyleSheet("color: #f87171;")

        self._samples_label.setText(str(state.get("ns", "—")))
