idf_component_register(
    SRCS "light_sensor.c" "soil_moisture.c" "temp_hum_sensor.c"
    INCLUDE_DIRS "."
    REQUIRES hw_init common driver esp_adc espressif__aht20
)