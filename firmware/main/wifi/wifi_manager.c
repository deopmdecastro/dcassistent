/**
 * @file wifi_manager.c
 * @brief Ver wifi_manager.h. Máquina de estados orientada a eventos
 * (esp_event) — nenhuma chamada aqui bloqueia a task que a invoca.
 *
 * NÃO TESTADO EM HARDWARE nesta sessão (sem toolchain ESP-IDF disponível no
 * ambiente onde este código foi escrito). Reveja com `idf.py build` antes do
 * primeiro flash — ver nota equivalente em hal/touch_hal.c.
 */
#include "wifi_manager.h"
#include "storage/settings_manager.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "dc_wifi_manager";

#define DC_WIFI_MAX_RETRIES_BEFORE_ERROR   5
#define DC_WIFI_RETRY_BASE_DELAY_MS        2000

static dc_wifi_state_t     s_state = DC_WIFI_STATE_OFF;
static dc_wifi_state_cb_t  s_state_cb = NULL;
static esp_netif_t        *s_netif = NULL;
static uint8_t             s_retry_count = 0;
static bool                s_wifi_started = false;

static void dc_wifi_set_state(dc_wifi_state_t new_state)
{
    if (s_state == new_state) {
        return;
    }
    s_state = new_state;
    ESP_LOGI(TAG, "Estado Wi-Fi -> %d", (int)new_state);
    if (s_state_cb) {
        s_state_cb(new_state);
    }
}

static void dc_wifi_try_reconnect(void)
{
    if (s_retry_count >= DC_WIFI_MAX_RETRIES_BEFORE_ERROR) {
        ESP_LOGW(TAG, "Limite de tentativas atingido — a marcar erro (reconexão manual pela UI)");
        dc_wifi_set_state(DC_WIFI_STATE_ERROR);
        return;
    }
    s_retry_count++;
    ESP_LOGI(TAG, "A reconectar (tentativa %d/%d)", s_retry_count, DC_WIFI_MAX_RETRIES_BEFORE_ERROR);
    dc_wifi_set_state(DC_WIFI_STATE_CONNECTING);
    /* esp_wifi_connect() é não-bloqueante — o resultado chega por evento. */
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        ESP_LOGW(TAG, "esp_wifi_connect() falhou de imediato: %s", esp_err_to_name(err));
        dc_wifi_set_state(DC_WIFI_STATE_ERROR);
    }
}

static void dc_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            dc_wifi_try_reconnect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGW(TAG, "Desligado da rede (motivo=%d)", evt ? evt->reason : -1);
            /* Reconexão controlada: não tenta imediatamente em loop apertado —
             * dc_wifi_try_reconnect incrementa o contador de tentativas. */
            dc_wifi_try_reconnect();
            break;
        }
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Ligado, IP=" IPSTR, IP2STR(&evt->ip_info.ip));
        s_retry_count = 0;
        dc_wifi_set_state(DC_WIFI_STATE_CONNECTED);
    }
}

static esp_err_t dc_wifi_apply_credentials(const char *ssid, const char *password)
{
    wifi_config_t wifi_cfg = { 0 };
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
    }
    wifi_cfg.sta.threshold.authmode = (password && strlen(password) > 0)
                                           ? WIFI_AUTH_WPA2_PSK
                                           : WIFI_AUTH_OPEN;
    return esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
}

esp_err_t dc_wifi_manager_start(dc_wifi_state_cb_t state_cb)
{
    s_state_cb = state_cb;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &dc_wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &dc_wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    dc_wifi_credentials_t creds;
    esp_err_t load_err = dc_settings_load_wifi(&creds);
    if (load_err != ESP_OK || !creds.has_credentials) {
        ESP_LOGI(TAG, "Sem credenciais Wi-Fi guardadas — a aguardar configuração pela UI");
        dc_wifi_set_state(DC_WIFI_STATE_NO_NETWORK);
        ESP_ERROR_CHECK(esp_wifi_start());
        s_wifi_started = true;
        return ESP_OK;
    }

    ESP_ERROR_CHECK(dc_wifi_apply_credentials(creds.ssid, creds.password));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_wifi_started = true;
    /* A ligação em si arranca no handler de WIFI_EVENT_STA_START — não bloqueia aqui. */
    return ESP_OK;
}

esp_err_t dc_wifi_manager_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = dc_settings_save_wifi(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Falha a guardar credenciais em NVS: %s", esp_err_to_name(err));
        /* Continua mesmo assim — tenta ligar com o que foi pedido, mas não
         * sobreviverá a um reboot se a escrita falhou. */
    }

    ESP_ERROR_CHECK(dc_wifi_apply_credentials(ssid, password));
    s_retry_count = 0;

    if (!s_wifi_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
        s_wifi_started = true;
        /* liga-se automaticamente via WIFI_EVENT_STA_START */
    } else {
        esp_wifi_disconnect();
        dc_wifi_try_reconnect();
    }
    return ESP_OK;
}

esp_err_t dc_wifi_manager_forget(void)
{
    esp_err_t err = dc_settings_clear_wifi();
    if (s_wifi_started) {
        esp_wifi_disconnect();
    }
    dc_wifi_set_state(DC_WIFI_STATE_NO_NETWORK);
    return err;
}

dc_wifi_state_t dc_wifi_manager_get_state(void)
{
    return s_state;
}

bool dc_wifi_manager_is_connected(void)
{
    return s_state == DC_WIFI_STATE_CONNECTED;
}
