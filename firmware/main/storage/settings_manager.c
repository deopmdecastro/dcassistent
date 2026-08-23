/**
 * @file settings_manager.c
 * @brief Ver settings_manager.h. Guarda tudo através de storage_manager
 * (nunca chama nvs_* diretamente).
 */
#include "settings_manager.h"
#include "storage_manager.h"

#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "dc_settings";

#define NS_WIFI     "dc_wifi"
#define NS_SETTINGS "dc_settings"

#define KEY_WIFI_SSID     "ssid"
#define KEY_WIFI_PASS     "pass"
#define KEY_DEVICE_NAME   "dev_name"
#define KEY_VOLUME        "volume"
#define KEY_BRIGHTNESS    "brightness"
#define KEY_THEME         "theme"
#define KEY_FAV_COUNT     "fav_count"
#define KEY_FAV_PREFIX    "fav_" /* fav_0, fav_1, ... */

#define DEFAULT_DEVICE_NAME "DC Assistant"
#define DEFAULT_VOLUME      60
#define DEFAULT_BRIGHTNESS  80

esp_err_t dc_settings_manager_init(void)
{
    /* Nada a inicializar para já — mantido por simetria com storage_manager
     * e para permitir, no futuro, migração de esquema de settings antigo. */
    return ESP_OK;
}

esp_err_t dc_settings_save_wifi(const char *ssid, const char *password)
{
    if (ssid == NULL || strlen(ssid) == 0 || strlen(ssid) > DC_SETTINGS_SSID_MAX_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = dc_storage_set_str(NS_WIFI, KEY_WIFI_SSID, ssid);
    if (err != ESP_OK) {
        return err;
    }
    return dc_storage_set_str(NS_WIFI, KEY_WIFI_PASS, password ? password : "");
}

esp_err_t dc_settings_load_wifi(dc_wifi_credentials_t *out_creds)
{
    memset(out_creds, 0, sizeof(*out_creds));
    esp_err_t err = dc_storage_get_str(NS_WIFI, KEY_WIFI_SSID,
                                        out_creds->ssid, sizeof(out_creds->ssid));
    if (err != ESP_OK) {
        out_creds->has_credentials = false;
        return err; /* tipicamente ESP_ERR_NVS_NOT_FOUND — primeira vez a arrancar */
    }
    /* password pode ser vazia (rede aberta) — não tratar isso como erro */
    dc_storage_get_str(NS_WIFI, KEY_WIFI_PASS, out_creds->password, sizeof(out_creds->password));
    out_creds->has_credentials = true;
    return ESP_OK;
}

esp_err_t dc_settings_clear_wifi(void)
{
    esp_err_t err = dc_storage_erase_key(NS_WIFI, KEY_WIFI_SSID);
    esp_err_t err2 = dc_storage_erase_key(NS_WIFI, KEY_WIFI_PASS);
    return (err != ESP_OK) ? err : err2;
}

esp_err_t dc_settings_get_device_name(char *out_buf, size_t out_buf_len)
{
    esp_err_t err = dc_storage_get_str(NS_SETTINGS, KEY_DEVICE_NAME, out_buf, out_buf_len);
    if (err != ESP_OK) {
        snprintf(out_buf, out_buf_len, "%s", DEFAULT_DEVICE_NAME);
        return ESP_OK; /* valor por omissão é uma resposta válida, não erro */
    }
    return ESP_OK;
}

esp_err_t dc_settings_set_device_name(const char *name)
{
    if (name == NULL || strlen(name) == 0 || strlen(name) > DC_SETTINGS_DEVICE_NAME_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    return dc_storage_set_str(NS_SETTINGS, KEY_DEVICE_NAME, name);
}

uint8_t dc_settings_get_volume(void)
{
    uint8_t v = DEFAULT_VOLUME;
    dc_storage_get_u8(NS_SETTINGS, KEY_VOLUME, &v);
    return (v > 100) ? 100 : v;
}

esp_err_t dc_settings_set_volume(uint8_t volume)
{
    if (volume > 100) {
        volume = 100;
    }
    return dc_storage_set_u8(NS_SETTINGS, KEY_VOLUME, volume);
}

uint8_t dc_settings_get_brightness(void)
{
    uint8_t v = DEFAULT_BRIGHTNESS;
    dc_storage_get_u8(NS_SETTINGS, KEY_BRIGHTNESS, &v);
    return (v > 100) ? 100 : v;
}

esp_err_t dc_settings_set_brightness(uint8_t brightness)
{
    if (brightness > 100) {
        brightness = 100;
    }
    return dc_storage_set_u8(NS_SETTINGS, KEY_BRIGHTNESS, brightness);
}

dc_theme_t dc_settings_get_theme(void)
{
    uint8_t v = (uint8_t)DC_THEME_DARK_PURPLE;
    dc_storage_get_u8(NS_SETTINGS, KEY_THEME, &v);
    return (dc_theme_t)v;
}

esp_err_t dc_settings_set_theme(dc_theme_t theme)
{
    return dc_storage_set_u8(NS_SETTINGS, KEY_THEME, (uint8_t)theme);
}

esp_err_t dc_settings_get_favorites(char out_ids[DC_SETTINGS_MAX_FAVORITES][16], uint8_t *out_count)
{
    uint8_t count = 0;
    dc_storage_get_u8(NS_SETTINGS, KEY_FAV_COUNT, &count);
    if (count > DC_SETTINGS_MAX_FAVORITES) {
        count = DC_SETTINGS_MAX_FAVORITES;
    }
    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%u", KEY_FAV_PREFIX, i);
        if (dc_storage_get_str(NS_SETTINGS, key, out_ids[i], 16) != ESP_OK) {
            out_ids[i][0] = '\0';
        }
    }
    *out_count = count;
    return ESP_OK;
}

esp_err_t dc_settings_set_favorites(const char ids[DC_SETTINGS_MAX_FAVORITES][16], uint8_t count)
{
    if (count > DC_SETTINGS_MAX_FAVORITES) {
        count = DC_SETTINGS_MAX_FAVORITES;
    }
    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%u", KEY_FAV_PREFIX, i);
        esp_err_t err = dc_storage_set_str(NS_SETTINGS, key, ids[i]);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Falha a guardar favorito %u: %s", i, esp_err_to_name(err));
        }
    }
    return dc_storage_set_u8(NS_SETTINGS, KEY_FAV_COUNT, count);
}
