idf_component_register(
    SRCS "adc_init.c" "i2c_init.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc
)