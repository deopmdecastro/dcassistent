/**
 * @file lcd_hal.c
 * @brief Inicialização de baixo nível do LCD ILI9341 (SPI 4 fios) e backlight PWM.
 *
 * Usa o driver oficial esp_lcd_ili9341 (idf_component.yml) sobre o barramento
 * SPI2_HOST. O reset físico é partilhado com o reset do ESP32-S3 (RST=-1),
 * por isso não fazemos reset por GPIO dedicado.
 */
#include "lcd_hal.h"
#include "app_config.h"

#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "dc_lcd_hal";

#define DC_BL_LEDC_TIMER    LEDC_TIMER_0
#define DC_BL_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define DC_BL_LEDC_CHANNEL  LEDC_CHANNEL_0
#define DC_BL_LEDC_DUTY_RES LEDC_TIMER_10_BIT /* 0-1023 */

static esp_err_t dc_lcd_backlight_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = DC_BL_LEDC_MODE,
        .timer_num       = DC_BL_LEDC_TIMER,
        .duty_resolution = DC_BL_LEDC_DUTY_RES,
        .freq_hz         = DC_LCD_BL_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "ledc_timer_config");

    ledc_channel_config_t ch_cfg = {
        .gpio_num   = DC_LCD_PIN_BL,
        .speed_mode = DC_BL_LEDC_MODE,
        .channel    = DC_BL_LEDC_CHANNEL,
        .timer_sel  = DC_BL_LEDC_TIMER,
        .duty       = 0, /* desligado por omissão (igual ao hardware) */
        .hpoint     = 0,
    };
    return ledc_channel_config(&ch_cfg);
}

void dc_lcd_hal_set_brightness(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    uint32_t max_duty = (1 << DC_BL_LEDC_DUTY_RES) - 1;
    uint32_t duty = (max_duty * percent) / 100;
    ledc_set_duty(DC_BL_LEDC_MODE, DC_BL_LEDC_CHANNEL, duty);
    ledc_update_duty(DC_BL_LEDC_MODE, DC_BL_LEDC_CHANNEL);
}

esp_err_t dc_lcd_hal_init(esp_lcd_panel_handle_t *out_panel,
                           esp_lcd_panel_io_handle_t *out_io_handle)
{
    ESP_LOGI(TAG, "A inicializar LCD ILI9341 (%s) — %dx%d",
             DC_BOARD_NAME, DC_LCD_H_RES, DC_LCD_V_RES);

    spi_bus_config_t bus_cfg = {
        .sclk_io_num     = DC_LCD_PIN_SCK,
        .mosi_io_num     = DC_LCD_PIN_MOSI,
        .miso_io_num     = DC_LCD_PIN_MISO,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = DC_LCD_H_RES * DC_LVGL_BUF_LINES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DC_LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num       = DC_LCD_PIN_CS,
        .dc_gpio_num       = DC_LCD_PIN_DC,
        .spi_mode          = 0,
        .pclk_hz           = DC_LCD_SPI_CLOCK_HZ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)DC_LCD_SPI_HOST, &io_cfg, out_io_handle));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = DC_LCD_PIN_RST,
        .color_space    = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(*out_io_handle, &panel_cfg, out_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(*out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*out_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*out_panel, true));

    ESP_ERROR_CHECK(dc_lcd_backlight_init());
    dc_lcd_hal_set_brightness(80); /* brilho inicial — ajustável nas Definições */

    ESP_LOGI(TAG, "LCD pronto");
    return ESP_OK;
}
