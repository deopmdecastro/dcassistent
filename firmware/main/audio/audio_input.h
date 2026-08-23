/**
 * @file audio_input.h
 * @brief Captura de microfone (mono, via o mesmo codec ES8311/I2S da saída).
 *
 * ESTADO REAL (não simulado): esta camada só abre o canal de captura e
 * expõe dc_audio_input_read(); NÃO existe wake-word, VAD nem STT — isso
 * é trabalho de uma fase futura (DC 0.4+ no roadmap) e não deve ser
 * inventado aqui. Quem chamar dc_audio_input_start() é responsável por
 * decidir o que fazer com as amostras (ex.: um futuro AudioService de
 * comandos de voz, ou uma app de gravação).
 */
#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Abre o canal de captura I2S/codec em modo IN. Idempotente. */
esp_err_t dc_audio_input_start(void);

/** @brief Fecha o canal de captura, libertando o barramento para uso exclusivo de saída. */
esp_err_t dc_audio_input_stop(void);

/**
 * @brief Lê até sample_count amostras PCM16 mono (bloqueia até haver dados
 * ou timeout_ms expirar). Chamar apenas depois de dc_audio_input_start().
 */
esp_err_t dc_audio_input_read(int16_t *out_samples, size_t sample_count, uint32_t timeout_ms);

bool dc_audio_input_is_active(void);

#ifdef __cplusplus
}
#endif
