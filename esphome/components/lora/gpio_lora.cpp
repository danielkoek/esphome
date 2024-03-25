#include "gpio_lora.h"

namespace esphome {
namespace lora {
static const char *const TAGPin = "lora.pin";
void LoraGPIOPin::setup() { pin_mode(flags_); }
void LoraGPIOPin::pin_mode(gpio::Flags flags) {
  if (flags != gpio::FLAG_OUTPUT) {
    ESP_LOGD(TAGPin, "Output only supported");
  }
}
bool LoraGPIOPin::digital_read() { return false; }
void LoraGPIOPin::digital_write(bool value) { this->parent_->digital_write(this->pin_, value != this->inverted_); }
std::string LoraGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%u via Lora", pin_);
  return buffer;
}
}  // namespace lora
}  // namespace esphome
