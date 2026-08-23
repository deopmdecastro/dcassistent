/**
 * @file lcd_hal.h
 * @brief Inicialização de baixo nível do LCD ILI9341 (SPI) e do backlight.
 */
#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o barramento SPI, o painel ILI9341 e o backlight (PWM).
 *
 * @param[out] out_panel     Handle do painel LCD (usado depois pelo esp_lvgl_port).
 * @param[out] out_io_handle Handle do IO SPI do painel.
 * @return ESP_OK em sucesso.
 */
esp_err_t dc_lcd_hal_init(esp_lcd_panel_handle_t *out_panel,
                           esp_lcd_panel_io_handle_t *out_io_handle);

/**
 * @brief Define o brilho do backlight.
 * @param percent 0-100.
 */
void dc_lcd_hal_set_brightness(uint8_t percent);

#ifdef __cplusplus
}
#endif
