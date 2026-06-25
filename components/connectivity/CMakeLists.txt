idf_component_register(
    SRCS "firebase_db.c" "http_server.c" "mqtt.c" "wifi.c"
    INCLUDE_DIRS "."
    REQUIRES common config esp_wifi mqtt esp_http_server esp_http_client json mbedtls esp-tls
    EMBED_TXTFILES
        "certs/ca.pem"
        "certs/esp32.crt"
        "certs/esp32.key"
    EMBED_FILES
        "index.html"
)