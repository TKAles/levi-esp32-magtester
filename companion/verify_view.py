"""VERIFY mode view."""

from __future__ import annotations

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)


class VerifyView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        root = QVBoxLayout(self)
        root.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.setSpacing(24)

        title = QLabel("VERIFY MODE")
        title.setObjectName("mode_title")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(title)

        # Result banner (PASS / FAIL / READY / MEASURING)
        self._result_label = QLabel("READY")
        self._result_label.setObjectName("result_banner")
        self._result_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(self._result_label)

        # Sub-info card
        card = QFrame()
        card.setObjectName("info_card")
        card.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        card_layout = QVBoxLayout(card)
        card_layout.setSpacing(14)
        card_layout.setContentsMargins(40, 28, 40, 28)

        # Settings row
        settings_row = QHBoxLayout()
        settings_row.addWidget(QLabel("Hysteresis:"))
        self._hyst_label = QLabel("—")
        self._hyst_label.setObjectName("value_label")
        settings_row.addWidget(self._hyst_label)
        settings_row.addSpacing(40)
        settings_row.addWidget(QLabel("Samples:"))
        self._samples_label = QLabel("—")
        self._samples_label.setObjectName("value_label")
        settings_row.addWidget(self._samples_label)
        settings_row.addStretch()
        card_layout.addLayout(settings_row)

        sep = QFrame()
        sep.setFrameShape(QFrame.Shape.HLine)
        sep.setObjectName("separator")
        card_layout.addWidget(sep)

        # Sensor failure detail area (scrollable)
        self._detail_label = QLabel("—")
        self._detail_label.setObjectName("detail_text")
        self._detail_label.setWordWrap(True)
        self._detail_label.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)

        scroll = QScrollArea()
        scroll.setWidget(self._detail_label)
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)
        scroll.setMaximumHeight(200)
        card_layout.addWidget(scroll)

        root.addWidget(card)

        hint = QLabel("Press  D1  to run a verification")
        hint.setObjectName("hint_label")
        hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
        root.addWidget(hint)

    # ------------------------------------------------------------------

    def update_state(self, state: dict) -> None:
        vs = state.get("vs", 0)
        has_data = state.get("hd", 0)

        self._hyst_label.setText(f"±{state.get('hy', '—')}")
        self._samples_label.setText(str(state.get("ns", "—")))

        if vs == 0:  # idle
            if not has_data:
                self._result_label.setText("NO BASELINE")
                self._result_label.setStyleSheet("color: #f87171;")
                self._detail_label.setText("Run LEARN mode first to capture a baseline.")
            else:
                self._result_label.setText("READY")
                self._result_label.setStyleSheet("color: #58a6ff;")
                self._detail_label.setText("Press D1 to compare sensors against the stored baseline.")
        elif vs == 1:  # measuring
            self._result_label.setText("MEASURING…")
            self._result_label.setStyleSheet("color: #fde047;")
            self._detail_label.setText("Collecting sensor samples…")
        elif vs == 2:  # pass
            self._result_label.setText("✓  PASS")
            self._result_label.setStyleSheet("color: #4ade80;")
            self._detail_label.setText("All 28 sensors are within tolerance.")
        elif vs == 3:  # fail
            self._result_label.setText("✗  FAIL")
            self._result_label.setStyleSheet("color: #f87171;")
            self._build_fail_detail(state)

    def _build_fail_detail(self, state: dict) -> None:
        bad_indices = state.get("vb", [])
        averaged    = state.get("va", [])
        learned     = state.get("lb", [])
        n_bad       = state.get("vn", 0)
        hyst        = state.get("hy", 0)

        lines = [f"<b>{n_bad} sensor(s) out of tolerance (±{hyst}):</b><br>"]
        for i, idx in enumerate(bad_indices):
            av  = averaged[i] if i < len(averaged) else "?"
            ref = learned[i]  if i < len(learned)  else "?"
            dev = abs(int(av) - int(ref)) if isinstance(av, int) and isinstance(ref, int) else "?"
            adc_num = idx // 4 + 1
            ch_num  = idx % 4
            lines.append(
                f"  S{idx:02d} (ADC{adc_num} CH{ch_num}):  "
                f"now=<span style='color:#fb923c'>{av}</span>  "
                f"ref=<span style='color:#86efac'>{ref}</span>  "
                f"Δ=<span style='color:#f87171'>{dev}</span><br>"
            )

        self._detail_label.setText("".join(lines))
