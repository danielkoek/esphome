#include "sx1262.h"
#include "esphome/core/log.h"
namespace esphome {
namespace sx1262 {
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_RXEN 38
#define SX126X_TXEN RADIOLIB_NC
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
void SX1262Component::setup_pins_() {
  this->dio1_pin_->setup();   // OUTPUT
  this->reset_pin_->setup();  // OUTPUT
  this->busy_pin_->setup();   // INPUT
  this->spi_setup();
}
void SX1262Component::initialize() {
  int lora_cs = this->internal_cs_pin_->get_pin();
  int lora_dio1 = this->dio1_pin_->get_pin();
  int lora_reset = this->reset_pin_->get_pin();
  int lora_busy = this->busy_pin_->get_pin();
  this->radio = new Module(lora_cs, lora_dio1, lora_reset, lora_busy);
  // very random freq for now
  float freq = 868.5;
  float bw = 125;
  uint8_t sf = 9;
  uint8_t cr = 5;
  // same settings as meshtastic
  uint8_t syncWord = 0x2b;
  uint8_t power = 17;
  uint8_t preamble = 16;
  float tcxoVoltage = 1.8;
  bool ldo = false;
  int state = this->radio.begin(freq, bw, sf, cr, syncWord, power, preamble, tcxoVoltage, ldo);
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGD(TAG, "Success, setting swich");
    state = radio.setDio2AsRfSwitch(true);
    if (state == RADIOLIB_ERR_NONE)
      ESP_LOGD(TAG, "Success, setting switch pin");

    // SX1262 rf switch order: setRfSwitchPins(rxEn, txEn);
    radio.setRfSwitchPins(this->dio2_pin_->get_pin(), RADIOLIB_NC);
    ESP_LOGD(TAG, "Success");
  }

  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGD(TAG, "Failed %s", GetCodeDescription(state));
  }
}
void SX1262Component::dump_config() {
  ESP_LOGCONFIG(TAG, "SX1262Component:");
  ESP_LOGCONFIG(TAG, "  Lora CS pin: %u", this->internal_cs_pin_->get_pin());
}
void SX1262Component::update() {
  int numBytes = radio.getPacketLength();
  byte byteArr[numBytes];
  int state = radio.readData(byteArr, numBytes);

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
    ESP_LOGD(TAG, "[SX1262] Received packet!");

    // print data of the packet
    ESP_LOGD(TAG, "[SX1262] Data:\t\t");
    for (size_t i = 0; i < numBytes - 1; i++) {
      ESP_LOGD(TAG, "%u", byteArr[i]);
    }

    // print RSSI (Received Signal Strength Indicator)
    ESP_LOGD(TAG, "[SX1262] RSSI:\t\t %f dBm", this->radio.getRSSI());

    // print SNR (Signal-to-Noise Ratio)
    ESP_LOGD(TAG, "[SX1262] SNR:\t\t %f dB", this->radio.getSNR());

    // print frequency error
    ESP_LOGD(TAG, "[SX1262] Frequency error:\t %f Hz", this->radio.getFrequencyError());
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    ESP_LOGD(TAG, "CRC error!");
  } else {
    ESP_LOGD(TAG, "Failed %s", GetCodeDescription(state));
  }
}  // namespace sx1262
}  // namespace esphome
