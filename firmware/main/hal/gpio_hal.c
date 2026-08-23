/**
 * @file gpio_hal.c
 * @brief Leitura debounced do botão BOOT (short/long press) e controlo do LED WS2812B.
 *
 * A ES3N28P não tem touch, por isso o botão BOOT (IO0) é reaproveitado como
 * único input físico da interface V1:
 *   - toque curto  -> navega para o próximo item
 *   - toque longo  -> seleciona o item atual
 *
 * NOTA: IO0 também controla o modo de download (se estiver em nível baixo no
 * boot/reset). Isto é normal e não interfere com o uso pós-boot como botão.
 */
#include "gpio_hal.h"
#include "app_config.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "dc_gpio_hal";

#define DC_BTN_DEBOUNCE_US      30000    /* 30 ms */
#define DC_BTN_LONG_PRESS_US    600000   /* 600 ms */

static led_strip_handle_t s_led_strip = NULL;

static bool s_btn_was_pressed = false;
static int64_t s_btn_press_started_us = 0;
static int64_t s_btn_last_change_us = 0;

static esp_err_t dc_led_init(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = DC_PIN_LED_RGB,
        .max_leds       = 1,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .resolution_hz = 10 * 1000 * 1000,
    };
    esp_err_t err = led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_led_strip);
    if (err == ESP_OK) {
        led_strip_clear(s_led_strip);
    }
    return err;
}

void dc_gpio_hal_set_led(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led_strip) {
        return;
    }
    led_strip_set_pixel(s_led_strip, 0, r, g, b);
    led_strip_refresh(s_led_strip);
}

esp_err_t dc_gpio_hal_init(void)
{
    gpio_config_t btn_cfg = {
        .pin_bit_mask = 1ULL << DC_PIN_BTN_BOOT,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE, /* BOOT já tem pull-up externo, mas garantimos */
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&btn_cfg);
    if (err != ESP_OK) {
        return err;
    }

    err = dc_led_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "LED RGB não inicializou (%s) — a continuar sem LED", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "GPIO HAL pronto (botão BOOT em IO%d)", DC_PIN_BTN_BOOT);
    return ESP_OK;
}

dc_btn_event_t dc_gpio_hal_poll_button(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_btn_last_change_us < DC_BTN_DEBOUNCE_US) {
        return DC_BTN_EVENT_NONE;
    }

    bool pressed = (gpio_get_level(DC_PIN_BTN_BOOT) == 0); /* ativo em nível baixo */

    if (pressed && !s_btn_was_pressed) {
        s_btn_press_started_us = now;
        s_btn_was_pressed = true;
        s_btn_last_change_us = now;
        return DC_BTN_EVENT_NONE;
    }

    if (!pressed && s_btn_was_pressed) {
        s_btn_was_pressed = false;
        s_btn_last_change_us = now;
        int64_t held_us = now - s_btn_press_started_us;
        return (held_us >= DC_BTN_LONG_PRESS_US)
                   ? DC_BTN_EVENT_LONG_PRESS
                   : DC_BTN_EVENT_SHORT_PRESS;
    }

    return DC_BTN_EVENT_NONE;
}
