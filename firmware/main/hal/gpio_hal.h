/**
 * @file gpio_hal.h
 * @brief Botão BOOT (input de navegação, já que a ES3N28P não tem touch) e LED RGB.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DC_BTN_EVENT_NONE = 0,
    DC_BTN_EVENT_SHORT_PRESS, /* avança para o próximo item do menu */
    DC_BTN_EVENT_LONG_PRESS,  /* seleciona o item atual */
} dc_btn_event_t;

esp_err_t dc_gpio_hal_init(void);

/**
 * @brief Deve ser chamado periodicamente (ex. a cada 20 ms) pela ui_task.
 * @return o evento detetado desde a última chamada (debounce + long-press incluídos).
 */
dc_btn_event_t dc_gpio_hal_poll_button(void);

/** @brief Liga o LED de estado com uma cor RGB (0-255 cada canal). */
void dc_gpio_hal_set_led(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif
