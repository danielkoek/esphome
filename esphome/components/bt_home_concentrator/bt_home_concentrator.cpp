#include "bt_home_concentrator.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace bt_home_concentrator {

static const char *const TAG = "bt_home_concentrator";

void BTHomeConcentrator::setup() { ESP_LOGCONFIG(TAG, "Setting up BTHome Concentrator..."); }

void BTHomeConcentrator::loop() {
  // Cleanup old states periodically
  uint32_t now = millis();
  static uint32_t last_cleanup = 0;
  if (now - last_cleanup > 60000) {  // Every minute
    this->cleanup_old_states_();
    last_cleanup = now;
  }
}

void BTHomeConcentrator::update() {
  // Called at transmit_interval, attempt to transmit concentrated data
  if (this->has_new_data_ && this->should_transmit_()) {
    this->transmit_data_();
  }
}

void BTHomeConcentrator::dump_config() {
  ESP_LOGCONFIG(TAG, "BTHome Concentrator:");
  ESP_LOGCONFIG(TAG, "  Max Devices: %u", this->max_devices_);
  ESP_LOGCONFIG(TAG, "  Transmit Interval: %u ms", this->transmit_interval_ms_);
  ESP_LOGCONFIG(TAG, "  State Timeout: %u ms", this->state_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Duty Cycle: %.2f%%", this->duty_cycle_percent_);
  LOG_UPDATE_INTERVAL(this);
}

bool BTHomeConcentrator::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  const auto &service_datas = device.get_service_datas();

  for (const auto &service_data : service_datas) {
    // Check if this is a BTHome service data (UUID 0xFCD2)
    if (service_data.uuid.get_uuid().uuid.uuid16 != BTHOME_UUID) {
      continue;
    }

    ESP_LOGV(TAG, "Found BTHome device: %s", device.address_str().c_str());

    // Parse the BTHome data
    auto state = this->parse_bthome_data_(service_data.data, device.address_uint64());

    if (!state.has_value()) {
      ESP_LOGW(TAG, "Failed to parse BTHome data from %s", device.address_str().c_str());
      continue;
    }

    uint64_t mac = device.address_uint64();

    // Check if we already have state for this device
    auto it = this->device_states_.find(mac);

    if (it != this->device_states_.end()) {
      // Check if the data has changed (excluding packet_id and last_seen)
      if (it->second == state.value()) {
        // Data is the same, just update timestamp
        ESP_LOGVV(TAG, "Duplicate data from %s, updating timestamp", device.address_str().c_str());
        it->second.last_seen = millis();
        continue;
      }

      // Check packet_id for deduplication
      if (state.value().packet_id == it->second.packet_id) {
        ESP_LOGVV(TAG, "Same packet_id from %s, skipping", device.address_str().c_str());
        continue;
      }
    }

    // New or changed data - update state
    ESP_LOGD(TAG, "New/updated BTHome data from %s, %u measurements", device.address_str().c_str(),
             state.value().measurements.size());

    this->device_states_[mac] = state.value();
    this->has_new_data_ = true;

    return true;
  }

  return false;
}

optional<BTHomeDeviceState> BTHomeConcentrator::parse_bthome_data_(const std::vector<uint8_t> &service_data,
                                                                   uint64_t mac_address) {
  if (service_data.size() < 1) {
    ESP_LOGW(TAG, "BTHome service data too short");
    return {};
  }

  BTHomeDeviceState state;
  state.mac_address = mac_address;
  state.last_seen = millis();

  size_t pos = 0;

  // Parse device info byte
  uint8_t device_info_byte = service_data[pos++];
  state.device_info.encrypted = (device_info_byte & 0x01) != 0;
  state.device_info.trigger_based = (device_info_byte & 0x04) != 0;
  state.device_info.version = (device_info_byte >> 5) & 0x07;

  ESP_LOGVV(TAG, "BTHome v%u, encrypted=%d, trigger_based=%d", state.device_info.version, state.device_info.encrypted,
            state.device_info.trigger_based);

  if (state.device_info.encrypted) {
    ESP_LOGW(TAG, "Encrypted BTHome data not supported");
    return {};
  }

  if (state.device_info.version != 2) {
    ESP_LOGW(TAG, "Only BTHome v2 supported, got v%u", state.device_info.version);
    return {};
  }

  // Parse measurements
  while (pos < service_data.size()) {
    uint8_t object_id = service_data[pos++];

    if (pos >= service_data.size()) {
      ESP_LOGW(TAG, "Incomplete measurement data");
      break;
    }

    BTHomeMeasurement measurement;
    measurement.object_id = object_id;

    // Determine data length based on object_id
    // This is a simplified parser - you can extend it based on the BTHome spec
    size_t data_len = 0;

    switch (object_id) {
      case 0x00:  // packet_id
        state.packet_id = service_data[pos];
        data_len = 1;
        break;
      case 0x01:  // battery (%)
      case 0x2E:  // humidity (uint8)
      case 0x2F:  // moisture (uint8)
      case 0x46:  // UV index
      case 0x57:  // temperature (sint8)
      case 0x58:  // temperature (sint8, 0.35 factor)
      case 0x59:  // count (sint8)
        data_len = 1;
        break;
      case 0x02:  // temperature (sint16)
      case 0x03:  // humidity (uint16)
      case 0x04:  // pressure (uint24) - actually 3 bytes
      case 0x05:  // illuminance (uint24) - actually 3 bytes
      case 0x06:  // mass kg (uint16)
      case 0x07:  // mass lb (uint16)
      case 0x08:  // dewpoint (sint16)
      case 0x09:  // count (uint8) - actually 1 byte
      case 0x0C:  // voltage (uint16)
      case 0x0D:  // pm2.5 (uint16)
      case 0x0E:  // pm10 (uint16)
      case 0x12:  // co2 (uint16)
      case 0x13:  // tvoc (uint16)
      case 0x14:  // moisture (uint16)
      case 0x3D:  // count (uint16)
      case 0x3F:  // rotation (sint16)
      case 0x40:  // distance mm (uint16)
      case 0x41:  // distance m (uint16)
      case 0x43:  // current (uint16)
      case 0x44:  // speed (uint16)
      case 0x45:  // temperature (sint16, 0.1 factor)
      case 0x47:  // volume (uint16)
      case 0x48:  // volume mL (uint16)
      case 0x49:  // volume flow rate (uint16)
      case 0x4A:  // voltage (uint16, 0.1 factor)
      case 0x51:  // acceleration (uint16)
      case 0x52:  // gyroscope (uint16)
      case 0x56:  // conductivity (uint16)
      case 0x5A:  // count (sint16)
      case 0x5C:  // power (sint32) - actually 4 bytes
      case 0x5D:  // current (sint16)
      case 0x5E:  // direction (uint16)
      case 0x5F:  // precipitation (uint16)
      case 0x60:  // channel (uint8) - actually 1 byte
      case 0x61:  // rotational speed (uint16)
        if (object_id == 0x09 || object_id == 0x60) {
          data_len = 1;
        } else if (object_id == 0x04 || object_id == 0x05) {
          data_len = 3;
        } else if (object_id == 0x5C) {
          data_len = 4;
        } else {
          data_len = 2;
        }
        break;
      case 0x0A:  // energy (uint24)
      case 0x0B:  // power (uint24)
      case 0x42:  // duration (uint24)
      case 0x4B:  // gas (uint24)
        data_len = 3;
        break;
      case 0x3E:  // count (uint32)
      case 0x4C:  // gas (uint32)
      case 0x4D:  // energy (uint32)
      case 0x4E:  // volume (uint32)
      case 0x4F:  // water (uint32)
      case 0x50:  // timestamp (uint32)
      case 0x55:  // volume storage (uint32)
      case 0x5B:  // count (sint32)
        data_len = 4;
        break;
      case 0x53:  // text (variable length)
      case 0x54:  // raw (variable length)
        if (pos < service_data.size()) {
          data_len = service_data[pos];
          measurement.data.push_back(service_data[pos++]);
        }
        break;
      // Binary sensors (all uint8)
      case 0x0F:  // generic boolean
      case 0x10:  // power
      case 0x11:  // opening
      case 0x15:  // battery
      case 0x16:  // battery charging
      case 0x17:  // carbon monoxide
      case 0x18:  // cold
      case 0x19:  // connectivity
      case 0x1A:  // door
      case 0x1B:  // garage door
      case 0x1C:  // gas
      case 0x1D:  // heat
      case 0x1E:  // light
      case 0x1F:  // lock
      case 0x20:  // moisture
      case 0x21:  // motion
      case 0x22:  // moving
      case 0x23:  // occupancy
      case 0x24:  // plug
      case 0x25:  // presence
      case 0x26:  // problem
      case 0x27:  // running
      case 0x28:  // safety
      case 0x29:  // smoke
      case 0x2A:  // sound
      case 0x2B:  // tamper
      case 0x2C:  // vibration
      case 0x2D:  // window
        data_len = 1;
        break;
      // Events
      case 0x3A:  // button
        data_len = 1;
        break;
      case 0x3C:  // dimmer
        data_len = 2;
        break;
      // Device info
      case 0xF0:  // device type id
        data_len = 2;
        break;
      case 0xF1:  // firmware version (uint32)
        data_len = 4;
        break;
      case 0xF2:  // firmware version (uint24)
        data_len = 3;
        break;
      default:
        ESP_LOGW(TAG, "Unknown BTHome object_id: 0x%02X", object_id);
        // Try to continue parsing
        return state;
    }

    // Extract measurement data
    if (pos + data_len > service_data.size()) {
      ESP_LOGW(TAG, "Insufficient data for object_id 0x%02X", object_id);
      break;
    }

    for (size_t i = 0; i < data_len; i++) {
      measurement.data.push_back(service_data[pos++]);
    }

    // Only add non-packet_id measurements to the state
    if (object_id != 0x00) {
      state.measurements.push_back(measurement);
    }
  }

  ESP_LOGV(TAG, "Parsed %u measurements, packet_id=%u", state.measurements.size(), state.packet_id);

  return state;
}

std::vector<uint8_t> BTHomeConcentrator::encode_lora_packet_() {
  std::vector<uint8_t> packet;

  // Packet format for LoRa transmission:
  // Header (2 bytes): 0xBF 0xC0 (BTHome Concentrator magic)
  // Device count (1 byte)
  // For each device:
  //   MAC address (6 bytes, compressed from 8)
  //   Packet ID (1 byte)
  //   Measurement count (1 byte)
  //   For each measurement:
  //     Object ID (1 byte)
  //     Data length (1 byte)
  //     Data (variable)

  packet.push_back(0xBF);
  packet.push_back(0xC0);  // Count devices (limit to max_devices)
  uint8_t device_count = std::min((size_t) this->max_devices_, this->device_states_.size());
  packet.push_back(device_count);

  uint8_t count = 0;
  for (const auto &entry : this->device_states_) {
    if (count >= device_count)
      break;

    const auto &state = entry.second;

    // Add MAC address (6 bytes, little endian)
    for (int i = 0; i < 6; i++) {
      packet.push_back((state.mac_address >> (i * 8)) & 0xFF);
    }

    // Add packet ID
    packet.push_back(state.packet_id);

    // Add measurement count
    packet.push_back(state.measurements.size());

    // Add each measurement
    for (const auto &measurement : state.measurements) {
      packet.push_back(measurement.object_id);
      packet.push_back(measurement.data.size());
      for (uint8_t byte : measurement.data) {
        packet.push_back(byte);
      }
    }

    count++;
  }

  ESP_LOGI(TAG, "Encoded LoRa packet: %u devices, %u bytes", device_count, packet.size());

  return packet;
}

void BTHomeConcentrator::transmit_data_() {
  auto packet = this->encode_lora_packet_();

  if (packet.empty()) {
    ESP_LOGW(TAG, "No data to transmit");
    return;
  }

  // Check maximum packet size
  size_t max_size = this->parent_->get_max_packet_size();
  if (packet.size() > max_size) {
    ESP_LOGW(TAG, "Packet too large (%u > %u), truncating data", packet.size(), max_size);
    // In a production implementation, you might split into multiple packets
    packet.resize(max_size);
  }

  ESP_LOGI(TAG, "Transmitting LoRa packet with %u bytes", packet.size());

  uint32_t start_time = millis();
  auto result = this->parent_->transmit_packet(packet);
  uint32_t end_time = millis();

  if (result == sx126x::SX126xError::NONE) {
    ESP_LOGI(TAG, "LoRa transmission successful");
    this->transmit_duration_ms_ = end_time - start_time;
    this->last_transmit_time_ = start_time;
    this->has_new_data_ = false;
  } else {
    ESP_LOGW(TAG, "LoRa transmission failed");
  }
}

bool BTHomeConcentrator::should_transmit_() {
  uint32_t now = millis();

  // Calculate duty cycle usage in the current window
  uint32_t window_start = now - this->duty_cycle_window_ms_;

  // If last transmit was before the window, we're clear to transmit
  if (this->last_transmit_time_ < window_start) {
    return true;
  }

  // Calculate time used in current window
  uint32_t time_used_ms = this->transmit_duration_ms_;

  // Calculate allowed time based on duty cycle
  float allowed_time_ms = (this->duty_cycle_window_ms_ * this->duty_cycle_percent_) / 100.0f;

  bool can_transmit = time_used_ms < allowed_time_ms;

  if (!can_transmit) {
    ESP_LOGD(TAG, "Duty cycle limit reached: used %.1f ms of %.1f ms allowed", time_used_ms, allowed_time_ms);
  }

  return can_transmit;
}

void BTHomeConcentrator::cleanup_old_states_() {
  uint32_t now = millis();

  auto it = this->device_states_.begin();
  while (it != this->device_states_.end()) {
    if (now - it->second.last_seen > this->state_timeout_ms_) {
      ESP_LOGD(TAG, "Removing stale device state: %012llX", it->first);
      it = this->device_states_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace bt_home_concentrator
}  // namespace esphome

#endif  // USE_ESP32
