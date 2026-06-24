idf_component_register(
    SRCS "uart_connection.c" "buttons.c" "display.c" "status_leds.c"
    INCLUDE_DIRS "."
    REQUIRES hw_init common driver esp_timer espressif__esp_lvgl_port config
)