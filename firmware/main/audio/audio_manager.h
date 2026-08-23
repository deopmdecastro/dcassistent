/**
 * @file audio_manager.h
 * @brief API pública do subsistema de áudio. A UI e outros serviços só
 * devem incluir este header — audio_input/audio_output são detalhe interno.
 *
 * Âmbito da DC 0.3: inicialização do codec ES8311, controlo de volume,
 * estados de reprodução e feedback sonoro básico (toques de UI). O
 * pipeline de voz completo (mic -> STT -> IA -> TTS, ver docs/roadmap.md
 * "DC 0.3 — Voz" no sentido do produto) fica para uma fase seguinte: aqui
 * apenas se prepara a captura de áudio (audio_input) para esse consumidor
 * futuro, sem inventar um STT/wake-word que não existe.
 */
#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DC_AUDIO_STATE_IDLE = 0,
    DC_AUDIO_STATE_PLAYING,
    DC_AUDIO_STATE_RECORDING,   /* reservado para quando a captura de voz for ligada a um consumidor */
    DC_AUDIO_STATE_ERROR,
} dc_audio_state_t;

typedef enum {
    DC_AUDIO_FEEDBACK_TAP = 0,   /* toque curto num botão/ícone */
    DC_AUDIO_FEEDBACK_CONFIRM,   /* ação confirmada (ex.: Wi-Fi ligado) */
    DC_AUDIO_FEEDBACK_ERROR,     /* ação falhou */
} dc_audio_feedback_t;

/**
 * @brief Inicializa o codec ES8311 (I2S + I2C) e arranca em estado IDLE.
 * Reutiliza o barramento I2C já criado por touch_hal (dc_touch_hal_get_i2c_bus).
 * Se essa inicialização ainda não tiver corrido (placa sem touch), este
 * módulo cria o seu próprio barramento I2C.
 */
esp_err_t dc_audio_manager_init(void);

/** @brief Volume 0-100. Persiste em NVS via settings_manager. */
esp_err_t dc_audio_manager_set_volume(uint8_t volume_pct);
uint8_t   dc_audio_manager_get_volume(void);

/** @brief Toca um som de feedback curto e pré-definido (não bloqueia a task chamadora). */
esp_err_t dc_audio_manager_play_feedback(dc_audio_feedback_t sound);

dc_audio_state_t dc_audio_manager_get_state(void);

/** @return true se o codec respondeu na inicialização (false = a correr em modo silencioso/degradado). */
bool dc_audio_manager_is_ready(void);

#ifdef __cplusplus
}
#endif
