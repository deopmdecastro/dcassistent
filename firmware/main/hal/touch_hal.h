/**
 * @file touch_hal.h
 * @brief Inicialização do touch capacitivo FT6336G (ES3C28P) e do barramento
 * I2C partilhado com o codec de áudio ES8311.
 *
 * Esta HAL é a única dona do barramento I2C (DC_I2C_PORT). Qualquer outro
 * módulo que precise do mesmo barramento (ex. audio/AudioManager para o
 * ES8311) deve pedir o handle por dc_touch_hal_get_i2c_bus(), nunca chamar
 * i2c_new_master_bus() outra vez.
 */
#pragma once

#include "esp_err.h"
#include "esp_lcd_touch.h"
#include "driver/i2c_master.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o barramento I2C partilhado e o driver de touch FT6336G.
 *
 * Só deve ser chamada se DC_BOARD_HAS_TOUCH == 1. Em placas sem touch
 * (ES3N28P) este ficheiro não deve ser compilado/chamado.
 *
 * @param[out] out_touch_handle Handle esp_lcd_touch, usado depois pelo
 *                               esp_lvgl_port (lvgl_port_add_touch).
 */
esp_err_t dc_touch_hal_init(esp_lcd_touch_handle_t *out_touch_handle);

/**
 * @brief Devolve o handle do barramento I2C partilhado (para o AudioManager
 * inicializar o codec ES8311 no mesmo barramento em vez de criar outro).
 * Só é válido depois de dc_touch_hal_init() ter sido chamado com sucesso.
 */
i2c_master_bus_handle_t dc_touch_hal_get_i2c_bus(void);

/** @return true se o touch foi inicializado e está a responder no barramento. */
bool dc_touch_hal_is_ready(void);

#ifdef __cplusplus
}
#endif
