/**
 * @file storage_manager.c
 * @brief Implementação da camada NVS. Abre/fecha um handle por operação —
 * simples e seguro para a frequência de escrita esperada (configurações,
 * não dados de alta cadência). Se isso mudar, considerar cache em RAM.
 */
#include "storage_manager.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "dc_storage";

esp_err_t dc_storage_manager_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS sem espaço/versão antiga — a apagar e reinicializar");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t dc_storage_open(const char *ns, nvs_open_mode_t mode, nvs_handle_t *out_handle)
{
    esp_err_t err = nvs_open(ns, mode, out_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open('%s') falhou: %s", ns, esp_err_to_name(err));
    }
    return err;
}

esp_err_t dc_storage_set_str(const char *ns, const char *key, const char *value)
{
    nvs_handle_t h;
    esp_err_t err = dc_storage_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_str(h, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t dc_storage_get_str(const char *ns, const char *key, char *out_buf, size_t out_buf_len)
{
    nvs_handle_t h;
    esp_err_t err = dc_storage_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_str(h, key, out_buf, &out_buf_len);
    nvs_close(h);
    return err;
}

esp_err_t dc_storage_set_u8(const char *ns, const char *key, uint8_t value)
{
    nvs_handle_t h;
    esp_err_t err = dc_storage_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_u8(h, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t dc_storage_get_u8(const char *ns, const char *key, uint8_t *out_value)
{
    nvs_handle_t h;
    esp_err_t err = dc_storage_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_u8(h, key, out_value);
    nvs_close(h);
    return err;
}

esp_err_t dc_storage_set_bool(const char *ns, const char *key, bool value)
{
    return dc_storage_set_u8(ns, key, value ? 1 : 0);
}

esp_err_t dc_storage_get_bool(const char *ns, const char *key, bool *out_value)
{
    uint8_t raw = 0;
    esp_err_t err = dc_storage_get_u8(ns, key, &raw);
    if (err == ESP_OK) {
        *out_value = (raw != 0);
    }
    return err;
}

esp_err_t dc_storage_erase_key(const char *ns, const char *key)
{
    nvs_handle_t h;
    esp_err_t err = dc_storage_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_erase_key(h, key);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = ESP_OK; /* já não existia — não é erro para quem chama */
    }
    nvs_close(h);
    return err;
}

esp_err_t dc_storage_erase_namespace(const char *ns)
{
    nvs_handle_t h;
    esp_err_t err = dc_storage_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_erase_all(h);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}
