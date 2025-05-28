#pragma once

#include "./waveshare_epaper.h"
namespace esphome {
namespace waveshare_epaper {
class GDEY075T7 : public WaveshareEPaper {
 public:
  bool wait_until_idle_();

  void initialize() override;

  void display() override;

  void dump_config() override;

  void deep_sleep() override {
    // COMMAND POWER OFF
    this->command(0x02);
    this->wait_until_idle_();
    // COMMAND DEEP SLEEP
    this->command(0x07);
    this->data(0xA5);  // check byte
  }

  void set_full_update_every(uint32_t full_update_every);

 protected:
  int get_width_internal() override;

  int get_height_internal() override;

  uint32_t idle_timeout_() override;

  uint32_t full_update_every_{30};
  uint32_t at_update_{0};

 private:
  uint8_t *old_buffer_{nullptr};
  void reset_();
  void init_partial_();
  void init_full_();
};
}  // namespace waveshare_epaper
}  // namespace esphome
