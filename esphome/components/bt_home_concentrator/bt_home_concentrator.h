#pragma once

// BTHome Concentrator Component
//
// This component collects BTHome BLE advertisements from multiple devices
// and transmits them via LoRa. It can also include local sensor data.
//
// Usage example for adding local sensor data:
//
//   // In your sensor's publish_state or update method:
//   auto concentrator = App.get_bt_home_concentrator();
//   if (concentrator != nullptr) {
//     // Temperature in Celsius (0x02 = temperature sint16, 0.01 factor)
//     concentrator->add_measurement_float(0x02, temperature_value, 0.01);
//
//     // Humidity in % (0x03 = humidity uint16, 0.01 factor)
//     concentrator->add_measurement_float(0x03, humidity_value, 0.01);
//
//     // Battery in % (0x01 = battery uint8)
//     concentrator->add_measurement_uint8(0x01, battery_percent);
//   }
//
// BTHome object IDs reference:
//   0x01 - Battery (%)
//   0x02 - Temperature (°C, sint16, 0.01 factor)
//   0x03 - Humidity (%, uint16, 0.01 factor)
//   0x04 - Pressure (hPa, uint24, 0.01 factor)
//   0x05 - Illuminance (lux, uint24, 0.01 factor)
//   0x0C - Voltage (V, uint16, 0.001 factor)
//   ... (see BTHome spec for complete list)

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sx126x/sx126x.h"
#include <map>
#include <vector>

#ifdef USE_ESP32

namespace esphome {
namespace bt_home_concentrator {

// BTHome V2 UUID
static const uint16_t BTHOME_UUID = 0xFCD2;

// BTHome device info flags
struct BTHomeDeviceInfo {
  bool encrypted;
  bool trigger_based;
  uint8_t version;
};

// Represents a parsed BTHome measurement
struct BTHomeMeasurement {
  uint8_t object_id;
  std::vector<uint8_t> data;
};

// Represents a complete BTHome device state
struct BTHomeDeviceState {
  uint64_t mac_address;
  uint8_t packet_id;
  BTHomeDeviceInfo device_info;
  std::vector<BTHomeMeasurement> measurements;
  uint32_t last_seen;

  bool operator==(const BTHomeDeviceState &other) const {
    if (measurements.size() != other.measurements.size())
      return false;
    for (size_t i = 0; i < measurements.size(); i++) {
      if (measurements[i].object_id != other.measurements[i].object_id)
        return false;
      if (measurements[i].data != other.measurements[i].data)
        return false;
    }
    return true;
  }

  bool operator!=(const BTHomeDeviceState &other) const { return !(*this == other); }
};

class BTHomeConcentrator : public PollingComponent,
                           public esp32_ble_tracker::ESPBTDeviceListener,
                           public Parented<sx126x::SX126x> {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  float get_setup_priority() const override { return setup_priority::DATA; }

  // Configuration
  void set_max_devices(uint8_t max_devices) { this->max_devices_ = max_devices; }
  void set_transmit_interval(uint32_t interval_ms) { this->transmit_interval_ms_ = interval_ms; }
  void set_state_timeout(uint32_t timeout_ms) { this->state_timeout_ms_ = timeout_ms; }
  void set_duty_cycle_percent(float percent) { this->duty_cycle_percent_ = percent; }

  // Add local sensor measurements
  void add_measurement(uint8_t object_id, const std::vector<uint8_t> &data);
  void add_measurement_uint8(uint8_t object_id, uint8_t value);
  void add_measurement_sint8(uint8_t object_id, int8_t value);
  void add_measurement_uint16(uint8_t object_id, uint16_t value);
  void add_measurement_sint16(uint8_t object_id, int16_t value);
  void add_measurement_uint24(uint8_t object_id, uint32_t value);
  void add_measurement_uint32(uint8_t object_id, uint32_t value);
  void add_measurement_float(uint8_t object_id, float value, float factor);
  void clear_local_measurements();

 protected:
  // Parse BTHome service data
  optional<BTHomeDeviceState> parse_bthome_data_(const std::vector<uint8_t> &service_data, uint64_t mac_address);

  // Encode concentrated BTHome data for LoRa transmission
  std::vector<uint8_t> encode_lora_packet_();

  // Transmit the concentrated data
  void transmit_data_();

  // Check if we should transmit based on duty cycle
  bool should_transmit_();

  // Cleanup old device states
  void cleanup_old_states_();

  // Device states indexed by MAC address
  std::map<uint64_t, BTHomeDeviceState> device_states_;

  // Transmission tracking for duty cycle management
  uint32_t last_transmit_time_{0};
  uint32_t transmit_duration_ms_{0};
  uint32_t duty_cycle_window_ms_{60000};  // 1 minute window

  // Configuration
  uint8_t max_devices_{32};
  uint32_t transmit_interval_ms_{30000};  // 30 seconds default
  uint32_t state_timeout_ms_{300000};     // 5 minutes default
  float duty_cycle_percent_{1.0};         // 1% duty cycle

  bool has_new_data_{false};

  // Local sensor measurements (from this device)
  std::vector<BTHomeMeasurement> local_measurements_;
  uint64_t local_mac_address_{0};
};

}  // namespace bt_home_concentrator
}  // namespace esphome

#endif  // USE_ESP32
