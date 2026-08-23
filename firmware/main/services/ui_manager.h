/**
 * @file ui_manager.h
 * @brief Serviço dono do LVGL: arranca a ui_task, gere ecrãs e navegação.
 *
 * Regra de dependência (docs/firmware-architecture.md secção 2):
 * ninguém fora deste ficheiro deve chamar funções lv_* diretamente.
 * Outras tarefas comunicam por eventos (ver dc_ui_notify_*).
 */
#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DC_UI_STATE_IDLE = 0,   /* ecrã Home parado */
    DC_UI_STATE_LISTENING,  /* a ouvir (wake word / voz) */
    DC_UI_STATE_THINKING,   /* a processar no Gateway */
    DC_UI_STATE_SPEAKING,   /* a responder por voz */
} dc_ui_state_t;

/**
 * @brief Inicializa o LVGL + display + input e arranca a ui_task.
 * Deve ser chamado uma única vez, depois do LCD HAL estar pronto.
 */
esp_err_t dc_ui_manager_start(void);

/** @brief Atualiza o ícone de Wi-Fi no ecrã Home (chamado pelo net_manager). */
void dc_ui_notify_wifi_connected(bool connected);

/** @brief Atualiza a percentagem de bateria mostrada no ecrã Home. */
void dc_ui_notify_battery(uint8_t percent);

/** @brief Muda o estado visual da DC (ouvir/pensar/falar) — animações no círculo central. */
void dc_ui_notify_state(dc_ui_state_t state);

#ifdef __cplusplus
}
#endif
