/**
 * @file web_server.h
 * @brief HTTP + WebSocket server que serve a interface DC OS a partir do ESP32.
 *
 * Papel:
 *   - HTTP: serve os assets estáticos (index.html + tudo o que vier no SPIFFS
 *     `dcweb`) a partir de raiz "/", com Content-Type inferido pela extensão.
 *   - WebSocket em "/ws": permite ao browser sincronizar estado com o firmware
 *     (bateria, Wi-Fi, brilho, volume, estados da UI, comandos remotos).
 *
 * O servidor arranca automaticamente quando o Wi-Fi passa a estado
 * DC_WIFI_STATE_CONNECTED (via `dc_web_server_notify_ip`). Se o Wi-Fi cair,
 * pára-se a instância; volta a subir na próxima conexão.
 *
 * Dependência de camada (docs/firmware-architecture.md §2):
 *   web_server -> esp_http_server (idf) -> lwIP -> wifi_manager
 *   web_server publica eventos para o resto do firmware via callbacks; nunca
 *   toca em LVGL diretamente (envia notificações pelo ui_manager).
 */
#pragma once

#include "esp_err.h"
#include "esp_netif_ip_addr.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o subsistema. Deve ser chamado uma vez no arranque, depois
 *        do storage_manager (para ler settings persistidos no futuro). Não liga
 *        ainda o httpd — só o faz quando houver IP.
 */
esp_err_t dc_web_server_init(void);

/** @brief Avisa o web server que o IP mudou (nova ligação Wi-Fi). */
void dc_web_server_notify_ip(bool has_ip, esp_ip4_addr_t ip);

/**
 * @brief Envia (broadcast) um payload JSON a todos os clientes WebSocket
 *        ligados. Usado pelo ui/audio/wifi manager para propagar estado.
 *        Chamada thread-safe (usa work-queue interna do httpd).
 */
esp_err_t dc_web_server_broadcast_json(const char *json);

#ifdef __cplusplus
}
#endif
