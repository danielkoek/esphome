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
  bool is_binary;
};

// Represents a BTHome device's state
struct BTHomeDevice {
  uint64_t mac_address;
  std::string mac_str;
  std::string mac_suffix;  // Last 4 chars (XX:XX)
  uint8_t packet_id;
  uint32_t last_seen;
  float rssi;
  float snr;
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

  // Set text sensor for device list
  void set_devices_list_sensor(text_sensor::TextSensor *sensor) { this->devices_list_sensor_ = sensor; }

  // Register a sensor for a specific device and measurement
  void register_device_sensor(const std::string &mac_suffix, const std::string &measurement, sensor::Sensor *sens);

  // Register a binary sensor for a specific device and measurement
  void register_device_binary_sensor(const std::string &mac_suffix, const std::string &measurement,
                                     binary_sensor::BinarySensor *sens);

 protected:
  // Decode LoRa packet from BTHome concentrator
  bool decode_packet_(const std::vector<uint8_t> &packet, float rssi, float snr);

  // Parse BTHome measurement value
  float parse_measurement_value_(uint8_t object_id, const std::vector<uint8_t> &data);

  // Get BTHome object info
  const BTHomeObjectInfo *get_object_info_(uint8_t object_id);

  // Publish device list to text sensor
  void publish_devices_list_();

  // Publish measurements to registered sensors
  void publish_measurements_();

  // Format MAC address as string
  std::string mac_to_string_(uint64_t mac);

  // Extract MAC suffix (last 4 chars: XX:XX)
  std::string get_mac_suffix_(const std::string &mac_str);

  // Cleanup expired devices
  void cleanup_expired_devices_();

  // Device states indexed by MAC address
  std::map<uint64_t, BTHomeDevice> devices_;

  // Text sensor for publishing device list
  text_sensor::TextSensor *devices_list_sensor_{nullptr};

  // Registered sensors: key = "mac_suffix:measurement_name"
  std::map<std::string, sensor::Sensor *> registered_sensors_;
  std::map<std::string, binary_sensor::BinarySensor *> registered_binary_sensors_;

  // Statistics
  uint32_t packets_received_{0};
  uint32_t packets_decoded_{0};
  uint32_t packets_failed_{0};
};

}  // namespace lora_bthome_receiver
}  // namespace esphome
