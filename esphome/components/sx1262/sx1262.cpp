#include "sx1262.h"
#include "esphome/core/log.h"
namespace esphome {
namespace sx1262 {
void SX1262Component::setup_pins_() {
  this->dio1_pin_->setup();   // OUTPUT
  this->reset_pin_->setup();  // OUTPUT
  this->busy_pin_->setup();   // INPUT
  this->spi_setup();
}
void SX1262Component::initialize() {
  int cs = this->internal_cs_pin_->get_pin();
  int dio1 = this->dio1_pin_->get_pin();
  int reset = this->reset_pin_->get_pin();
  int busy = this->busy_pin_->get_pin();
  this->radio = new Module(cs, dio1, reset, busy);
}
}  // namespace sx1262
}  // namespace esphome
