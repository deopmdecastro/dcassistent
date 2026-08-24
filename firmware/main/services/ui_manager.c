/**
 * @file ui_manager.c
 * @brief Interface LVGL da DC — reorganizada como SISTEMA OPERATIVO.
 *
 * A DC deixou de ser um dashboard com telas numeradas. A primeira tela é o
 * LAUNCHER (ecrã principal do sistema) com uma grelha de APLICAÇÕES. Cada
 * aplicação tem o seu próprio ambiente e o seu botão "voltar" para o launcher.
 *
 * Aplicações (App Registry — ver docs/interface-os.md):
 *   DC Assistant · Controlo · Monitorização · Alarmes · Agenda · Música ·
 *   Loja · Definições
 *
 * Navegação: Sistema Operativo → Aplicações → Funções dentro de cada aplicação.
 * Sem navegação por números de tela. O botão BOOT mantém-se como input
 * secundário/recovery (curto = próximo ícone, longo = abrir aplicação).
 */
#include "ui_manager.h"
#include "app_config.h"
#include "hal/lcd_hal.h"
#include "hal/gpio_hal.h"
#include "hal/touch_hal.h"
#include "storage/settings_manager.h"
#include "audio/audio_manager.h"

#include "esp_lvgl_port.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

static const char *TAG = "dc_ui_manager";

/* ------------------------------------------------------------------------ */
/* Paleta / tema (design visual mantido do mockup — ver docs/interface-os.md) */
/* ------------------------------------------------------------------------ */
#define DC_COLOR_BG        lv_color_hex(0x0B0F14)
#define DC_COLOR_CARD      lv_color_hex(0x161C24)
#define DC_COLOR_CARD_ACTIVE lv_color_hex(0x232B36) /* feedback de "pressed" em toque */
#define DC_COLOR_ACCENT    lv_color_hex(0x36D6C0)  /* teal/verde — cor de marca */
#define DC_COLOR_TEXT      lv_color_hex(0xE8EEF3)
#define DC_COLOR_TEXT_DIM  lv_color_hex(0x7C8A99)

/* ------------------------------------------------------------------------ */
/* App Registry — fonte de verdade das aplicações do sistema.               */
/* Cada aplicação tem id, nome, ícone e o ecrã que a constrói.              */
/* ------------------------------------------------------------------------ */
typedef enum {
    DC_APP_ASSISTANT = 0,   /* DC Assistant — assistente inteligente */
    DC_APP_CONTROLO,        /* Controlo — comandos do sistema */
    DC_APP_MONITORIZACAO,   /* Monitorização — tempo real e histórico */
    DC_APP_ALARMES,         /* Alarmes — central de alarmes e eventos */
    DC_APP_AGENDA,          /* Agenda — tarefas e eventos */
    DC_APP_MUSICA,          /* Música — entretenimento / Spotify */
    DC_APP_LOJA,            /* Loja — App Store do sistema */
    DC_APP_DEFINICOES,      /* Definições — globais do sistema */
    DC_APP_COUNT,
} dc_app_id_t;

typedef struct {
    dc_app_id_t id;
    const char *name;
    const char *icon;   /* LV_SYMBOL_* */
} dc_app_t;

static const dc_app_t s_apps[DC_APP_COUNT] = {
    { DC_APP_ASSISTANT,     "DC Assistant",  LV_SYMBOL_AUDIO },
    { DC_APP_CONTROLO,      "Controlo",      LV_SYMBOL_PLAY },
    { DC_APP_MONITORIZACAO, "Monitorizacao", LV_SYMBOL_REFRESH },
    { DC_APP_ALARMES,       "Alarmes",       LV_SYMBOL_WARNING },
    { DC_APP_AGENDA,        "Agenda",        LV_SYMBOL_LIST },
    { DC_APP_MUSICA,        "Musica",        LV_SYMBOL_PAUSE },
    { DC_APP_LOJA,          "Loja",          LV_SYMBOL_DOWNLOAD },
    { DC_APP_DEFINICOES,    "Definicoes",    LV_SYMBOL_SETTINGS },
};

/* ------------------------------------------------------------------------ */
/* Estado dos widgets partilhados                                           */
/* ------------------------------------------------------------------------ */
static lv_obj_t *s_scr_launcher    = NULL;
static lv_obj_t *s_lbl_clock       = NULL;
static lv_obj_t *s_lbl_wifi_icon   = NULL;
static lv_obj_t *s_lbl_battery     = NULL;
static lv_obj_t *s_avatar_circle   = NULL;
static lv_obj_t *s_lbl_greeting    = NULL;
static lv_obj_t *s_app_items[DC_APP_COUNT];

/* Ambientes das aplicações */
static lv_obj_t *s_scr_assistant    = NULL;
static lv_obj_t *s_lbl_voice_status = NULL;
static lv_obj_t *s_scr_musica       = NULL;
static lv_obj_t *s_scr_definicoes   = NULL;
static lv_obj_t *s_lbl_brightness   = NULL;
static lv_obj_t *s_bar_brightness   = NULL;
static lv_obj_t *s_scr_placeholder  = NULL;
static lv_obj_t *s_lbl_placeholder_title = NULL;

static dc_app_id_t s_current_app = DC_APP_ASSISTANT;
static int s_app_focus_idx = 0;
static uint8_t s_brightness_pct = 80; /* espelha o valor inicial definido em lcd_hal */

/* ------------------------------------------------------------------------ */
/* Protótipos                                                               */
/* ------------------------------------------------------------------------ */
static void dc_ui_open_app(dc_app_id_t app);
static void dc_ui_go_launcher(void);
static void dc_ui_show_placeholder(const char *title);
static void dc_ui_highlight_app(int idx);

/* ------------------------------------------------------------------------ */
/* Construção do ícone de aplicação no launcher                            */
/* ------------------------------------------------------------------------ */
static lv_obj_t *dc_ui_build_app_icon(lv_obj_t *parent, const dc_app_t *app)
{
    lv_obj_t *icon = lv_obj_create(parent);
    lv_obj_set_size(icon, 50, 50);
    lv_obj_set_style_radius(icon, 14, 0);
    lv_obj_set_style_bg_color(icon, DC_COLOR_CARD, 0);
    lv_obj_set_style_bg_color(icon, DC_COLOR_CARD_ACTIVE, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(icon, 2, 0);
    lv_obj_set_style_border_color(icon, DC_COLOR_CARD, 0);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl_icon = lv_label_create(icon);
    lv_label_set_text(lbl_icon, app->icon);
    lv_obj_set_style_text_color(lbl_icon, DC_COLOR_ACCENT, 0);
    lv_obj_center(lbl_icon);

    lv_obj_t *lbl_name = lv_label_create(parent);
    lv_label_set_text(lbl_name, app->name);
    lv_obj_set_style_text_color(lbl_name, DC_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl_name, 54);
    lv_obj_align_to(lbl_name, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

    return icon;
}

/** @brief Callback de toque num ícone do launcher — abre a aplicação. */
static void dc_ui_app_icon_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    s_app_focus_idx = idx;
    dc_ui_highlight_app(idx);
    dc_ui_open_app((dc_app_id_t)idx);
}

/** @brief Callback de toque no botão "Voltar" (regressa ao launcher). */
static void dc_ui_back_button_click_cb(lv_event_t *e)
{
    (void)e;
    dc_ui_go_launcher();
}

/** @brief Incrementa o brilho em 20% (com wrap 100%->20%). */
static void dc_ui_increment_brightness(void)
{
    s_brightness_pct = (s_brightness_pct >= 100) ? 20 : s_brightness_pct + 20;
    dc_lcd_hal_set_brightness(s_brightness_pct);
    dc_settings_set_brightness(s_brightness_pct);
    lv_label_set_text_fmt(s_lbl_brightness, "Brilho: %d%% (toca para +20%%)", s_brightness_pct);
    lv_bar_set_value(s_bar_brightness, s_brightness_pct, LV_ANIM_ON);
}

static void dc_ui_brightness_bar_click_cb(lv_event_t *e)
{
    (void)e;
    dc_ui_increment_brightness();
}

static void dc_ui_highlight_app(int idx)
{
    for (int i = 0; i < DC_APP_COUNT; i++) {
        lv_color_t border = (i == idx) ? DC_COLOR_ACCENT : DC_COLOR_CARD;
        lv_obj_set_style_border_color(s_app_items[i], border, 0);
    }
}

/* ------------------------------------------------------------------------ */
/* LAUNCHER — ecrã principal do sistema (substitui o antigo Home + Apps)   */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_launcher(void)
{
    s_scr_launcher = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_launcher, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_launcher, LV_OBJ_FLAG_SCROLLABLE);

    /* --- Barra de topo: relógio, wifi, bateria --- */
    s_lbl_clock = lv_label_create(s_scr_launcher);
    lv_label_set_text(s_lbl_clock, "--:--");
    lv_obj_set_style_text_color(s_lbl_clock, DC_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_clock, LV_ALIGN_TOP_LEFT, 8, 6);

    s_lbl_battery = lv_label_create(s_scr_launcher);
    lv_label_set_text(s_lbl_battery, LV_SYMBOL_BATTERY_FULL " --%");
    lv_obj_set_style_text_color(s_lbl_battery, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_battery, LV_ALIGN_TOP_RIGHT, -8, 6);

    s_lbl_wifi_icon = lv_label_create(s_scr_launcher);
    lv_label_set_text(s_lbl_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(s_lbl_wifi_icon, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(s_lbl_wifi_icon, s_lbl_battery, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    /* --- Avatar central da DC (círculo) + saudação --- */
    s_avatar_circle = lv_obj_create(s_scr_launcher);
    lv_obj_set_size(s_avatar_circle, 64, 64);
    lv_obj_set_style_radius(s_avatar_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_avatar_circle, DC_COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(s_avatar_circle, 0, 0);
    lv_obj_clear_flag(s_avatar_circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(s_avatar_circle, LV_ALIGN_TOP_MID, 0, 26);

    lv_obj_t *lbl_dc = lv_label_create(s_avatar_circle);
    lv_label_set_text(lbl_dc, "DC");
    lv_obj_set_style_text_color(lbl_dc, DC_COLOR_BG, 0);
    lv_obj_center(lbl_dc);

    s_lbl_greeting = lv_label_create(s_scr_launcher);
    lv_label_set_text(s_lbl_greeting, "Ola! Sou a DC");
    lv_obj_set_style_text_color(s_lbl_greeting, DC_COLOR_TEXT, 0);
    lv_obj_align_to(s_lbl_greeting, s_avatar_circle, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* --- Grelha de aplicações (4 colunas x 2 linhas) --- */
    lv_obj_t *grid = lv_obj_create(s_scr_launcher);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, DC_LCD_H_RES - 12, 150);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY,
                           LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < DC_APP_COUNT; i++) {
        s_app_items[i] = dc_ui_build_app_icon(grid, &s_apps[i]);
        lv_obj_add_event_cb(s_app_items[i], dc_ui_app_icon_click_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
    dc_ui_highlight_app(s_app_focus_idx);

    lv_scr_load(s_scr_launcher);
}

/* ------------------------------------------------------------------------ */
/* Cabeçalho genérico dos ambientes de aplicação: voltar + título           */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_back_header(lv_obj_t *screen, const char *title, lv_obj_t **out_title_lbl)
{
    lv_obj_t *btn_back = lv_button_create(screen);
    lv_obj_set_size(btn_back, 36, 28);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_obj_set_style_bg_color(btn_back, DC_COLOR_CARD, 0);
    lv_obj_set_style_bg_color(btn_back, DC_COLOR_CARD_ACTIVE, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, dc_ui_back_button_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_back_icon = lv_label_create(btn_back);
    lv_label_set_text(lbl_back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lbl_back_icon, DC_COLOR_TEXT, 0);
    lv_obj_center(lbl_back_icon);

    lv_obj_t *lbl_title = lv_label_create(screen);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, DC_COLOR_TEXT, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);
    if (out_title_lbl) {
        *out_title_lbl = lbl_title;
    }

    lv_obj_t *lbl_hint = lv_label_create(screen);
    lv_label_set_text(lbl_hint, "Toque em " LV_SYMBOL_LEFT " ou toque longo no BOOT para voltar");
    lv_obj_set_style_text_color(lbl_hint, DC_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, -8);
}

/* ------------------------------------------------------------------------ */
/* App Música — placeholder até a fase 0.5 (Spotify) estar ligada           */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_musica_screen(void)
{
    s_scr_musica = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_musica, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_musica, LV_OBJ_FLAG_SCROLLABLE);

    dc_ui_build_back_header(s_scr_musica, "Musica", NULL);

    lv_obj_t *cover = lv_obj_create(s_scr_musica);
    lv_obj_set_size(cover, 100, 100);
    lv_obj_set_style_radius(cover, 10, 0);
    lv_obj_set_style_bg_color(cover, DC_COLOR_CARD, 0);
    lv_obj_set_style_border_width(cover, 0, 0);
    lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cover, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_t *lbl_note = lv_label_create(cover);
    lv_label_set_text(lbl_note, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(lbl_note, DC_COLOR_TEXT_DIM, 0);
    lv_obj_center(lbl_note);

    lv_obj_t *lbl_track = lv_label_create(s_scr_musica);
    lv_label_set_text(lbl_track, "Sem musica a tocar");
    lv_obj_set_style_text_color(lbl_track, DC_COLOR_TEXT, 0);
    lv_obj_align_to(lbl_track, cover, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);

    lv_obj_t *lbl_artist = lv_label_create(s_scr_musica);
    lv_label_set_text(lbl_artist, "Liga o Spotify no Gateway (fase 0.5)");
    lv_obj_set_style_text_color(lbl_artist, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(lbl_artist, lbl_track, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    lv_obj_t *transport = lv_label_create(s_scr_musica);
    lv_label_set_text(transport, LV_SYMBOL_PREV "   " LV_SYMBOL_PAUSE "   " LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(transport, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align(transport, LV_ALIGN_BOTTOM_MID, 0, -34);
}

/* ------------------------------------------------------------------------ */
/* App Definições — globais do sistema (brilho já é funcional)             */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_definicoes_screen(void)
{
    s_scr_definicoes = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_definicoes, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_definicoes, LV_OBJ_FLAG_SCROLLABLE);

    dc_ui_build_back_header(s_scr_definicoes, "Definicoes", NULL);

    s_lbl_brightness = lv_label_create(s_scr_definicoes);
    lv_label_set_text_fmt(s_lbl_brightness, "Brilho: %d%% (toca para +20%%)", s_brightness_pct);
    lv_obj_set_style_text_color(s_lbl_brightness, DC_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_brightness, LV_ALIGN_CENTER, 0, -30);

    s_bar_brightness = lv_bar_create(s_scr_definicoes);
    lv_obj_set_size(s_bar_brightness, DC_LCD_H_RES - 60, 14);
    lv_obj_align(s_bar_brightness, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(s_bar_brightness, 0, 100);
    lv_bar_set_value(s_bar_brightness, s_brightness_pct, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_brightness, DC_COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_brightness, DC_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_add_flag(s_bar_brightness, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_bar_brightness, dc_ui_brightness_bar_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_more = lv_label_create(s_scr_definicoes);
    lv_label_set_text(lbl_more, "Sistema, Rede, Seguranca e Hardware chegam na fase 0.3/0.4");
    lv_obj_set_style_text_color(lbl_more, DC_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_more, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_more, LV_ALIGN_CENTER, 0, 40);
}

/* ------------------------------------------------------------------------ */
/* App DC Assistant — assistente inteligente (estados visuais + botão mic)  */
/* DC 0.3: só a UI e o feedback sonoro estão ligados; NÃO existe wake-word/ */
/* STT real ainda (audio_input devolve ESP_ERR_NOT_SUPPORTED de propósito). */
/* ------------------------------------------------------------------------ */
static void dc_ui_assistant_mic_click_cb(lv_event_t *e)
{
    (void)e;
    dc_audio_manager_play_feedback(DC_AUDIO_FEEDBACK_TAP);
    lv_label_set_text(s_lbl_voice_status,
                       "Captura de voz ainda nao ligada\n(pendente para a fase 0.4)");
}

static void dc_ui_build_assistant_screen(void)
{
    s_scr_assistant = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_assistant, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_assistant, LV_OBJ_FLAG_SCROLLABLE);

    dc_ui_build_back_header(s_scr_assistant, "DC Assistant", NULL);

    lv_obj_t *ring = lv_obj_create(s_scr_assistant);
    lv_obj_set_size(ring, 140, 140);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_border_color(ring, DC_COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(ring, LV_OPA_40, 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *btn_mic = lv_button_create(s_scr_assistant);
    lv_obj_set_size(btn_mic, 84, 84);
    lv_obj_set_style_radius(btn_mic, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_mic, DC_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_color(btn_mic, DC_COLOR_CARD_ACTIVE, LV_STATE_PRESSED);
    lv_obj_align(btn_mic, LV_ALIGN_CENTER, 0, -10);
    lv_obj_add_event_cb(btn_mic, dc_ui_assistant_mic_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_mic_icon = lv_label_create(btn_mic);
    lv_label_set_text(lbl_mic_icon, LV_SYMBOL_AUDIO);
    lv_obj_center(lbl_mic_icon);

    s_lbl_voice_status = lv_label_create(s_scr_assistant);
    lv_label_set_text(s_lbl_voice_status, "Pronto para ouvir");
    lv_obj_set_style_text_color(s_lbl_voice_status, DC_COLOR_TEXT, 0);
    lv_obj_set_style_text_align(s_lbl_voice_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_lbl_voice_status, DC_LCD_H_RES - 40);
    lv_obj_align(s_lbl_voice_status, LV_ALIGN_BOTTOM_MID, 0, -28);
}

/* ------------------------------------------------------------------------ */
/* Ecrã placeholder genérico — apps ainda por implementar (Controlo,        */
/* Monitorização, Alarmes, Agenda, Loja). Mostra "Em breve" honestamente.  */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_placeholder_screen(void)
{
    s_scr_placeholder = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_placeholder, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_placeholder, LV_OBJ_FLAG_SCROLLABLE);

    dc_ui_build_back_header(s_scr_placeholder, "", &s_lbl_placeholder_title);

    lv_obj_t *lbl_soon = lv_label_create(s_scr_placeholder);
    lv_label_set_text(lbl_soon, "Em breve");
    lv_obj_set_style_text_color(lbl_soon, DC_COLOR_TEXT_DIM, 0);
    lv_obj_center(lbl_soon);
}

static void dc_ui_show_placeholder(const char *title)
{
    lv_label_set_text(s_lbl_placeholder_title, title);
    lv_scr_load(s_scr_placeholder);
}

/* ------------------------------------------------------------------------ */
/* Navegação: abre o ambiente da aplicação selecionada no launcher          */
/* ------------------------------------------------------------------------ */
static void dc_ui_open_app(dc_app_id_t app)
{
    s_current_app = app;
    switch (app) {
        case DC_APP_ASSISTANT:      /* DC Assistant — assistente inteligente */
            lv_label_set_text(s_lbl_voice_status, "Pronto para ouvir");
            lv_scr_load(s_scr_assistant);
            break;
        case DC_APP_MUSICA:         /* Música — Spotify */
            lv_scr_load(s_scr_musica);
            break;
        case DC_APP_DEFINICOES:     /* Definições — globais do sistema */
            lv_label_set_text_fmt(s_lbl_brightness, "Brilho: %d%% (toca para +20%%)",
                                   s_brightness_pct);
            lv_bar_set_value(s_bar_brightness, s_brightness_pct, LV_ANIM_OFF);
            lv_scr_load(s_scr_definicoes);
            break;
        case DC_APP_CONTROLO:       /* Controlo */
            dc_ui_show_placeholder("Controlo");
            break;
        case DC_APP_MONITORIZACAO:  /* Monitorização */
            dc_ui_show_placeholder("Monitorizacao");
            break;
        case DC_APP_ALARMES:        /* Alarmes */
            dc_ui_show_placeholder("Alarmes");
            break;
        case DC_APP_AGENDA:         /* Agenda */
            dc_ui_show_placeholder("Agenda");
            break;
        case DC_APP_LOJA:           /* Loja */
            dc_ui_show_placeholder("Loja");
            break;
        default:
            dc_ui_show_placeholder("App");
            break;
    }
}

/** @brief Volta ao launcher (ecrã principal do sistema). */
static void dc_ui_go_launcher(void)
{
    lv_scr_load(s_scr_launcher);
}

/* ------------------------------------------------------------------------ */
/* Notificações de outras tarefas (thread-safe via lvgl_port lock)          */
/* ------------------------------------------------------------------------ */
void dc_ui_notify_wifi_connected(bool connected)
{
    if (!s_lbl_wifi_icon || !lvgl_port_lock(0)) {
        return;
    }
    lv_obj_set_style_text_color(s_lbl_wifi_icon,
                                 connected ? DC_COLOR_ACCENT : DC_COLOR_TEXT_DIM, 0);
    lvgl_port_unlock();
}

void dc_ui_notify_battery(uint8_t percent)
{
    if (!s_lbl_battery || !lvgl_port_lock(0)) {
        return;
    }
    lv_label_set_text_fmt(s_lbl_battery, LV_SYMBOL_BATTERY_FULL " %d%%", percent);
    lvgl_port_unlock();
}

void dc_ui_notify_state(dc_ui_state_t state)
{
    if (!s_lbl_greeting || !lvgl_port_lock(0)) {
        return;
    }
    switch (state) {
        case DC_UI_STATE_LISTENING:
            lv_label_set_text(s_lbl_greeting, "A ouvir...");
            break;
        case DC_UI_STATE_THINKING:
            lv_label_set_text(s_lbl_greeting, "A pensar...");
            break;
        case DC_UI_STATE_SPEAKING:
            lv_label_set_text(s_lbl_greeting, "...");
            break;
        case DC_UI_STATE_IDLE:
        default:
            lv_label_set_text(s_lbl_greeting, "Ola! Sou a DC");
            break;
    }
    lvgl_port_unlock();
}

/* ------------------------------------------------------------------------ */
/* Tarefa da UI: lv_timer_handler + poll do botão (regra: só esta tarefa    */
/* chama lv_* diretamente, ver docs/firmware-architecture.md)               */
/* ------------------------------------------------------------------------ */
static void dc_ui_task(void *arg)
{
    (void)arg;
    uint32_t seconds = 0;

    while (1) {
        dc_btn_event_t ev = dc_gpio_hal_poll_button();

        if (ev != DC_BTN_EVENT_NONE && lvgl_port_lock(0)) {
            /* No launcher: curto = próximo ícone, longo = abrir aplicação */
            if (lv_disp_get_scr_act(NULL) == s_scr_launcher) {
                if (ev == DC_BTN_EVENT_SHORT_PRESS) {
                    s_app_focus_idx = (s_app_focus_idx + 1) % DC_APP_COUNT;
                    dc_ui_highlight_app(s_app_focus_idx);
                } else if (ev == DC_BTN_EVENT_LONG_PRESS) {
                    ESP_LOGI(TAG, "A abrir app: %s", s_apps[s_app_focus_idx].name);
                    dc_gpio_hal_set_led(0, 60, 255); /* feedback visual rápido no LED */
                    dc_ui_open_app((dc_app_id_t)s_app_focus_idx);
                }
            } else if (lv_disp_get_scr_act(NULL) == s_scr_definicoes) {
                if (ev == DC_BTN_EVENT_SHORT_PRESS) {
                    dc_ui_increment_brightness();
                } else if (ev == DC_BTN_EVENT_LONG_PRESS) {
                    dc_ui_go_launcher();
                }
            } else {
                /* Ambientes das restantes apps: só "voltar" está disponível por agora */
                if (ev == DC_BTN_EVENT_LONG_PRESS) {
                    dc_ui_go_launcher();
                }
            }
            lvgl_port_unlock();
        }

        /* Relógio placeholder — a fase 0.3 substitui por hora real (NTP/RTC) */
        if (++seconds % 50 == 0 && lvgl_port_lock(0)) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%02lu:%02lu",
                      (unsigned long)((seconds / 50 / 60) % 24),
                      (unsigned long)((seconds / 50) % 60));
            lv_label_set_text(s_lbl_clock, buf);
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t dc_ui_manager_start(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_RETURN_ON_ERROR(dc_lcd_hal_init(&panel, &io_handle), TAG, "lcd_hal_init");

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = io_handle,
        .panel_handle  = panel,
        .buffer_size   = DC_LCD_H_RES * DC_LVGL_BUF_LINES,
        .double_buffer = true,
        .hres          = DC_LCD_H_RES,
        .vres          = DC_LCD_V_RES,
        .flags = {
            .buff_spiram = true, /* frame buffers em PSRAM — ver secção 9 da arquitetura */
        },
    };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "Falha ao adicionar display ao lvgl_port");
        return ESP_FAIL;
    }

#if DC_BOARD_HAS_TOUCH
    esp_lcd_touch_handle_t touch_handle = NULL;
    esp_err_t touch_err = dc_touch_hal_init(&touch_handle);
    if (touch_err == ESP_OK) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = disp,
            .handle = touch_handle,
        };
        if (!lvgl_port_add_touch(&touch_cfg)) {
            ESP_LOGW(TAG, "lvgl_port_add_touch falhou — a UI continua utilizável pelo botão BOOT");
        }
    } else {
        ESP_LOGW(TAG, "Touch indisponível (%s) — a UI cai para navegação só por botão BOOT",
                 esp_err_to_name(touch_err));
    }
#else
    ESP_LOGI(TAG, "Build sem touch (DC_BOARD_HAS_TOUCH=0) — navegação só por botão BOOT");
#endif

    s_brightness_pct = dc_settings_get_brightness();
    dc_lcd_hal_set_brightness(s_brightness_pct);

    if (lvgl_port_lock(0)) {
        dc_ui_build_launcher();
        dc_ui_build_assistant_screen();
        dc_ui_build_musica_screen();
        dc_ui_build_definicoes_screen();
        dc_ui_build_placeholder_screen();
        lv_scr_load(s_scr_launcher); /* garante que arrancamos no launcher */
        lvgl_port_unlock();
    }

    xTaskCreatePinnedToCore(dc_ui_task, "dc_ui_task",
                             DC_TASK_UI_STACK_SIZE, NULL,
                             DC_TASK_UI_PRIORITY, NULL, DC_TASK_UI_CORE);

    ESP_LOGI(TAG, "UI Manager pronto — launcher carregado (%d apps, touch=%d, BOOT como fallback)",
             DC_APP_COUNT, dc_touch_hal_is_ready());
    return ESP_OK;
}
