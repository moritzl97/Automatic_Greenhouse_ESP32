// connectivity/firebase.c
#include "firebase_db.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "parameter_config.h"
#include "utils.h"
#include "secrets.h"
#include <string.h>
#include "esp_crt_bundle.h"

#define TAG "FIREBASE"
#define FIREBASE_PATH "/measurements"

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

void firebase_post_measurements(measurements_t *m)
{
    // build JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature_C",          m->temperature);
    cJSON_AddNumberToObject(root, "relative_humidity_pct",  m->relative_humidity);
    cJSON_AddNumberToObject(root, "light_intensity_pct",    m->light);

    cJSON *pots = cJSON_AddArrayToObject(root, "pots");
    for (int i = 0; i < 4; i++) {
        cJSON *pot = cJSON_CreateObject();
        cJSON_AddNumberToObject(pot, "id",                i);
        cJSON_AddStringToObject(pot, "name",              m->pots[i].name);
        cJSON_AddBoolToObject(pot,   "active",            m->pots[i].active);
        cJSON_AddNumberToObject(pot, "soil_moisture_pct", m->pots[i].soil_moisture);
        cJSON_AddItemToArray(pots, pot);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    // POST to Firebase -- .json suffix required by REST API
    char url[256];
    snprintf(url, sizeof(url), "%s%s.json", FIREBASE_DB_URL, FIREBASE_PATH);

    esp_http_client_config_t config = {
        .url             = url,
        .method          = HTTP_METHOD_POST,
        .event_handler   = http_event_handler,
        .transport_type  = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,  // handles Firebase SSL cert
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json, strlen(json));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Posted measurements to Firebase");
    } else {
        ESP_LOGE(TAG, "Firebase POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_free(json);
}