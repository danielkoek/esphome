#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sx126x/sx126x.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <map>
#include <vector>
#include <string>

namespace esphome {
namespace lora_bthome_receiver {

// BTHome V2 UUID
static const uint16_t BTHOME_UUID = 0xFCD2;

// Magic bytes for BTHome Concentrator packets
static const uint8_t MAGIC_BYTE_1 = 0xBF;
static const uint8_t MAGIC_BYTE_2 = 0xC0;

// BTHome object ID metadata
struct BTHomeObjectInfo {
  const char *name;
  const char *unit;
  const char *device_class;
  uint8_t data_length;
  float factor;
  bool is_signed;
  bool is_binary;
  const char *icon;
};

// Represents a measurement from a BTHome device
struct BTHomeMeasurement {
  uint8_t object_id;
  std::vector<uint8_t> data;
  float value;
  std::string name;
  std::string unit;
};

// Represents a BTHome device's state
struct BTHomeDevice {
  uint64_t mac_address;
  std::string mac_str;
  uint8_t packet_id;
  uint32_t last_seen;
  std::vector<BTHomeMeasurement> measurements;
};

class LoRaBTHomeReceiver : public Component, public Parented<sx126x::SX126x>, public sx126x::SX126xListener {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

  // SX126xListener callback - called when a packet is received
  void on_packet(const std::vector<uint8_t> &packet, float rssi, float snr) override;

  // Configuration
  void set_update_sensors_on_receive(bool update) { this->update_sensors_on_receive_ = update; }
  void set_create_rssi_sensors(bool create) { this->create_rssi_sensors_ = create; }
  void set_create_snr_sensors(bool create) { this->create_snr_sensors_ = create; }
  void set_sensor_expire_time(uint32_t expire_ms) { this->sensor_expire_time_ms_ = expire_ms; }

  // Get or create sensor for a device measurement
  sensor::Sensor *get_sensor(uint64_t mac, uint8_t object_id, const std::string &name);
  binary_sensor::BinarySensor *get_binary_sensor(uint64_t mac, uint8_t object_id, const std::string &name);
  text_sensor::TextSensor *get_text_sensor(uint64_t mac, const std::string &name);
  sensor::Sensor *get_rssi_sensor(uint64_t mac);
  sensor::Sensor *get_snr_sensor(uint64_t mac);

 protected:
  // Decode LoRa packet from BTHome concentrator
  bool decode_packet_(const std::vector<uint8_t> &packet, float rssi, float snr);

  // Parse BTHome measurement value
  float parse_measurement_value_(uint8_t object_id, const std::vector<uint8_t> &data);

  // Get BTHome object info
  const BTHomeObjectInfo *get_object_info_(uint8_t object_id);

  // Generate sensor entity ID
  std::string generate_entity_id_(uint64_t mac, const std::string &measurement_name);

  // Cleanup expired sensors
  void cleanup_expired_sensors_();

  // Format MAC address as string
  std::string mac_to_string_(uint64_t mac);

  // Device states indexed by MAC address
  std::map<uint64_t, BTHomeDevice> devices_;

  // Dynamically created sensors
  std::map<std::string, sensor::Sensor *> sensors_;
  std::map<std::string, binary_sensor::BinarySensor *> binary_sensors_;
  std::map<std::string, text_sensor::TextSensor *> text_sensors_;
  std::map<uint64_t, sensor::Sensor *> rssi_sensors_;
  std::map<uint64_t, sensor::Sensor *> snr_sensors_;

  // Configuration
  bool update_sensors_on_receive_{true};
  bool create_rssi_sensors_{true};
  bool create_snr_sensors_{true};
  uint32_t sensor_expire_time_ms_{600000};  // 10 minutes default

  // Statistics
  uint32_t packets_received_{0};
  uint32_t packets_decoded_{0};
  uint32_t packets_failed_{0};
};

}  // namespace lora_bthome_receiver
}  // namespace esphome
