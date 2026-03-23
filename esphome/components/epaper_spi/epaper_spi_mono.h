#pragma once

#include "epaper_spi.h"

namespace esphome::epaper_spi {
/**
 * A class for monochrome epaper displays.
 */
class EPaperMono : public EPaperBase {
 public:
  EPaperMono(const char *name, uint16_t width, uint16_t height, const uint8_t *init_sequence,
             size_t init_sequence_length)
      : EPaperBase(name, width, height, init_sequence, init_sequence_length, DISPLAY_TYPE_BINARY) {
    this->buffer_length_ = (width + 7) / 8 * height;  // 8 pixels per byte, rounded up
  }

 protected:
  void refresh_screen(bool partial) override;
  void power_on() override {}
  void power_off() override{};
  void deep_sleep() override;
  bool reset() override;
  virtual void set_window();
  bool transfer_data() override;
  bool send_red_{true};
};

/**
 * Good Display GDEY029T94 (SSD1680) monochrome panel.
 *
 * The SSD1680 controller expects:
 * - 2-byte X window addresses (byte units, not pixel units) for 0x44/0x4E
 * - deep sleep mode byte 0x01 after command 0x10
 * - only a single BW RAM plane (no red/secondary plane needed)
 */
class EPaperGDEY029T94 final : public EPaperMono {
 public:
  EPaperGDEY029T94(const char *name, uint16_t width, uint16_t height, const uint8_t *init_sequence,
                   size_t init_sequence_length)
      : EPaperMono(name, width, height, init_sequence, init_sequence_length) {
    // No red plane — skip the secondary buffer pass
    this->send_red_ = false;
  }

 protected:
  void set_window() override;
  void deep_sleep() override;
};

}  // namespace esphome::epaper_spi
