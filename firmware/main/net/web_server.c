/**
 * @file web_server.c
 * @brief Implementação do HTTP + WebSocket server descrito em web_server.h.
 *
 * Nota importante: para manter o firmware pequeno servimos os assets do
 * `frontend-preview` a partir de uma partição SPIFFS gravada em flash
 * (label "dcweb"). Isto evita ter de embeber o HTML no binário e permite
 * atualizar a interface sem recompilar todo o firmware (basta um
 * `idf.py spiffs_dcweb-flash`).
 */
#include "net/web_server.h"
#include "app_config.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char *TAG = "dc_web_server";

#define DC_WEB_BASE_PATH        "/dcweb"
#define DC_WEB_MAX_URI_LEN      256
#define DC_WEB_SCRATCH_BUF_LEN  2048
#define DC_WEB_MAX_WS_CLIENTS   4

typedef struct {
    httpd_handle_t server;
    int ws_fds[DC_WEB_MAX_WS_CLIENTS];
    bool spiffs_mounted;
} dc_web_ctx_t;

static dc_web_ctx_t s_ctx = { 0 };

/* -------------------------------------------------------------- Helpers */

static const char *mime_for(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot) return "text/plain";
    if (!strcasecmp(dot, ".html") || !strcasecmp(dot, ".htm")) return "text/html; charset=utf-8";
    if (!strcasecmp(dot, ".css"))  return "text/css; charset=utf-8";
    if (!strcasecmp(dot, ".js"))   return "application/javascript; charset=utf-8";
    if (!strcasecmp(dot, ".json")) return "application/json; charset=utf-8";
    if (!strcasecmp(dot, ".svg"))  return "image/svg+xml";
    if (!strcasecmp(dot, ".png"))  return "image/png";
    if (!strcasecmp(dot, ".jpg") || !strcasecmp(dot, ".jpeg")) return "image/jpeg";
    if (!strcasecmp(dot, ".ico"))  return "image/x-icon";
    if (!strcasecmp(dot, ".woff2"))return "font/woff2";
    return "application/octet-stream";
}

static esp_err_t mount_spiffs(void)
{
    if (s_ctx.spiffs_mounted) return ESP_OK;
    esp_vfs_spiffs_conf_t conf = {
        .base_path = DC_WEB_BASE_PATH,
        .partition_label = "dcweb",
        .max_files = 6,
        .format_if_mount_failed = false,
    };
    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SPIFFS 'dcweb' nao montada (%s) — a UI web nao vai ficar disponivel", esp_err_to_name(err));
        return err;
    }
    size_t total = 0, used = 0;
    esp_spiffs_info("dcweb", &total, &used);
    ESP_LOGI(TAG, "SPIFFS 'dcweb' montada em %s (usado %u / %u bytes)", DC_WEB_BASE_PATH, (unsigned)used, (unsigned)total);
    s_ctx.spiffs_mounted = true;
    return ESP_OK;
}

/* -------------------------------------------------------------- Handlers */

static esp_err_t serve_file(httpd_req_t *req, const char *path)
{
    char fs_path[128];
    if (strcmp(path, "/") == 0) path = "/index.html";
    snprintf(fs_path, sizeof(fs_path), "%s%s", DC_WEB_BASE_PATH, path);

    int fd = open(fs_path, O_RDONLY, 0);
    if (fd < 0) {
        ESP_LOGW(TAG, "404: %s", fs_path);
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, "DC OS: recurso nao encontrado no SPIFFS.");
        return ESP_OK;
    }

    httpd_resp_set_type(req, mime_for(fs_path));
    /* cache curta para poder atualizar via OTA-web */
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=60");

    static char buf[DC_WEB_SCRATCH_BUF_LEN];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
            close(fd);
            httpd_resp_send_chunk(req, NULL, 0);
            return ESP_FAIL;
        }
    }
    close(fd);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t http_get_handler(httpd_req_t *req)
{
    return serve_file(req, req->uri);
}

/* WebSocket -------------------------------------------------------------- */

static void ws_register_fd(int fd)
{
    for (int i = 0; i < DC_WEB_MAX_WS_CLIENTS; i++) {
        if (s_ctx.ws_fds[i] == fd) return;
    }
    for (int i = 0; i < DC_WEB_MAX_WS_CLIENTS; i++) {
        if (s_ctx.ws_fds[i] == 0) { s_ctx.ws_fds[i] = fd; return; }
    }
    ESP_LOGW(TAG, "Sem slots WS livres — cliente rejeitado (fd=%d)", fd);
}

static void ws_forget_fd(int fd)
{
    for (int i = 0; i < DC_WEB_MAX_WS_CLIENTS; i++) {
        if (s_ctx.ws_fds[i] == fd) s_ctx.ws_fds[i] = 0;
    }
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ws_register_fd(httpd_req_to_sockfd(req));
        ESP_LOGI(TAG, "Novo cliente WS (fd=%d)", httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    httpd_ws_frame_t frame = { 0 };
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
    if (err != ESP_OK) return err;

    if (frame.len && frame.len < 1024) {
        uint8_t *buf = calloc(1, frame.len + 1);
        if (!buf) return ESP_ERR_NO_MEM;
        frame.payload = buf;
        err = httpd_ws_recv_frame(req, &frame, frame.len);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "WS <- %.*s", (int)frame.len, (char *)buf);
            /* TODO fase 0.4: encaminhar comandos JSON para o event loop. */
        }
        free(buf);
    }
    return err;
}

/* Broadcast (thread-safe via httpd work-queue) ---------------------------- */

typedef struct {
    char *json;
} bcast_ctx_t;

static void bcast_work(void *arg)
{
    bcast_ctx_t *c = (bcast_ctx_t *)arg;
    if (!c || !c->json || !s_ctx.server) { free(c ? c->json : NULL); free(c); return; }

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)c->json,
        .len = strlen(c->json),
        .final = true,
    };
    for (int i = 0; i < DC_WEB_MAX_WS_CLIENTS; i++) {
        int fd = s_ctx.ws_fds[i];
        if (!fd) continue;
        esp_err_t err = httpd_ws_send_frame_async(s_ctx.server, fd, &frame);
        if (err != ESP_OK) {
            ESP_LOGD(TAG, "Cliente WS (fd=%d) ca'iu (%s)", fd, esp_err_to_name(err));
            ws_forget_fd(fd);
        }
    }
    free(c->json);
    free(c);
}

esp_err_t dc_web_server_broadcast_json(const char *json)
{
    if (!s_ctx.server || !json) return ESP_ERR_INVALID_STATE;
    bcast_ctx_t *c = calloc(1, sizeof(*c));
    if (!c) return ESP_ERR_NO_MEM;
    c->json = strdup(json);
    if (!c->json) { free(c); return ESP_ERR_NO_MEM; }
    return httpd_queue_work(s_ctx.server, bcast_work, c);
}

/* Ligar/desligar em função do IP -----------------------------------------  */

static esp_err_t start_httpd(void)
{
    if (s_ctx.server) return ESP_OK;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size      = 6144;
    cfg.max_uri_handlers = 8;
    cfg.max_open_sockets = DC_WEB_MAX_WS_CLIENTS + 3;
    cfg.uri_match_fn    = httpd_uri_match_wildcard;
    cfg.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_ctx.server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start falhou: %s", esp_err_to_name(err));
        s_ctx.server = NULL;
        return err;
    }

    static const httpd_uri_t uri_ws = {
        .uri = "/ws", .method = HTTP_GET, .handler = ws_handler,
        .user_ctx = NULL, .is_websocket = true,
    };
    httpd_register_uri_handler(s_ctx.server, &uri_ws);

    static const httpd_uri_t uri_root = {
        .uri = "/*", .method = HTTP_GET, .handler = http_get_handler, .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_ctx.server, &uri_root);

    ESP_LOGI(TAG, "HTTP server no ar (porta %u)", cfg.server_port);
    return ESP_OK;
}

static void stop_httpd(void)
{
    if (!s_ctx.server) return;
    httpd_stop(s_ctx.server);
    s_ctx.server = NULL;
    memset(s_ctx.ws_fds, 0, sizeof(s_ctx.ws_fds));
    ESP_LOGI(TAG, "HTTP server parado");
}

esp_err_t dc_web_server_init(void)
{
    mount_spiffs();   /* falha "soft" — o firmware continua sem UI web */
    return ESP_OK;
}

void dc_web_server_notify_ip(bool has_ip, esp_ip4_addr_t ip)
{
    if (has_ip) {
        if (start_httpd() == ESP_OK) {
            ESP_LOGI(TAG, "DC OS web disponivel em http://" IPSTR "/", IP2STR(&ip));
        }
    } else {
        stop_httpd();
    }
}
