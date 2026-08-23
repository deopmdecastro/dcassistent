/**
 * @file audio_manager.c
 * @brief Ver audio_manager.h.
 */
#include "audio_manager.h"
#include "audio_output.h"
#include "audio_input.h"
#include "hal/touch_hal.h"
#include "storage/settings_manager.h"
#include "app_config.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *TAG = "dc_audio_manager";

static dc_audio_state_t s_state = DC_AUDIO_STATE_IDLE;
static bool s_ready = false;

/* Buffer pequeno para tons de feedback — gerado uma vez, reutilizado. */
#define DC_TONE_SAMPLE_COUNT 320 /* 20ms a 16kHz */
static int16_t s_tone_buf[DC_TONE_SAMPLE_COUNT];

static void dc_audio_fill_tone(float freq_hz, float amplitude)
{
    for (int i = 0; i < DC_TONE_SAMPLE_COUNT; i++) {
        float t = (float)i / (float)DC_AUDIO_SAMPLE_RATE_HZ;
        s_tone_buf[i] = (int16_t)(amplitude * 32767.0f * sinf(2.0f * (float)M_PI * freq_hz * t));
    }
}

static void dc_audio_feedback_task(void *arg)
{
    dc_audio_feedback_t sound = (dc_audio_feedback_t)(intptr_t)arg;
    float freq = 880.0f;
    switch (sound) {
    case DC_AUDIO_FEEDBACK_TAP:     freq = 1200.0f; break;
    case DC_AUDIO_FEEDBACK_CONFIRM: freq = 880.0f;  break;
    case DC_AUDIO_FEEDBACK_ERROR:   freq = 220.0f;  break;
    }
    dc_audio_fill_tone(freq, 0.3f);
    s_state = DC_AUDIO_STATE_PLAYING;
    dc_audio_output_write(s_tone_buf, DC_TONE_SAMPLE_COUNT);
    s_state = DC_AUDIO_STATE_IDLE;
    vTaskDelete(NULL);
}

esp_err_t dc_audio_manager_init(void)
{
    ESP_LOGI(TAG, "A inicializar subsistema de áudio");

    i2c_master_bus_handle_t bus = dc_touch_hal_get_i2c_bus();
    if (bus == NULL) {
        ESP_LOGW(TAG, "Barramento I2C partilhado (touch) indisponível — este build "
                       "assume que dc_touch_hal_init() já correu antes. Sem touch "
                       "(ex. ES3N28P) seria preciso criar aqui um barramento I2C próprio.");
        s_ready = false;
        s_state = DC_AUDIO_STATE_ERROR;
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = dc_audio_output_init(bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha a inicializar saída de áudio: %s — DC continua a "
                       "funcionar sem som (modo degradado)", esp_err_to_name(err));
        s_ready = false;
        s_state = DC_AUDIO_STATE_ERROR;
        return err;
    }

    dc_audio_output_set_volume(dc_settings_get_volume());
    s_ready = true;
    s_state = DC_AUDIO_STATE_IDLE;
    ESP_LOGI(TAG, "Subsistema de áudio pronto");
    return ESP_OK;
}

esp_err_t dc_audio_manager_set_volume(uint8_t volume_pct)
{
    if (volume_pct > 100) {
        volume_pct = 100;
    }
    dc_settings_set_volume(volume_pct);
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE; /* guardado para quando o áudio ficar disponível */
    }
    return dc_audio_output_set_volume(volume_pct);
}

uint8_t dc_audio_manager_get_volume(void)
{
    return dc_settings_get_volume();
}

esp_err_t dc_audio_manager_play_feedback(dc_audio_feedback_t sound)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    /* Corre numa task própria de baixa prioridade para nunca bloquear quem
     * chamou (tipicamente a UI task num onClick). */
    BaseType_t ok = xTaskCreatePinnedToCore(
        dc_audio_feedback_task, "dc_audio_fb", DC_TASK_AUDIO_STACK_SIZE,
        (void *)(intptr_t)sound, DC_TASK_AUDIO_PRIORITY, NULL, DC_TASK_AUDIO_CORE);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

dc_audio_state_t dc_audio_manager_get_state(void)
{
    return s_state;
}

bool dc_audio_manager_is_ready(void)
{
    return s_ready;
}
