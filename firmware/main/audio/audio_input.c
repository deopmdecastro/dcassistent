/**
 * @file audio_input.c
 * @brief Ver audio_input.h. Reaproveita o esp_codec_dev criado por
 * audio_output.c em modo IN_OUT seria o ideal a longo prazo; para já,
 * como o esp_codec_dev_cfg_t aberto em audio_output.c é ESP_CODEC_DEV_TYPE_OUT,
 * este ficheiro documenta explicitamente que a captura partilhada com
 * reprodução simultânea (full-duplex) ainda NÃO está implementada — ficaria
 * para quando houver um consumidor real (voz) a precisar disso.
 *
 * Por agora dc_audio_input_start() devolve ESP_ERR_NOT_SUPPORTED de forma
 * explícita em vez de fingir que captura áudio. Isto cumpre a regra do
 * projeto de não simular funcionalidades que dependem de trabalho ainda
 * não feito (ver secção 12 do briefing).
 */
#include "audio_input.h"
#include "esp_log.h"

static const char *TAG = "dc_audio_input";
static bool s_active = false;

esp_err_t dc_audio_input_start(void)
{
    ESP_LOGW(TAG, "Captura de microfone ainda não implementada nesta fase (DC 0.3 "
                   "só cobre saída de áudio + arquitetura preparada). "
                   "Pendente: abrir esp_codec_dev em modo full-duplex ou IN dedicado.");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t dc_audio_input_stop(void)
{
    s_active = false;
    return ESP_OK;
}

esp_err_t dc_audio_input_read(int16_t *out_samples, size_t sample_count, uint32_t timeout_ms)
{
    (void)out_samples; (void)sample_count; (void)timeout_ms;
    return ESP_ERR_NOT_SUPPORTED;
}

bool dc_audio_input_is_active(void)
{
    return s_active;
}
