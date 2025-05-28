#include "GDEY075T7.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include <cinttypes>
#include <bitset>
// ========================================================
//     Good Display 7.5in black/white
// Datasheet:
//  - https://files.seeedstudio.com/wiki/Other_Display/750-epaper/EN-Image_to_EPD_Instruction_Manual.pdf
// Software example:
//  - https://files.seeedstudio.com/wiki/Other_Display/750-epaper/GDEY075T7%20ESP32%20Sample%20Code.zip
// ========================================================
namespace esphome {
namespace waveshare_epaper {
static const char *const TAG = "waveshare_epaper";
bool GDEY075T7::wait_until_idle_() {
  if (this->busy_pin_ == nullptr || !this->busy_pin_->digital_read()) {
    return true;
  }
  const uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > this->idle_timeout_()) {
      ESP_LOGE(TAG, "Timeout while displaying image!");
      return false;
    }
    App.feed_wdt();
    delay(10);
  }
  return true;
}

void GDEY075T7::reset_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);
  }
}
void GDEY075T7::init_partial_() {
  this->command(0x50);
  this->data(0xA9);
  this->data(0x07);
  this->command(0x91);  // PARTIAL IN

  this->command(0x90);  // PARTIAL WINDOW
  this->data(0x00);
  this->data(0x00);
  this->data((get_width_internal() - 1) >> 8);
  this->data((get_width_internal() - 1) & 0xFF);
  this->data(0x00);
  this->data(0x00);
  this->data((get_height_internal() - 1) >> 8);
  this->data((get_height_internal() - 1) & 0xFF);
  this->data(0x01);  // PARTIAL MODE ENABLE
}
void GDEY075T7::initialize() { this->init_full_(); }
void GDEY075T7::init_full_() {
  this->reset_();
  this->command(0x01);  // POWER SETTING
  this->data(0x07);
  this->data(0x07);
  this->data(0x3F);
  this->data(0x3F);

  this->command(0x06);  // BOOSTER SOFT START
  this->data(0x17);
  this->data(0x17);
  this->data(0x28);
  this->data(0x17);

  this->command(0x04);  // POWER ON
  this->wait_until_idle_();

  this->command(0x00);  // PANEL SETTING
  this->data(0x1F);

  this->command(0x61);  // RESOLUTION SETTING
  this->data(0x03);     // 800
  this->data(0x20);
  this->data(0x01);  // 480
  this->data(0xE0);

  this->command(0x15);
  this->data(0x00);

  this->command(0x50);  // VCOM AND DATA INTERVAL SETTING
  this->data(0x10);
  this->data(0x07);

  this->command(0x60);  // TCON SETTING
  this->data(0x22);
}
void HOT GDEY075T7::display() {
  uint32_t buf_len = this->get_buffer_length_();

  this->command(0x04);
  delay(200);  // NOLINT
  this->wait_until_idle_();

  if (this->full_update_every_ == 1) {
    if (this->at_update_ == 0) {
      ESP_LOGD(TAG, "Full update");
      this->init_full_();
    } else {
      ESP_LOGD(TAG, "Partial update");
      this->init_partial_();
    }

    // Write image data
    this->command(0x13);  // Write RAM
    this->start_data_();
    this->write_array(this->buffer_, this->get_buffer_length_());
    this->end_data_();

    // Refresh display
    this->command(0x12);
    this->wait_until_idle_();

    this->at_update_ = (this->at_update_ + 1) % this->full_update_every_;
  }

  ESP_LOGV(TAG, "Before command(0x02) (>> power off)");
  this->command(0x02);
  this->wait_until_idle_();
  ESP_LOGV(TAG, "After command(0x02) (>> power off)");

  this->at_update_ = (this->at_update_ + 1) % this->full_update_every_;
}

int GDEY075T7::get_width_internal() { return 800; }
int GDEY075T7::get_height_internal() { return 480; }
uint32_t GDEY075T7::idle_timeout_() { return 10000; }
void GDEY075T7::set_full_update_every(uint32_t full_update_every) { this->full_update_every_ = full_update_every; }
void GDEY075T7::dump_config() {
  LOG_DISPLAY("", "E-Paper (Good Display)", this);
  ESP_LOGCONFIG(TAG, "  Model: 7.5in Greyscale GDEY075T7");
  ESP_LOGCONFIG(TAG, "  Full Update Every: %" PRIu32, this->full_update_every_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_UPDATE_INTERVAL(this);
}
}  // namespace waveshare_epaper
}  // namespace esphome
