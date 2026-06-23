idf_component_register(
    SRCS "http_server.c" "mqtt.c" "wifi.c"
    INCLUDE_DIRS "."
    REQUIRES common config esp_wifi mqtt esp_http_server
    EMBED_TXTFILES
        "certs/ca.pem"
        "certs/esp32.crt"
        "certs/esp32.key"
    EMBED_FILES
        "index.html"
)