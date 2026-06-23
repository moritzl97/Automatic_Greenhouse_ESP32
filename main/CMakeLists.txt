idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        actuators
        common
        connectivity
        hw_init
        sensors
        ui
        nvs_flash
)