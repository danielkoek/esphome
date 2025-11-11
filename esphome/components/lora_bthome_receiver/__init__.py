import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sx126x, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_SENSORS,
    CONF_BINARY_SENSORS,
)

CODEOWNERS = ["@danielkoek"]
DEPENDENCIES = ["sx126x"]
AUTO_LOAD = ["text_sensor", "binary_sensor", "sensor"]

lora_bthome_receiver_ns = cg.esphome_ns.namespace("lora_bthome_receiver")
LoRaBTHomeReceiver = lora_bthome_receiver_ns.class_(
    "LoRaBTHomeReceiver",
    cg.Component,
    cg.Parented.template(sx126x.SX126x),
)

CONF_SX126X_ID = "sx126x_id"
CONF_DEVICES_LIST = "devices_list"
CONF_DEVICES = "devices"
CONF_MAC_SUFFIX = "mac_suffix"
CONF_MEASUREMENT = "measurement"

# Device sensor configuration
DEVICE_SENSOR_SCHEMA = sensor.sensor_schema().extend(
    {
        cv.Required(CONF_MAC_SUFFIX): cv.string,
        cv.Required(CONF_MEASUREMENT): cv.string,
    }
)

# Device binary sensor configuration
DEVICE_BINARY_SENSOR_SCHEMA = binary_sensor.binary_sensor_schema().extend(
    {
        cv.Required(CONF_MAC_SUFFIX): cv.string,
        cv.Required(CONF_MEASUREMENT): cv.string,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LoRaBTHomeReceiver),
        cv.GenerateID(CONF_SX126X_ID): cv.use_id(sx126x.SX126x),
        cv.Optional(CONF_DEVICES_LIST): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_SENSORS): cv.ensure_list(DEVICE_SENSOR_SCHEMA),
        cv.Optional(CONF_BINARY_SENSORS): cv.ensure_list(DEVICE_BINARY_SENSOR_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Set parent SX126x
    parent = await cg.get_variable(config[CONF_SX126X_ID])
    cg.add(
        cg.RawExpression(
            f"{var}->Parented<esphome::sx126x::SX126x>::set_parent({parent})"
        )
    )

    # Configure devices list text sensor
    if CONF_DEVICES_LIST in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DEVICES_LIST])
        cg.add(var.set_devices_list_sensor(sens))

    # Register device sensors
    if CONF_SENSORS in config:
        for sens_config in config[CONF_SENSORS]:
            sens = await sensor.new_sensor(sens_config)
            cg.add(
                var.register_device_sensor(
                    sens_config[CONF_MAC_SUFFIX], sens_config[CONF_MEASUREMENT], sens
                )
            )

    # Register device binary sensors
    if CONF_BINARY_SENSORS in config:
        for sens_config in config[CONF_BINARY_SENSORS]:
            sens = await binary_sensor.new_binary_sensor(sens_config)
            cg.add(
                var.register_device_binary_sensor(
                    sens_config[CONF_MAC_SUFFIX], sens_config[CONF_MEASUREMENT], sens
                )
            )
