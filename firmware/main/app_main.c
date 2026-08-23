/**
 * @file app_main.c
 * @brief Boot da DC V1 (ES3N28P). Fase atual: DC 0.1 + 0.2 — HAL + interface
 * LVGL navegável. Wi-Fi, áudio, ligação ao Gateway e OTA entram nas fases
 * 0.3/0.4 (ver docs/roadmap.md e docs/firmware-architecture.md).
 */
#include "app_config.h"
#include "hal/gpio_hal.h"
#include "services/ui_manager.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

static const char *TAG = "dc_app_main";

static void dc_log_memory(void)
{
    ESP_LOGI(TAG, "Heap interno livre: %d bytes | PSRAM livre: %d bytes",
             (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void app_main(void)
{
    ESP_LOGI(TAG, "DC V1 a arrancar — placa %s", DC_BOARD_NAME);

    /* NVS é necessário mesmo nesta fase (Wi-Fi/BLE usam-no internamente
     * assim que forem ligados na fase 0.3; inicializar já evita retrabalho). */
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    ESP_ERROR_CHECK(dc_gpio_hal_init());
    dc_gpio_hal_set_led(0, 20, 10); /* verde ténue = a arrancar */

    ESP_ERROR_CHECK(dc_ui_manager_start());
    dc_ui_notify_state(DC_UI_STATE_IDLE);

    dc_gpio_hal_set_led(0, 0, 0); /* LED apaga após arranque — estados futuros acendem-no */

    dc_log_memory();
    ESP_LOGI(TAG, "Boot concluido. Interface pronta (navegacao: botao BOOT).");
}
