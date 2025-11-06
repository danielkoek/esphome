import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sx126x
from esphome.const import CONF_ID

CODEOWNERS = ["@danielkoek"]
DEPENDENCIES = ["sx126x"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

lora_bthome_receiver_ns = cg.esphome_ns.namespace("lora_bthome_receiver")
LoRaBTHomeReceiver = lora_bthome_receiver_ns.class_(
    "LoRaBTHomeReceiver",
    cg.Component,
    cg.Parented.template(sx126x.SX126x),
)

CONF_SX126X_ID = "sx126x_id"
CONF_UPDATE_SENSORS_ON_RECEIVE = "update_sensors_on_receive"
CONF_CREATE_RSSI_SENSORS = "create_rssi_sensors"
CONF_CREATE_SNR_SENSORS = "create_snr_sensors"
CONF_SENSOR_EXPIRE_TIME = "sensor_expire_time"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LoRaBTHomeReceiver),
        cv.GenerateID(CONF_SX126X_ID): cv.use_id(sx126x.SX126x),
        cv.Optional(CONF_UPDATE_SENSORS_ON_RECEIVE, default=True): cv.boolean,
        cv.Optional(CONF_CREATE_RSSI_SENSORS, default=True): cv.boolean,
        cv.Optional(CONF_CREATE_SNR_SENSORS, default=True): cv.boolean,
        cv.Optional(
            CONF_SENSOR_EXPIRE_TIME, default="10min"
        ): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Set parent SX126x
    parent = await cg.get_variable(config[CONF_SX126X_ID])
    cg.add(var.set_parent(parent))

    # Configure parameters
    cg.add(var.set_update_sensors_on_receive(config[CONF_UPDATE_SENSORS_ON_RECEIVE]))
    cg.add(var.set_create_rssi_sensors(config[CONF_CREATE_RSSI_SENSORS]))
    cg.add(var.set_create_snr_sensors(config[CONF_CREATE_SNR_SENSORS]))
    cg.add(var.set_sensor_expire_time(config[CONF_SENSOR_EXPIRE_TIME]))
