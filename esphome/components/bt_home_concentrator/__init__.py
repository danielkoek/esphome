import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker, sx126x
from esphome.const import CONF_ID

CODEOWNERS = ["@danielkoek"]
DEPENDENCIES = ["esp32_ble_tracker", "sx126x"]
AUTO_LOAD = []

bt_home_concentrator_ns = cg.esphome_ns.namespace("bt_home_concentrator")
BTHomeConcentrator = bt_home_concentrator_ns.class_(
    "BTHomeConcentrator",
    cg.PollingComponent,
    esp32_ble_tracker.ESPBTDeviceListener,
    cg.Parented.template(sx126x.SX126x),
)

CONF_SX126X_ID = "sx126x_id"
CONF_MAX_DEVICES = "max_devices"
CONF_TRANSMIT_INTERVAL = "transmit_interval"
CONF_STATE_TIMEOUT = "state_timeout"
CONF_DUTY_CYCLE = "duty_cycle_percent"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BTHomeConcentrator),
            cv.GenerateID(CONF_SX126X_ID): cv.use_id(sx126x.SX126x),
            cv.Optional(CONF_MAX_DEVICES, default=32): cv.int_range(min=1, max=255),
            cv.Optional(
                CONF_TRANSMIT_INTERVAL, default="30s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_STATE_TIMEOUT, default="5min"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_DUTY_CYCLE, default=1.0): cv.percentage,
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("30s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    # Set parent SX126x
    parent = await cg.get_variable(config[CONF_SX126X_ID])
    cg.add(var.set_parent(parent))

    # Configure parameters
    cg.add(var.set_max_devices(config[CONF_MAX_DEVICES]))
    cg.add(var.set_transmit_interval(config[CONF_TRANSMIT_INTERVAL]))
    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT]))
    cg.add(var.set_duty_cycle_percent(config[CONF_DUTY_CYCLE]))
