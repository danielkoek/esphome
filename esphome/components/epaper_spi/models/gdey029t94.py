"""Good Display GDEY029T94 2.9" monochrome e-paper display using SSD1680 controller.

Supported models:
- gdey029t94: 128x296 pixels (2.9" display)

This display uses the SSD1680 controller with internal temperature sensor
for waveform generation. Supports full and partial refresh via the
built-in display update control register.

Datasheet:
- https://files.seeedstudio.com/wiki/Other_Display/29-epaper/GDEY029T94.pdf
- https://github.com/Allen-Kuang/e-ink_Demo/blob/main/2.9%20inch%20E-paper%20-%20monocolor%20128x296/example/Display_EPD_W21.cpp
"""

from . import EpaperModel


class GDEY029T94Model(EpaperModel):
    """EpaperModel for GDEY029T94 monochrome display using SSD1680 controller."""

    def __init__(self, name, **defaults):
        super().__init__(name, "EPaperMono", **defaults)

    def get_init_sequence(self, config):
        """Generate initialization sequence for GDEY029T94 display.

        The initialization sequence is based on the SSD1680 controller datasheet.
        The DRV_OUT_CTL command is calculated from the display height.
        """
        _, height = self.get_dimensions(config)
        height_minus_1 = height - 1
        msb = height_minus_1 >> 8
        lsb = height_minus_1 & 0xFF
        return (
            # DRV_OUT_CTL - driver output control (height-dependent)
            (0x01, lsb, msb, 0x00),
            # DATA_ENTRY - data entry mode (0x03 = increment Y, increment X)
            (0x11, 0x03),
            # BORDER_FULL - border waveform control
            (0x3C, 0x05),
            # DISPLAY_UPDATE - display update control
            (0x21, 0x00, 0x80),
            # TEMP_SENS - use internal temperature sensor
            (0x18, 0x80),
        )


GDEY029T94Model(
    "gdey029t94",
    width=128,
    height=296,
    data_rate="10MHz",
    minimum_update_interval="1s",
)
