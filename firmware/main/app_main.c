/**
 * @file app_main.c
 * @brief Boot da DC (placa ES3C28P). Fase atual: DC 0.3 — Wi-Fi, NVS/settings,
 * touch, arquitetura de áudio. Ligação ao Gateway, OTA e voz completa (STT/IA/
 * TTS) continuam para fases seguintes (ver docs/roadmap.md).
 *
 * Ordem de arranque (importa):
 *   1. storage (NVS raw) -> settings (typed, por cima do storage)
 *   2. GPIO/LED (independente)
 *   3. UI (LVGL + LCD + touch) — o touch_hal cria o barramento I2C partilhado
 *   4. Audio — reaproveita esse mesmo barramento I2C (por isso vem depois da UI)
 *   5. Wi-Fi — não bloqueia; liga-se em background e notifica a UI por callback
 */
#include "app_config.h"
#include "hal/gpio_hal.h"
#include "services/ui_manager.h"
#include "storage/storage_manager.h"
#include "storage/settings_manager.h"
#include "wifi/wifi_manager.h"
#include "audio/audio_manager.h"
#include "net/web_server.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "dc_app_main";

static void dc_log_memory(void)
{
    ESP_LOGI(TAG, "Heap interno livre: %d bytes | PSRAM livre: %d bytes",
             (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

/** @brief Ponte Wi-Fi -> UI. Mantém a UI livre de saber como o Wi-Fi funciona. */
static void dc_on_wifi_state_changed(dc_wifi_state_t state)
{
    dc_ui_notify_wifi_connected(state == DC_WIFI_STATE_CONNECTED);
}

void app_main(void)
{
    ESP_LOGI(TAG, "DC 0.3 a arrancar — placa %s", DC_BOARD_NAME);

    ESP_ERROR_CHECK(dc_storage_manager_init());
    ESP_ERROR_CHECK(dc_settings_manager_init());

    ESP_ERROR_CHECK(dc_gpio_hal_init());
    dc_gpio_hal_set_led(0, 20, 10); /* verde ténue = a arrancar */

    ESP_ERROR_CHECK(dc_ui_manager_start());
    dc_ui_notify_state(DC_UI_STATE_IDLE);

    esp_err_t audio_err = dc_audio_manager_init();
    if (audio_err != ESP_OK) {
        ESP_LOGW(TAG, "Áudio em modo degradado (%s) — DC continua a funcionar sem som",
                 esp_err_to_name(audio_err));
    }

    /* Web server (HTTP + WS) — arranca em espera pela ligação Wi-Fi. */
    esp_err_t web_err = dc_web_server_init();
    if (web_err != ESP_OK) {
        ESP_LOGW(TAG, "Web server inicializacao com aviso (%s) — UI web pode ficar indisponivel",
                 esp_err_to_name(web_err));
    }

    esp_err_t wifi_err = dc_wifi_manager_start(dc_on_wifi_state_changed);
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "Falha a arrancar o Wi-Fi manager: %s", esp_err_to_name(wifi_err));
    }

    dc_gpio_hal_set_led(0, 0, 0); /* LED apaga após arranque — estados futuros acendem-no */

    dc_log_memory();
    ESP_LOGI(TAG, "Boot concluido. Interface pronta (touch + botao BOOT). "
                   "Wi-Fi e audio a inicializar/ligar em background.");
}
