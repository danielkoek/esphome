
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "config.h"

namespace esphome {
namespace sx1262 {
static const char *const TAG = "SX1262";
class SX1262Component : public Component,
                        public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                              spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_8MHZ> {
 public:
  void set_cs_pin(InternalGPIOPin *cs) { this->internal_cs_pin_ = cs; }
  void set_reset_pin(InternalGPIOPin *reset) { this->reset_pin_ = reset; }
  void set_busy_pin(InternalGPIOPin *busy) { this->busy_pin_ = busy; }
  void set_dio1_pin(InternalGPIOPin *dio1) { this->dio1_pin_ = dio1; }
  void set_dio2_pin(InternalGPIOPin *dio2) { this->dio2_pin_ = dio2; }
  void set_repeater(bool enable) { repeater_enabled_ = enable; }
  virtual void initialize() = 0;

  void loop() override;
  void dump_config() override;
  void setup() override {
    this->setup_pins_();
    this->initialize();
  }

 protected:
  void setup_pins_();
  SX1262 radio;
  bool repeater_enabled_ = false;
  InternalGPIOPin *internal_cs_pin_;
  InternalGPIOPin *dio1_pin_;
  InternalGPIOPin *dio2_pin_;
  InternalGPIOPin *reset_pin_;
  InternalGPIOPin *busy_pin_;
};
}  // namespace sx1262
}  // namespace esphome
