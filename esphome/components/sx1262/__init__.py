import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome import pins
from esphome.const import CONF_ID, CONF_RESET_PIN, CONF_BUSY_PIN, CONF_CS_PIN

CODEOWNERS = ["@danielkoek"]

MULTI_CONF = True

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]
CONF_DIO1_PIN = "dio1_pin"
CONF_DIO2_PIN = "dio2_pin"
CONF_REPEATER = "repeater"
# Hack to prevent compile error due to ambiguity with lib namespace
ebyte_lora_ns = cg.esphome_ns.namespace("sx1262")
SX1262Component = ebyte_lora_ns.class_(
    "SX1262Component",
    cg.PollingComponent,
    spi.SPIDevice,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CONF_ID),
            cv.Required(CONF_REPEATER): cv.boolean,
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_DIO1_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_DIO2_PIN): pins.gpio_output_pin_schema,
        }
    ).extend(spi.spi_device_schema()),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(reset))
    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs))
    dio2 = await cg.gpio_pin_expression(config[CONF_DIO2_PIN])
    cg.add(var.set_dio2_pin(dio2))
    cg.add(var.set_repeater(config[CONF_REPEATER]))
    # RadioLib
    cg.add_library("jgromes/RadioLib", "7.1.2")
