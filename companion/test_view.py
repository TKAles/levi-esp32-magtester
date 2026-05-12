"""TEST mode: live 7×4 colour-coded sensor grid."""

from __future__ import annotations

from PyQt6.QtCore import Qt, QRect
from PyQt6.QtGui import QColor, QFont, QPainter, QPen
from PyQt6.QtWidgets import QWidget

ADC_CENTER = 128
NUM_ADCS = 7
CHANNELS_PER_ADC = 4

# Deviation thresholds and their (background, text) colours
_DEVIATION_PALETTE: list[tuple[int, str, str]] = [
    (5,  "#0d3b0d", "#4ade80"),   # near centre — dark green bg, bright green text
    (15, "#1a3d1a", "#86efac"),   # green
    (30, "#3d3000", "#fde047"),   # yellow
    (50, "#3d1a00", "#fb923c"),   # orange
    (80, "#3d0a0a", "#f87171"),   # red
    (999, "#1f0505", "#ef4444"),  # dark red
]


def _cell_colors(val: int) -> tuple[QColor, QColor]:
    dev = abs(int(val) - ADC_CENTER)
    for threshold, bg_hex, fg_hex in _DEVIATION_PALETTE:
        if dev < threshold:
            return QColor(bg_hex), QColor(fg_hex)
    return QColor("#1f0505"), QColor("#ef4444")


class TestView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._sensors: list[int] = [128] * (NUM_ADCS * CHANNELS_PER_ADC)

    def update_state(self, state: dict) -> None:
        raw = state.get("s", [])
        if len(raw) == NUM_ADCS * CHANNELS_PER_ADC:
            self._sensors = list(raw)
            self.update()

    # ------------------------------------------------------------------

    def paintEvent(self, event) -> None:  # noqa: N802
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        w = self.width()
        h = self.height()

        painter.fillRect(0, 0, w, h, QColor("#0d1117"))

        LABEL_MARGIN = 28       # space for ADC / channel labels
        GAP = 8
        CORNER = 6

        avail_w = w - LABEL_MARGIN - GAP * (NUM_ADCS - 1) - GAP * 2
        avail_h = h - LABEL_MARGIN - GAP * (CHANNELS_PER_ADC - 1) - GAP * 2

        cell_w = avail_w // NUM_ADCS
        cell_h = avail_h // CHANNELS_PER_ADC

        cell_size = min(cell_w, cell_h)
        cell_size = max(cell_size, 20)

        grid_w = NUM_ADCS * cell_size + (NUM_ADCS - 1) * GAP
        grid_h = CHANNELS_PER_ADC * cell_size + (CHANNELS_PER_ADC - 1) * GAP

        grid_x = LABEL_MARGIN + (w - LABEL_MARGIN - grid_w) // 2
        grid_y = LABEL_MARGIN + (h - LABEL_MARGIN - grid_h) // 2

        font_size = max(10, int(cell_size * 0.28))
        # Fixed small size for axis labels so they never overflow the margin
        label_font = QFont("Segoe UI", 9)
        value_font = QFont("Consolas", font_size)
        value_font.setBold(True)

        # Column labels (A1 … A7)
        painter.setFont(label_font)
        painter.setPen(QColor("#58a6ff"))
        for adc in range(NUM_ADCS):
            cx = grid_x + adc * (cell_size + GAP)
            lbl_rect = QRect(cx, 2, cell_size, LABEL_MARGIN - 4)
            painter.drawText(lbl_rect, Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter, f"A{adc + 1}")

        # Row labels (C0 … C3)
        for ch in range(CHANNELS_PER_ADC):
            cy = grid_y + ch * (cell_size + GAP)
            lbl_rect = QRect(0, cy, LABEL_MARGIN - 4, cell_size)
            painter.drawText(lbl_rect, Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter, f"C{ch}")

        # Sensor cells
        for adc in range(NUM_ADCS):
            for ch in range(CHANNELS_PER_ADC):
                idx = adc * CHANNELS_PER_ADC + ch
                val = self._sensors[idx] if idx < len(self._sensors) else 0

                cx = grid_x + adc * (cell_size + GAP)
                cy = grid_y + ch * (cell_size + GAP)

                bg_color, fg_color = _cell_colors(val)

                # Cell background
                painter.setBrush(bg_color)
                painter.setPen(Qt.PenStyle.NoPen)
                painter.drawRoundedRect(cx, cy, cell_size, cell_size, CORNER, CORNER)

                # Subtle border
                painter.setPen(QPen(bg_color.lighter(140), 1))
                painter.setBrush(Qt.BrushStyle.NoBrush)
                painter.drawRoundedRect(cx, cy, cell_size, cell_size, CORNER, CORNER)

                # Value text
                painter.setFont(value_font)
                painter.setPen(fg_color)
                painter.drawText(
                    QRect(cx, cy, cell_size, cell_size),
                    Qt.AlignmentFlag.AlignCenter,
                    str(val),
                )

        painter.end()
