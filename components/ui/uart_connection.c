#include "uart_connection.h"
#include "esp_timer.h"
#include "gpio_config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "utils.h"
#include "parameter_config.h"

void uart_config_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 256, 0, 20, NULL, 0));
    
    uint8_t data[512];
    int json_start = -1, json_len = 0;
    
    while (1) {
        int len = uart_read_bytes(UART_NUM_1, data, sizeof(data)-1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = 0;  // Null terminate
            
            // Find JSON object boundaries
            for (int i = 0; i < len; i++) {
                if (data[i] == '{') {
                    json_start = i;
                    break;
                }
            }
            
            // Find end of JSON
            if (json_start >= 0) {
                for (int i = json_start; i < len; i++) {
                    if (data[i] == '}') {
                        json_len = i - json_start + 1;
                        parse_json_to_config((char*)data + json_start, json_len);
                        json_start = -1;
                        break;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}