/**
 * @file settings_manager.h
 * @brief Preferências persistentes da DC (Wi-Fi, volume, tema, nome do
 * dispositivo, favoritos). A UI e o WiFiManager falam com este módulo,
 * nunca diretamente com storage_manager/NVS.
 */
#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DC_SETTINGS_SSID_MAX_LEN     32
#define DC_SETTINGS_PASS_MAX_LEN     64
#define DC_SETTINGS_DEVICE_NAME_LEN  24
#define DC_SETTINGS_MAX_FAVORITES    8

typedef struct {
    char ssid[DC_SETTINGS_SSID_MAX_LEN + 1];
    char password[DC_SETTINGS_PASS_MAX_LEN + 1];
    bool has_credentials;
} dc_wifi_credentials_t;

typedef enum {
    DC_THEME_DARK_PURPLE = 0, /* tema por omissão — ver design system DC 0.3 */
    DC_THEME_DARK_TEAL,       /* tema legado (DC 0.1/0.2) */
} dc_theme_t;

esp_err_t dc_settings_manager_init(void);

/* --- Wi-Fi --- */
esp_err_t dc_settings_save_wifi(const char *ssid, const char *password);
esp_err_t dc_settings_load_wifi(dc_wifi_credentials_t *out_creds);
esp_err_t dc_settings_clear_wifi(void);

/* --- Dispositivo / interface --- */
esp_err_t dc_settings_get_device_name(char *out_buf, size_t out_buf_len);
esp_err_t dc_settings_set_device_name(const char *name);

uint8_t dc_settings_get_volume(void);      /* 0-100, com valor por omissão seguro */
esp_err_t dc_settings_set_volume(uint8_t volume);

uint8_t dc_settings_get_brightness(void);  /* 0-100 */
esp_err_t dc_settings_set_brightness(uint8_t brightness);

dc_theme_t dc_settings_get_theme(void);
esp_err_t dc_settings_set_theme(dc_theme_t theme);

/* --- Favoritos (atalhos na tela de Apps) --- */
esp_err_t dc_settings_get_favorites(char out_ids[DC_SETTINGS_MAX_FAVORITES][16], uint8_t *out_count);
esp_err_t dc_settings_set_favorites(const char ids[DC_SETTINGS_MAX_FAVORITES][16], uint8_t count);

#ifdef __cplusplus
}
#endif
