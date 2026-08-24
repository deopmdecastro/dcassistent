/**
 * @file touch_hal.c
 * @brief Driver de touch FT6336G (I2C addr 0x38) sobre o barramento partilhado
 * com o codec de áudio ES8311. Usa o componente espressif/esp_lcd_touch_ft5x06
 * (ver idf_component.yml) sobre a API i2c_master do ESP-IDF >= 5.3.
 *
 * IMPORTANTE (documentar, não simular): este ficheiro foi escrito e revisto
 * por código, mas NÃO foi compilado nem testado em hardware real nesta
 * sessão de trabalho (sem acesso a ESP-IDF/toolchain nem à placa física).
 * Antes do primeiro flash: correr `idf.py build` e confirmar que a versão
 * instalada de esp_lcd_touch_ft5x06 expõe exatamente estas assinaturas.
 */
#include "touch_hal.h"
#include "app_config.h"

#include "esp_lcd_touch_ft5x06.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "dc_touch_hal";

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_touch_handle_t  s_touch_handle = NULL;
static bool                    s_ready = false;

i2c_master_bus_handle_t dc_touch_hal_get_i2c_bus(void)
{
    return s_i2c_bus;
}

bool dc_touch_hal_is_ready(void)
{
    return s_ready;
}

esp_err_t dc_touch_hal_init(esp_lcd_touch_handle_t *out_touch_handle)
{
#if !DC_BOARD_HAS_TOUCH
    ESP_LOGE(TAG, "dc_touch_hal_init chamado numa placa sem touch (DC_BOARD_HAS_TOUCH=0)");
    return ESP_ERR_NOT_SUPPORTED;
#else
    ESP_LOGI(TAG, "A inicializar touch FT6336G no barramento I2C partilhado com o codec");

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = DC_I2C_PORT,
        .sda_io_num        = DC_I2C_PIN_SDA,
        .scl_io_num        = DC_I2C_PIN_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_i2c_bus), TAG, "i2c_new_master_bus");

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_cfg.scl_speed_hz = DC_I2C_FREQ_HZ;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_cfg, &tp_io_handle),
        TAG, "esp_lcd_new_panel_io_i2c (touch)");

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = DC_LCD_H_RES,
        .y_max = DC_LCD_V_RES,
        .rst_gpio_num = DC_TOUCH_PIN_RST,
        .int_gpio_num = DC_TOUCH_PIN_INT,
        .flags = {
            .swap_xy  = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_err_t err = esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &s_touch_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_touch_new_i2c_ft5x06 falhou (%s) — a UI cai para "
                       "navegação só por botão BOOT como fallback", esp_err_to_name(err));
        s_ready = false;
        return err;
    }

    s_ready = true;
    *out_touch_handle = s_touch_handle;
    ESP_LOGI(TAG, "Touch FT6336G pronto (%dx%d)", DC_LCD_H_RES, DC_LCD_V_RES);
    return ESP_OK;
#endif
}
