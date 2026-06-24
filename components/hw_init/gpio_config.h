#pragma once

// --- I2C Bus (AHT20) ---
#define I2C_SDA_PIN               GPIO_NUM_21
#define I2C_SCL_PIN               GPIO_NUM_22
#define I2C_BUS_PORT              0

// --- Grow Light ---
#define GROW_LIGHT_GPIO           GPIO_NUM_14

// --- Soil Moisture Sensors (ADC1 only -- ADC2 unusable with WiFi) ---
#define SOIL_SENSOR_1_PIN         GPIO_NUM_36
#define SOIL_SENSOR_2_PIN         GPIO_NUM_39
#define SOIL_SENSOR_3_PIN         GPIO_NUM_34
#define SOIL_SENSOR_4_PIN         GPIO_NUM_35
#define SOIL_SENSOR_1_ADC_CHANNEL   ADC1_CHANNEL_0
#define SOIL_SENSOR_2_ADC_CHANNEL   ADC1_CHANNEL_3
#define SOIL_SENSOR_3_ADC_CHANNEL   ADC1_CHANNEL_6
#define SOIL_SENSOR_4_ADC_CHANNEL   ADC1_CHANNEL_7
#define SOIL_SENSOR_ADC_UNIT        ADC_UNIT_1

// --- LDR Light Sensor ---
#define LIGHT_SENSOR_PIN          GPIO_NUM_32
#define LIGHT_SENSOR_ADC_CHANNEL  ADC_CHANNEL_4
#define LIGHT_SENSOR_ADC_UNIT     ADC_UNIT_1

// --- Pump Relays ---
#define PUMP_1_PIN                GPIO_NUM_33
#define PUMP_2_PIN                GPIO_NUM_25
#define PUMP_3_PIN                GPIO_NUM_26
#define PUMP_4_PIN                GPIO_NUM_27

// --- SPI Display ---
#define DISPLAY_MOSI_PIN          GPIO_NUM_23
#define DISPLAY_CLK_PIN           GPIO_NUM_18
#define DISPLAY_CS_PIN            GPIO_NUM_5
#define DISPLAY_DC_PIN            GPIO_NUM_16
#define DISPLAY_RST_PIN           GPIO_NUM_17
//#define DISPLAY_BL_PIN            GPIO_NUM_15

// --- UI ---
#define RED_WIFI_LED              GPIO_NUM_12
#define MOISTURE_LED              GPIO_NUM_13
#define BUTTON_PIN_WHITE          GPIO_NUM_4
#define BUTTON_PIN_BLUE           GPIO_NUM_19

