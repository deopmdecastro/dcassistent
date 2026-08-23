/**
 * @file ui_manager.c
 * @brief Interface LVGL da DC — ecrã Home com relógio/wifi/bateria, avatar
 * central e menu (Música/Agenda/Chamadas/Definições). DC 0.3: navegação
 * principal por toque (ES3C28P, hal/touch_hal.h), com o botão BOOT mantido
 * como input secundário/recovery (ver hal/gpio_hal.h) — útil se o touch
 * falhar na inicialização.
 *
 * DC 0.2/0.3: interface navegável, sem ainda estar ligada ao Gateway.
 */
#include "ui_manager.h"
#include "app_config.h"
#include "hal/lcd_hal.h"
#include "hal/gpio_hal.h"
#include "hal/touch_hal.h"
#include "storage/settings_manager.h"

#include "esp_lvgl_port.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

static const char *TAG = "dc_ui_manager";

/* ------------------------------------------------------------------------ */
/* Paleta / tema simples (ver mockup em docs/arquitetura.md)                */
/* ------------------------------------------------------------------------ */
#define DC_COLOR_BG        lv_color_hex(0x0B0F14)
#define DC_COLOR_CARD      lv_color_hex(0x161C24)
#define DC_COLOR_CARD_ACTIVE lv_color_hex(0x232B36) /* feedback de "pressed" em toque */
#define DC_COLOR_ACCENT    lv_color_hex(0x36D6C0)  /* teal/verde — cor de marca do Manuel */
#define DC_COLOR_TEXT      lv_color_hex(0xE8EEF3)
#define DC_COLOR_TEXT_DIM  lv_color_hex(0x7C8A99)

/* ------------------------------------------------------------------------ */
/* Estado dos widgets partilhados                                           */
/* ------------------------------------------------------------------------ */
static lv_obj_t *s_scr_home       = NULL;
static lv_obj_t *s_lbl_clock      = NULL;
static lv_obj_t *s_lbl_wifi_icon  = NULL;
static lv_obj_t *s_lbl_battery    = NULL;
static lv_obj_t *s_avatar_circle  = NULL;
static lv_obj_t *s_lbl_greeting   = NULL;
static lv_obj_t *s_menu_items[4];

/* Sub-ecrãs (DC 0.2: Now Playing, Definições, e placeholder p/ Agenda/Chamadas) */
static lv_obj_t *s_scr_now_playing   = NULL;
static lv_obj_t *s_scr_settings      = NULL;
static lv_obj_t *s_lbl_brightness    = NULL;
static lv_obj_t *s_bar_brightness    = NULL;
static lv_obj_t *s_scr_placeholder   = NULL;
static lv_obj_t *s_lbl_placeholder_title = NULL;

typedef enum {
    DC_UI_SCREEN_HOME = 0,
    DC_UI_SCREEN_NOW_PLAYING,
    DC_UI_SCREEN_SETTINGS,
    DC_UI_SCREEN_PLACEHOLDER,
} dc_ui_screen_t;

static dc_ui_screen_t s_current_screen = DC_UI_SCREEN_HOME;
static uint8_t s_brightness_pct = 80; /* espelha o valor inicial definido em lcd_hal */

#define DC_MENU_COUNT 4
static const char *s_menu_labels[DC_MENU_COUNT] = {
    "Musica", "Agenda", "Chamadas", "Definicoes"
};
static int s_menu_focus_idx = 0;

/* ------------------------------------------------------------------------ */
/* Construção do ecrã Home                                                  */
/* ------------------------------------------------------------------------ */
static lv_obj_t *dc_ui_build_menu_item(lv_obj_t *parent, const char *label)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 100, 60);
    lv_obj_set_style_bg_color(card, DC_COLOR_CARD, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, DC_COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    /* Feedback visual de toque (DC 0.3 / ES3C28P) — o índice real é ligado
     * ao evento logo a seguir, em dc_ui_build_home_screen, quando já
     * conhecemos a posição i no array s_menu_items. */
    lv_obj_set_style_bg_color(card, DC_COLOR_CARD_ACTIVE, LV_STATE_PRESSED);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, DC_COLOR_TEXT, 0);
    lv_obj_center(lbl);

    return card;
}

static void dc_ui_open_menu_item(int idx);
static void dc_ui_go_home(void);

/**
 * @brief Callback de toque num card do menu Home. DC 0.3: com touch (ES3C28P)
 * um único tap já abre o item — o utilizador não precisa do gesto de dois
 * passos (curto=foco / longo=selecionar) que existia só para o botão BOOT.
 */
static void dc_ui_menu_item_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    s_menu_focus_idx = idx;
    dc_ui_highlight_menu(idx);
    dc_ui_open_menu_item(idx);
}

/** @brief Callback de toque no botão "Voltar" visível nos sub-ecrãs. */
static void dc_ui_back_button_click_cb(lv_event_t *e)
{
    (void)e;
    dc_ui_go_home();
}

/**
 * @brief Incrementa o brilho em 20% (com wrap 100%->20%) e reflete no LCD
 * e nos widgets do ecrã Definições. Partilhado entre o botão BOOT (short
 * press) e o tap direto na barra (touch, DC 0.3).
 */
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

static void dc_ui_highlight_menu(int idx)
{
    for (int i = 0; i < DC_MENU_COUNT; i++) {
        lv_color_t border = (i == idx) ? DC_COLOR_ACCENT : DC_COLOR_CARD;
        lv_obj_set_style_border_color(s_menu_items[i], border, 0);
    }
}

static void dc_ui_build_home_screen(void)
{
    s_scr_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_home, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_home, LV_OBJ_FLAG_SCROLLABLE);

    /* --- Barra de topo: relógio, wifi, bateria --- */
    s_lbl_clock = lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_clock, "--:--");
    lv_obj_set_style_text_color(s_lbl_clock, DC_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_clock, LV_ALIGN_TOP_LEFT, 8, 6);

    s_lbl_battery = lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_battery, LV_SYMBOL_BATTERY_FULL " --%");
    lv_obj_set_style_text_color(s_lbl_battery, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align(s_lbl_battery, LV_ALIGN_TOP_RIGHT, -8, 6);

    s_lbl_wifi_icon = lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(s_lbl_wifi_icon, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(s_lbl_wifi_icon, s_lbl_battery, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    /* --- Avatar central da DC (círculo) + saudação --- */
    s_avatar_circle = lv_obj_create(s_scr_home);
    lv_obj_set_size(s_avatar_circle, 70, 70);
    lv_obj_set_style_radius(s_avatar_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_avatar_circle, DC_COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(s_avatar_circle, 0, 0);
    lv_obj_clear_flag(s_avatar_circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(s_avatar_circle, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *lbl_dc = lv_label_create(s_avatar_circle);
    lv_label_set_text(lbl_dc, "DC");
    lv_obj_set_style_text_color(lbl_dc, DC_COLOR_BG, 0);
    lv_obj_center(lbl_dc);

    s_lbl_greeting = lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_greeting, "Ola! Sou a DC");
    lv_obj_set_style_text_color(s_lbl_greeting, DC_COLOR_TEXT, 0);
    lv_obj_align_to(s_lbl_greeting, s_avatar_circle, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);

    /* --- Grelha do menu: Musica / Agenda / Chamadas / Definicoes --- */
    lv_obj_t *grid = lv_obj_create(s_scr_home);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, DC_LCD_H_RES - 16, 140);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY,
                           LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < DC_MENU_COUNT; i++) {
        s_menu_items[i] = dc_ui_build_menu_item(grid, s_menu_labels[i]);
        lv_obj_add_event_cb(s_menu_items[i], dc_ui_menu_item_click_cb, LV_EVENT_CLICKED,
                             (void *)(intptr_t)i);
    }
    dc_ui_highlight_menu(s_menu_focus_idx);

    lv_scr_load(s_scr_home);
}

/* ------------------------------------------------------------------------ */
/* Cabeçalho genérico reutilizado nos sub-ecrãs: título + dica "Voltar"      */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_back_header(lv_obj_t *screen, const char *title, lv_obj_t **out_title_lbl)
{
    /* Botão de voltar tocável (DC 0.3/ES3C28P). O botão BOOT com toque longo
     * continua a funcionar como atalho físico, mas deixa de ser a única via. */
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
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, -8);
}

/* ------------------------------------------------------------------------ */
/* Ecrã "Now Playing" — placeholder até a fase 0.5 (Spotify) estar ligada   */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_now_playing_screen(void)
{
    s_scr_now_playing = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_now_playing, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_now_playing, LV_OBJ_FLAG_SCROLLABLE);

    dc_ui_build_back_header(s_scr_now_playing, "Musica", NULL);

    lv_obj_t *cover = lv_obj_create(s_scr_now_playing);
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

    lv_obj_t *lbl_track = lv_label_create(s_scr_now_playing);
    lv_label_set_text(lbl_track, "Sem musica a tocar");
    lv_obj_set_style_text_color(lbl_track, DC_COLOR_TEXT, 0);
    lv_obj_align_to(lbl_track, cover, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);

    lv_obj_t *lbl_artist = lv_label_create(s_scr_now_playing);
    lv_label_set_text(lbl_artist, "Liga o Spotify no Gateway (fase 0.5)");
    lv_obj_set_style_text_color(lbl_artist, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align_to(lbl_artist, lbl_track, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    lv_obj_t *transport = lv_label_create(s_scr_now_playing);
    lv_label_set_text(transport, LV_SYMBOL_PREV "   " LV_SYMBOL_PAUSE "   " LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(transport, DC_COLOR_TEXT_DIM, 0);
    lv_obj_align(transport, LV_ALIGN_BOTTOM_MID, 0, -34);
}

/* ------------------------------------------------------------------------ */
/* Ecrã "Definições" — brilho já é funcional (backlight PWM real)          */
/* ------------------------------------------------------------------------ */
static void dc_ui_build_settings_screen(void)
{
    s_scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_settings, DC_COLOR_BG, 0);
    lv_obj_clear_flag(s_scr_settings, LV_OBJ_FLAG_SCROLLABLE);

    dc_ui_build_back_header(s_scr_settings, "Definicoes", NULL);

    s_lbl_brightness = lv_label_create(s_scr_settings);
    lv_label_set_text_fmt(s_lbl_brightness, "Brilho: %d%% (toca para +20%%)", s_brightness_pct);
    lv_obj_set_style_text_color(s_lbl_brightness, DC_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_brightness, LV_ALIGN_CENTER, 0, -30);

    s_bar_brightness = lv_bar_create(s_scr_settings);
    lv_obj_set_size(s_bar_brightness, DC_LCD_H_RES - 60, 14);
    lv_obj_align(s_bar_brightness, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(s_bar_brightness, 0, 100);
    lv_bar_set_value(s_bar_brightness, s_brightness_pct, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_brightness, DC_COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_brightness, DC_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_add_flag(s_bar_brightness, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_bar_brightness, dc_ui_brightness_bar_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_more = lv_label_create(s_scr_settings);
    lv_label_set_text(lbl_more, "Wi-Fi, volume e idioma chegam na fase 0.3/0.4");
    lv_obj_set_style_text_color(lbl_more, DC_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_more, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_more, LV_ALIGN_CENTER, 0, 40);
}

/* ------------------------------------------------------------------------ */
/* Ecrã placeholder genérico — reutilizado para Agenda e Chamadas até       */
/* essas integrações existirem no Gateway (fases 0.6 e 0.7)                 */
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
    s_current_screen = DC_UI_SCREEN_PLACEHOLDER;
    lv_scr_load(s_scr_placeholder);
}

/* ------------------------------------------------------------------------ */
/* Navegação: sai do Home para o ecrã do item selecionado                   */
/* ------------------------------------------------------------------------ */
static void dc_ui_open_menu_item(int idx)
{
    switch (idx) {
        case 0: /* Musica */
            s_current_screen = DC_UI_SCREEN_NOW_PLAYING;
            lv_scr_load(s_scr_now_playing);
            break;
        case 1: /* Agenda */
            dc_ui_show_placeholder("Agenda");
            break;
        case 2: /* Chamadas */
            dc_ui_show_placeholder("Chamadas");
            break;
        case 3: /* Definicoes */
        default:
            lv_label_set_text_fmt(s_lbl_brightness, "Brilho: %d%% (toca para +20%%)",
                                   s_brightness_pct);
            lv_bar_set_value(s_bar_brightness, s_brightness_pct, LV_ANIM_OFF);
            s_current_screen = DC_UI_SCREEN_SETTINGS;
            lv_scr_load(s_scr_settings);
            break;
    }
}

static void dc_ui_go_home(void)
{
    s_current_screen = DC_UI_SCREEN_HOME;
    lv_scr_load(s_scr_home);
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
            if (s_current_screen == DC_UI_SCREEN_HOME) {
                if (ev == DC_BTN_EVENT_SHORT_PRESS) {
                    s_menu_focus_idx = (s_menu_focus_idx + 1) % DC_MENU_COUNT;
                    dc_ui_highlight_menu(s_menu_focus_idx);
                } else if (ev == DC_BTN_EVENT_LONG_PRESS) {
                    ESP_LOGI(TAG, "A abrir ecra: %s", s_menu_labels[s_menu_focus_idx]);
                    dc_gpio_hal_set_led(0, 60, 255); /* feedback visual rápido no LED */
                    dc_ui_open_menu_item(s_menu_focus_idx);
                }
            } else if (s_current_screen == DC_UI_SCREEN_SETTINGS) {
                if (ev == DC_BTN_EVENT_SHORT_PRESS) {
                    dc_ui_increment_brightness();
                } else if (ev == DC_BTN_EVENT_LONG_PRESS) {
                    dc_ui_go_home();
                }
            } else {
                /* Now Playing / placeholders: só "voltar" está disponível por agora */
                if (ev == DC_BTN_EVENT_LONG_PRESS) {
                    dc_ui_go_home();
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
        dc_ui_build_home_screen();
        dc_ui_build_now_playing_screen();
        dc_ui_build_settings_screen();
        dc_ui_build_placeholder_screen();
        lv_scr_load(s_scr_home); /* garante que arrancamos no Home */
        lvgl_port_unlock();
    }

    xTaskCreatePinnedToCore(dc_ui_task, "dc_ui_task",
                             DC_TASK_UI_STACK_SIZE, NULL,
                             DC_TASK_UI_PRIORITY, NULL, DC_TASK_UI_CORE);

    ESP_LOGI(TAG, "UI Manager pronto — ecrã Home carregado (touch=%d, botao BOOT sempre disponivel)",
             dc_touch_hal_is_ready());
    return ESP_OK;
}
