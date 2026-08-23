/**
 * @file audio_output.c
 * @brief Ver audio_output.h.
 *
 * NÃO TESTADO EM HARDWARE nesta sessão (sem toolchain ESP-IDF/placa física
 * disponíveis no ambiente onde este código foi escrito — ver nota igual em
 * hal/touch_hal.c). Antes do primeiro flash, confirmar contra a versão
 * instalada de espressif/esp_codec_dev:
 *   - o nome exato dos structs de configuração do driver ES8311 (aqui
 *     assume-se a API "esp_codec_dev" genérica com um driver ES8311 por
 *     baixo, que é o padrão dos exemplos oficiais da Espressif);
 *   - se o I2S standard mode (driver/i2s_std.h) é o transporte esperado
 *     pelo esp_codec_dev nesta versão, ou se é preciso um adaptador.
 *
 * Este ficheiro fica isolado exatamente para que, se a API tiver de mudar,
 * o resto do projeto (audio_manager, UI, feedback sonoro) não seja afetado.
 */
#include "audio_output.h"
#include "app_config.h"

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "dc_audio_output";

static i2s_chan_handle_t s_i2s_tx_chan = NULL;
static esp_codec_dev_handle_t s_codec_dev = NULL;
static bool s_ready = false;

static esp_err_t dc_audio_i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_i2s_tx_chan, NULL), TAG, "i2s_new_channel");

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(DC_AUDIO_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            (i2s_data_bit_width_t)DC_AUDIO_BITS_PER_SAMPLE, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = DC_AUDIO_PIN_MCLK,
            .bclk = DC_AUDIO_PIN_BCLK,
            .ws   = DC_AUDIO_PIN_LRCK,
            .dout = DC_AUDIO_PIN_DOUT,
            .din  = DC_AUDIO_PIN_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_i2s_tx_chan, &std_cfg), TAG, "i2s_channel_init_std_mode");
    return i2s_channel_enable(s_i2s_tx_chan);
}

esp_err_t dc_audio_output_init(i2c_master_bus_handle_t i2c_bus)
{
    ESP_LOGI(TAG, "A inicializar saída de áudio (ES8311 + I2S)");

    /* Amplificador da coluna: ativo em nível baixo (ver app_config.h) */
    gpio_config_t amp_cfg = {
        .pin_bit_mask = 1ULL << DC_AUDIO_PIN_AMP_EN,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&amp_cfg);
    gpio_set_level(DC_AUDIO_PIN_AMP_EN, 0); /* ligado (ativo-baixo) */

    esp_err_t err = dc_audio_i2s_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha a inicializar I2S: %s", esp_err_to_name(err));
        return err;
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = DC_I2C_PORT,
        .addr = DC_AUDIO_CODEC_I2C_ADDR,
        .bus_handle = i2c_bus,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    ESP_RETURN_ON_FALSE(ctrl_if != NULL, ESP_FAIL, TAG, "audio_codec_new_i2c_ctrl falhou");

    audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = DC_AUDIO_PIN_AMP_EN,
        .pa_reverted = true, /* ativo-baixo */
        .use_mclk = true,
    };
    const audio_codec_if_t *codec_if = es8311_codec_new(&es8311_cfg);
    ESP_RETURN_ON_FALSE(codec_if != NULL, ESP_FAIL, TAG, "es8311_codec_new falhou");

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = NULL, /* usa o canal I2S já configurado externamente */
    };
    s_codec_dev = esp_codec_dev_new(&dev_cfg);
    ESP_RETURN_ON_FALSE(s_codec_dev != NULL, ESP_FAIL, TAG, "esp_codec_dev_new falhou");

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = DC_AUDIO_SAMPLE_RATE_HZ,
        .channel = 1,
        .bits_per_sample = DC_AUDIO_BITS_PER_SAMPLE,
    };
    ESP_RETURN_ON_ERROR(esp_codec_dev_open(s_codec_dev, &fs), TAG, "esp_codec_dev_open");

    s_ready = true;
    ESP_LOGI(TAG, "Saída de áudio pronta (%d Hz, %d bits, mono)",
             DC_AUDIO_SAMPLE_RATE_HZ, DC_AUDIO_BITS_PER_SAMPLE);
    return ESP_OK;
}

esp_err_t dc_audio_output_set_volume(uint8_t volume_pct)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (volume_pct > 100) {
        volume_pct = 100;
    }
    return esp_codec_dev_set_out_vol(s_codec_dev, volume_pct);
}

esp_err_t dc_audio_output_write(const int16_t *pcm_samples, size_t sample_count)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_codec_dev_write(s_codec_dev, (void *)pcm_samples, sample_count * sizeof(int16_t));
}

bool dc_audio_output_is_ready(void)
{
    return s_ready;
}
