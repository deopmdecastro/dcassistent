/**
 * @file wifi_manager.h
 * @brief Gestor de Wi-Fi não-bloqueante (STA). Guarda/lê credenciais via
 * settings_manager (NVS), liga automaticamente à última rede conhecida e
 * gere reconexão. Comunica o estado para a UI por callback — nada aqui
 * chama lv_* diretamente (ver docs/firmware-architecture.md secção 2).
 */
#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DC_WIFI_STATE_OFF = 0,
    DC_WIFI_STATE_CONNECTING,
    DC_WIFI_STATE_CONNECTED,
    DC_WIFI_STATE_NO_NETWORK,   /* sem credenciais guardadas */
    DC_WIFI_STATE_ERROR,        /* falhou a autenticar/ligar após as tentativas configuradas */
} dc_wifi_state_t;

typedef void (*dc_wifi_state_cb_t)(dc_wifi_state_t state);

/**
 * @brief Inicializa o stack Wi-Fi em modo STA e arranca a tentativa de
 * ligação à última rede guardada (se existir). Não bloqueia.
 * @param state_cb Callback opcional (pode ser NULL) chamado a cada mudança
 *                  de estado — tipicamente ligado a dc_ui_notify_wifi_connected.
 */
esp_err_t dc_wifi_manager_start(dc_wifi_state_cb_t state_cb);

/**
 * @brief Guarda novas credenciais e tenta ligar-se imediatamente.
 * Não bloqueia; o resultado chega pelo callback de estado.
 */
esp_err_t dc_wifi_manager_connect(const char *ssid, const char *password);

/** @brief Desliga, apaga as credenciais guardadas e volta a DC_WIFI_STATE_NO_NETWORK. */
esp_err_t dc_wifi_manager_forget(void);

/** @return o último estado conhecido (não bloqueia nem faz I/O). */
dc_wifi_state_t dc_wifi_manager_get_state(void);

/** @return true se atualmente ligado com IP atribuído. */
bool dc_wifi_manager_is_connected(void);

#ifdef __cplusplus
}
#endif
