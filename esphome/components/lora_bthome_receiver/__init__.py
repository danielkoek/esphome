import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sx126x, text_sensor
from esphome.const import CONF_ID

CODEOWNERS = ["@danielkoek"]
DEPENDENCIES = ["sx126x"]
AUTO_LOAD = ["text_sensor"]

lora_bthome_receiver_ns = cg.esphome_ns.namespace("lora_bthome_receiver")
LoRaBTHomeReceiver = lora_bthome_receiver_ns.class_(
    "LoRaBTHomeReceiver",
    cg.Component,
    cg.Parented.template(sx126x.SX126x),
)

CONF_SX126X_ID = "sx126x_id"
CONF_DEVICES = "devices"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LoRaBTHomeReceiver),
        cv.GenerateID(CONF_SX126X_ID): cv.use_id(sx126x.SX126x),
        cv.Optional(CONF_DEVICES): text_sensor.text_sensor_schema(
            icon="mdi:bluetooth-connect"
        ),
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

    # Configure devices text sensor if provided
    if CONF_DEVICES in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DEVICES])
        cg.add(var.set_devices_sensor(sens))
