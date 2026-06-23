idf_component_register(
    SRCS "grow_light.c" "pump.c"
    INCLUDE_DIRS "."
    REQUIRES hw_init common driver ui
)