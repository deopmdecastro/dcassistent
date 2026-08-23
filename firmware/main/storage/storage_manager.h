/**
 * @file storage_manager.h
 * @brief Camada mínima sobre a NVS. Ninguém fora de storage/ deve incluir
 * "nvs_flash.h" ou "nvs.h" diretamente — ver docs/firmware-architecture.md.
 *
 * Namespaces usados (mantidos aqui para evitar colisões espalhadas pelo código):
 *   "dc_wifi"     -> credenciais Wi-Fi (settings_manager)
 *   "dc_settings" -> preferências gerais (settings_manager)
 */
#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Inicializa a NVS flash (chamado uma única vez em app_main, antes de tudo). */
esp_err_t dc_storage_manager_init(void);

/** @brief Escreve uma string. @return ESP_OK ou erro NVS. */
esp_err_t dc_storage_set_str(const char *ns, const char *key, const char *value);

/**
 * @brief Lê uma string para out_buf (tamanho out_buf_len, incluindo terminador).
 * @return ESP_OK, ESP_ERR_NVS_NOT_FOUND se a chave não existir, ou outro erro NVS.
 */
esp_err_t dc_storage_get_str(const char *ns, const char *key, char *out_buf, size_t out_buf_len);

esp_err_t dc_storage_set_u8(const char *ns, const char *key, uint8_t value);
esp_err_t dc_storage_get_u8(const char *ns, const char *key, uint8_t *out_value);

esp_err_t dc_storage_set_bool(const char *ns, const char *key, bool value);
esp_err_t dc_storage_get_bool(const char *ns, const char *key, bool *out_value);

/** @brief Apaga uma chave específica (ex.: "esquecer rede Wi-Fi"). Não falha se não existir. */
esp_err_t dc_storage_erase_key(const char *ns, const char *key);

/** @brief Apaga todo um namespace (usar com cuidado — reset de fábrica de uma secção). */
esp_err_t dc_storage_erase_namespace(const char *ns);

#ifdef __cplusplus
}
#endif
