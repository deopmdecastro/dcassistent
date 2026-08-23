/**
 * @file audio_output.h
 * @brief Saída de áudio: I2S standard mode + codec ES8311 (via espressif/esp_codec_dev).
 * Uso interno de audio_manager — não incluir fora de audio/.
 */
#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param i2c_bus Barramento I2C partilhado (do touch_hal ou criado de novo se
 *                 não existir touch nesta build).
 */
esp_err_t dc_audio_output_init(i2c_master_bus_handle_t i2c_bus);

esp_err_t dc_audio_output_set_volume(uint8_t volume_pct);

/** @brief Reproduz um buffer PCM16 mono já pronto (bloqueia a task chamadora — usar a partir de uma task de áudio dedicada, nunca da UI task). */
esp_err_t dc_audio_output_write(const int16_t *pcm_samples, size_t sample_count);

bool dc_audio_output_is_ready(void);

#ifdef __cplusplus
}
#endif
