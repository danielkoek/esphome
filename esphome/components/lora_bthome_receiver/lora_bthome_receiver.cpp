#include "lora_bthome_receiver.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace lora_bthome_receiver {

static const char *const TAG = "lora_bthome_receiver";

// BTHome object ID lookup table
static const std::map<uint8_t, BTHomeObjectInfo> BTHOME_OBJECT_MAP = {
    // Sensors
    {0x01, {"battery", "%", "battery", 1, 1.0f, false, false, "mdi:battery"}},
    {0x02, {"temperature", "°C", "temperature", 2, 0.01f, true, false, "mdi:thermometer"}},
    {0x03, {"humidity", "%", "humidity", 2, 0.01f, false, false, "mdi:water-percent"}},
    {0x04, {"pressure", "hPa", "pressure", 3, 0.01f, false, false, "mdi:gauge"}},
    {0x05, {"illuminance", "lx", "illuminance", 3, 0.01f, false, false, "mdi:brightness-5"}},
    {0x06, {"mass", "kg", "", 2, 0.01f, false, false, "mdi:weight-kilogram"}},
    {0x07, {"mass", "lb", "", 2, 0.01f, false, false, "mdi:weight-pound"}},
    {0x08, {"dewpoint", "°C", "temperature", 2, 0.01f, true, false, "mdi:thermometer"}},
    {0x09, {"count", "", "", 1, 1.0f, false, false, "mdi:counter"}},
    {0x0A, {"energy", "kWh", "energy", 3, 0.001f, false, false, "mdi:lightning-bolt"}},
    {0x0B, {"power", "W", "power", 3, 0.01f, false, false, "mdi:flash"}},
    {0x0C, {"voltage", "V", "voltage", 2, 0.001f, false, false, "mdi:flash"}},
    {0x0D, {"pm25", "µg/m³", "pm25", 2, 1.0f, false, false, "mdi:air-filter"}},
    {0x0E, {"pm10", "µg/m³", "pm10", 2, 1.0f, false, false, "mdi:air-filter"}},
    {0x12, {"co2", "ppm", "carbon_dioxide", 2, 1.0f, false, false, "mdi:molecule-co2"}},
    {0x13, {"tvoc", "µg/m³", "volatile_organic_compounds", 2, 1.0f, false, false, "mdi:air-filter"}},
    {0x14, {"moisture", "%", "moisture", 2, 0.01f, false, false, "mdi:water-percent"}},
    {0x2E, {"humidity", "%", "humidity", 1, 1.0f, false, false, "mdi:water-percent"}},
    {0x2F, {"moisture", "%", "moisture", 1, 1.0f, false, false, "mdi:water-percent"}},
    {0x3D, {"count", "", "", 2, 1.0f, false, false, "mdi:counter"}},
    {0x3E, {"count", "", "", 4, 1.0f, false, false, "mdi:counter"}},
    {0x40, {"distance", "mm", "distance", 2, 1.0f, false, false, "mdi:arrow-expand-horizontal"}},
    {0x41, {"distance", "m", "distance", 2, 0.1f, false, false, "mdi:arrow-expand-horizontal"}},
    {0x45, {"temperature", "°C", "temperature", 2, 0.1f, true, false, "mdi:thermometer"}},
    {0x46, {"uv_index", "", "", 1, 0.1f, false, false, "mdi:weather-sunny"}},
    {0x4A, {"voltage", "V", "voltage", 2, 0.1f, false, false, "mdi:flash"}},
    {0x51, {"acceleration", "m/s²", "", 2, 0.001f, false, false, "mdi:axis-arrow"}},
    {0x52, {"gyroscope", "°/s", "", 2, 0.001f, false, false, "mdi:axis-arrow"}},
    {0x56, {"conductivity", "µS/cm", "", 2, 1.0f, false, false, "mdi:water-outline"}},
    {0x57, {"temperature", "°C", "temperature", 1, 1.0f, true, false, "mdi:thermometer"}},
    {0x5D, {"current", "A", "current", 2, 0.001f, true, false, "mdi:current-ac"}},
    {0x61, {"rotation", "rpm", "", 2, 1.0f, false, false, "mdi:rotate-right"}},

    // Binary Sensors
    {0x0F, {"generic_boolean", "", "", 1, 1.0f, false, true, "mdi:checkbox-marked-circle"}},
    {0x10, {"power", "", "power", 1, 1.0f, false, true, "mdi:power"}},
    {0x11, {"opening", "", "opening", 1, 1.0f, false, true, "mdi:door-open"}},
    {0x15, {"battery_low", "", "battery", 1, 1.0f, false, true, "mdi:battery-alert"}},
    {0x16, {"battery_charging", "", "battery_charging", 1, 1.0f, false, true, "mdi:battery-charging"}},
    {0x17, {"carbon_monoxide", "", "carbon_monoxide", 1, 1.0f, false, true, "mdi:molecule-co"}},
    {0x18, {"cold", "", "cold", 1, 1.0f, false, true, "mdi:snowflake"}},
    {0x19, {"connectivity", "", "connectivity", 1, 1.0f, false, true, "mdi:connection"}},
    {0x1A, {"door", "", "door", 1, 1.0f, false, true, "mdi:door"}},
    {0x1B, {"garage_door", "", "garage_door", 1, 1.0f, false, true, "mdi:garage"}},
    {0x1C, {"gas", "", "gas", 1, 1.0f, false, true, "mdi:gas-cylinder"}},
    {0x1D, {"heat", "", "heat", 1, 1.0f, false, true, "mdi:fire"}},
    {0x1E, {"light", "", "light", 1, 1.0f, false, true, "mdi:lightbulb"}},
    {0x1F, {"lock", "", "lock", 1, 1.0f, false, true, "mdi:lock"}},
    {0x20, {"moisture", "", "moisture", 1, 1.0f, false, true, "mdi:water"}},
    {0x21, {"motion", "", "motion", 1, 1.0f, false, true, "mdi:motion-sensor"}},
    {0x22, {"moving", "", "moving", 1, 1.0f, false, true, "mdi:axis-arrow"}},
    {0x23, {"occupancy", "", "occupancy", 1, 1.0f, false, true, "mdi:home-account"}},
    {0x24, {"plug", "", "plug", 1, 1.0f, false, true, "mdi:power-plug"}},
    {0x25, {"presence", "", "presence", 1, 1.0f, false, true, "mdi:home"}},
    {0x26, {"problem", "", "problem", 1, 1.0f, false, true, "mdi:alert-circle"}},
    {0x27, {"running", "", "running", 1, 1.0f, false, true, "mdi:run"}},
    {0x28, {"safety", "", "safety", 1, 1.0f, false, true, "mdi:shield-check"}},
    {0x29, {"smoke", "", "smoke", 1, 1.0f, false, true, "mdi:smoke-detector"}},
    {0x2A, {"sound", "", "sound", 1, 1.0f, false, true, "mdi:microphone"}},
    {0x2B, {"tamper", "", "tamper", 1, 1.0f, false, true, "mdi:lock-alert"}},
    {0x2C, {"vibration", "", "vibration", 1, 1.0f, false, true, "mdi:vibrate"}},
    {0x2D, {"window", "", "window", 1, 1.0f, false, true, "mdi:window-open"}},
};

void LoRaBTHomeReceiver::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LoRa BTHome Receiver...");
  this->parent_->register_listener(this);
}

void LoRaBTHomeReceiver::loop() {
  // Cleanup expired sensors periodically
  static uint32_t last_cleanup = 0;
  uint32_t now = millis();
  if (now - last_cleanup > 60000) {  // Every minute
    this->cleanup_expired_sensors_();
    last_cleanup = now;
  }
}

void LoRaBTHomeReceiver::dump_config() {
  ESP_LOGCONFIG(TAG, "LoRa BTHome Receiver:");
  ESP_LOGCONFIG(TAG, "  Update Sensors on Receive: %s", YESNO(this->update_sensors_on_receive_));
  ESP_LOGCONFIG(TAG, "  Create RSSI Sensors: %s", YESNO(this->create_rssi_sensors_));
  ESP_LOGCONFIG(TAG, "  Create SNR Sensors: %s", YESNO(this->create_snr_sensors_));
  ESP_LOGCONFIG(TAG, "  Sensor Expire Time: %u ms", this->sensor_expire_time_ms_);
  ESP_LOGCONFIG(TAG, "Statistics:");
  ESP_LOGCONFIG(TAG, "  Packets Received: %u", this->packets_received_);
  ESP_LOGCONFIG(TAG, "  Packets Decoded: %u", this->packets_decoded_);
  ESP_LOGCONFIG(TAG, "  Packets Failed: %u", this->packets_failed_);
}

void LoRaBTHomeReceiver::on_packet(const std::vector<uint8_t> &packet, float rssi, float snr) {
  this->packets_received_++;

  ESP_LOGD(TAG, "Received LoRa packet: %u bytes, RSSI: %.1f dBm, SNR: %.1f dB", packet.size(), rssi, snr);

  if (this->decode_packet_(packet, rssi, snr)) {
    this->packets_decoded_++;
    ESP_LOGI(TAG, "Successfully decoded BTHome concentrator packet");
  } else {
    this->packets_failed_++;
    ESP_LOGW(TAG, "Failed to decode packet");
  }
}

bool LoRaBTHomeReceiver::decode_packet_(const std::vector<uint8_t> &packet, float rssi, float snr) {
  if (packet.size() < 3) {
    ESP_LOGW(TAG, "Packet too short: %u bytes", packet.size());
    return false;
  }

  // Check magic bytes
  if (packet[0] != MAGIC_BYTE_1 || packet[1] != MAGIC_BYTE_2) {
    ESP_LOGV(TAG, "Invalid magic bytes: 0x%02X 0x%02X", packet[0], packet[1]);
    return false;
  }

  uint8_t device_count = packet[2];
  ESP_LOGD(TAG, "Decoding packet with %u device(s)", device_count);

  size_t pos = 3;

  for (uint8_t dev_idx = 0; dev_idx < device_count; dev_idx++) {
    if (pos + 8 > packet.size()) {
      ESP_LOGW(TAG, "Incomplete device data at position %u", pos);
      return false;
    }

    // Extract MAC address (6 bytes, little endian)
    uint64_t mac = 0;
    for (int i = 0; i < 6; i++) {
      mac |= ((uint64_t) packet[pos++]) << (i * 8);
    }

    std::string mac_str = this->mac_to_string_(mac);

    // Packet ID
    uint8_t packet_id = packet[pos++];

    // Measurement count
    uint8_t measurement_count = packet[pos++];

    ESP_LOGD(TAG, "Device %u: MAC=%s, Packet ID=%u, Measurements=%u", dev_idx + 1, mac_str.c_str(), packet_id,
             measurement_count);

    // Update or create device
    BTHomeDevice &device = this->devices_[mac];
    device.mac_address = mac;
    device.mac_str = mac_str;
    device.packet_id = packet_id;
    device.last_seen = millis();
    device.measurements.clear();

    // Parse measurements
    for (uint8_t meas_idx = 0; meas_idx < measurement_count; meas_idx++) {
      if (pos + 2 > packet.size()) {
        ESP_LOGW(TAG, "Incomplete measurement at position %u", pos);
        return false;
      }

      uint8_t object_id = packet[pos++];
      uint8_t data_length = packet[pos++];

      if (pos + data_length > packet.size()) {
        ESP_LOGW(TAG, "Insufficient data for measurement at position %u", pos);
        return false;
      }

      std::vector<uint8_t> data(packet.begin() + pos, packet.begin() + pos + data_length);
      pos += data_length;

      // Get object info
      const BTHomeObjectInfo *info = this->get_object_info_(object_id);

      BTHomeMeasurement measurement;
      measurement.object_id = object_id;
      measurement.data = data;

      if (info != nullptr) {
        measurement.name = info->name;
        measurement.unit = info->unit;
        measurement.value = this->parse_measurement_value_(object_id, data);

        ESP_LOGD(TAG, "  [%u] %s: %.2f %s (0x%02X)", meas_idx + 1, measurement.name.c_str(), measurement.value,
                 measurement.unit.c_str(), object_id);

        // Update or create sensor
        if (this->update_sensors_on_receive_) {
          if (info->is_binary) {
            auto *bin_sensor = this->get_binary_sensor(mac, object_id, measurement.name);
            if (bin_sensor != nullptr) {
              bin_sensor->publish_state(measurement.value > 0.5f);
            }
          } else {
            auto *sensor = this->get_sensor(mac, object_id, measurement.name);
            if (sensor != nullptr) {
              sensor->publish_state(measurement.value);
            }
          }
        }
      } else {
        ESP_LOGV(TAG, "  [%u] Unknown object 0x%02X", meas_idx + 1, object_id);
      }

      device.measurements.push_back(measurement);
    }

    // Update RSSI/SNR sensors if enabled
    if (this->create_rssi_sensors_) {
      auto *rssi_sensor = this->get_rssi_sensor(mac);
      if (rssi_sensor != nullptr) {
        rssi_sensor->publish_state(rssi);
      }
    }

    if (this->create_snr_sensors_) {
      auto *snr_sensor = this->get_snr_sensor(mac);
      if (snr_sensor != nullptr) {
        snr_sensor->publish_state(snr);
      }
    }
  }

  return true;
}

float LoRaBTHomeReceiver::parse_measurement_value_(uint8_t object_id, const std::vector<uint8_t> &data) {
  const BTHomeObjectInfo *info = this->get_object_info_(object_id);
  if (info == nullptr || data.empty()) {
    return 0.0f;
  }

  int32_t raw_value = 0;

  // Parse based on data length
  if (data.size() == 1) {
    if (info->is_signed) {
      raw_value = (int8_t) data[0];
    } else {
      raw_value = data[0];
    }
  } else if (data.size() == 2) {
    if (info->is_signed) {
      raw_value = (int16_t) (data[0] | (data[1] << 8));
    } else {
      raw_value = (uint16_t) (data[0] | (data[1] << 8));
    }
  } else if (data.size() == 3) {
    // 24-bit unsigned
    raw_value = data[0] | (data[1] << 8) | (data[2] << 16);
  } else if (data.size() == 4) {
    if (info->is_signed) {
      raw_value = (int32_t) (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    } else {
      raw_value = (uint32_t) (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    }
  }

  return raw_value * info->factor;
}

const BTHomeObjectInfo *LoRaBTHomeReceiver::get_object_info_(uint8_t object_id) {
  auto it = BTHOME_OBJECT_MAP.find(object_id);
  if (it != BTHOME_OBJECT_MAP.end()) {
    return &it->second;
  }
  return nullptr;
}

std::string LoRaBTHomeReceiver::generate_entity_id_(uint64_t mac, const std::string &measurement_name) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%012llx_%s", mac, measurement_name.c_str());
  return std::string(buffer);
}

std::string LoRaBTHomeReceiver::mac_to_string_(uint64_t mac) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", (uint8_t) ((mac >> 40) & 0xFF),
           (uint8_t) ((mac >> 32) & 0xFF), (uint8_t) ((mac >> 24) & 0xFF), (uint8_t) ((mac >> 16) & 0xFF),
           (uint8_t) ((mac >> 8) & 0xFF), (uint8_t) (mac & 0xFF));
  return std::string(buffer);
}

sensor::Sensor *LoRaBTHomeReceiver::get_sensor(uint64_t mac, uint8_t object_id, const std::string &name) {
  std::string entity_id = this->generate_entity_id_(mac, name);

  auto it = this->sensors_.find(entity_id);
  if (it != this->sensors_.end()) {
    return it->second;
  }

  // Create new sensor
  ESP_LOGI(TAG, "Creating sensor: %s", entity_id.c_str());

  auto *sensor = new sensor::Sensor();
  // Note: set_name expects const char* that persists - we use empty string and rely on object_id
  sensor->set_object_id(strdup(entity_id.c_str()));

  const BTHomeObjectInfo *info = this->get_object_info_(object_id);
  if (info != nullptr) {
    sensor->set_unit_of_measurement(info->unit);
    sensor->set_device_class(info->device_class);
    sensor->set_icon(info->icon);
  }

  App.register_sensor(sensor);
  this->sensors_[entity_id] = sensor;

  return sensor;
}
binary_sensor::BinarySensor *LoRaBTHomeReceiver::get_binary_sensor(uint64_t mac, uint8_t object_id,
                                                                   const std::string &name) {
  std::string entity_id = this->generate_entity_id_(mac, name);

  auto it = this->binary_sensors_.find(entity_id);
  if (it != this->binary_sensors_.end()) {
    return it->second;
  }

  // Create new binary sensor
  ESP_LOGI(TAG, "Creating binary sensor: %s", entity_id.c_str());

  auto *sensor = new binary_sensor::BinarySensor();
  // Note: set_name expects const char* that persists - we use empty string and rely on object_id
  sensor->set_object_id(strdup(entity_id.c_str()));

  const BTHomeObjectInfo *info = this->get_object_info_(object_id);
  if (info != nullptr) {
    sensor->set_device_class(info->device_class);
    sensor->set_icon(info->icon);
  }

  App.register_binary_sensor(sensor);
  this->binary_sensors_[entity_id] = sensor;

  return sensor;
}
text_sensor::TextSensor *LoRaBTHomeReceiver::get_text_sensor(uint64_t mac, const std::string &name) {
  std::string entity_id = this->generate_entity_id_(mac, name);

  auto it = this->text_sensors_.find(entity_id);
  if (it != this->text_sensors_.end()) {
    return it->second;
  }

  // Create new text sensor
  ESP_LOGI(TAG, "Creating text sensor: %s", entity_id.c_str());

  auto *sensor = new text_sensor::TextSensor();
  // Note: set_name expects const char* that persists - we use empty string and rely on object_id
  sensor->set_object_id(strdup(entity_id.c_str()));

  App.register_text_sensor(sensor);
  this->text_sensors_[entity_id] = sensor;

  return sensor;
}

sensor::Sensor *LoRaBTHomeReceiver::get_rssi_sensor(uint64_t mac) {
  auto it = this->rssi_sensors_.find(mac);
  if (it != this->rssi_sensors_.end()) {
    return it->second;
  }

  // Create new RSSI sensor
  std::string entity_id = str_sprintf("%012llx_rssi", mac);
  ESP_LOGI(TAG, "Creating RSSI sensor: %s", entity_id.c_str());

  auto *sensor = new sensor::Sensor();
  sensor->set_object_id(strdup(entity_id.c_str()));
  sensor->set_unit_of_measurement("dBm");
  sensor->set_device_class("signal_strength");
  sensor->set_icon("mdi:wifi");

  App.register_sensor(sensor);
  this->rssi_sensors_[mac] = sensor;

  return sensor;
}
sensor::Sensor *LoRaBTHomeReceiver::get_snr_sensor(uint64_t mac) {
  auto it = this->snr_sensors_.find(mac);
  if (it != this->snr_sensors_.end()) {
    return it->second;
  }

  // Create new SNR sensor
  ESP_LOGI(TAG, "Creating SNR sensor for MAC %s", this->mac_to_string_(mac).c_str());

  // Create new SNR sensor
  std::string entity_id = str_sprintf("%012llx_snr", mac);
  ESP_LOGI(TAG, "Creating SNR sensor: %s", entity_id.c_str());

  auto *sensor = new sensor::Sensor();
  sensor->set_object_id(strdup(entity_id.c_str()));
  sensor->set_unit_of_measurement("dB");
  sensor->set_icon("mdi:signal");

  App.register_sensor(sensor);
  this->snr_sensors_[mac] = sensor;

  return sensor;
}

void LoRaBTHomeReceiver::cleanup_expired_sensors_() {
  uint32_t now = millis();

  // Remove expired devices
  auto dev_it = this->devices_.begin();
  while (dev_it != this->devices_.end()) {
    if (now - dev_it->second.last_seen > this->sensor_expire_time_ms_) {
      ESP_LOGD(TAG, "Removing expired device: %s", dev_it->second.mac_str.c_str());
      dev_it = this->devices_.erase(dev_it);
    } else {
      ++dev_it;
    }
  }
}

}  // namespace lora_bthome_receiver
}  // namespace esphome
