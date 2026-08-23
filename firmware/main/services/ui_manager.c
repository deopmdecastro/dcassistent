/**
 * @file ui_manager.c
 * @brief Interface LVGL da DC — ecrã Home com relógio/wifi/bateria, avatar
 * central e menu (Música/Agenda/Chamadas/Definições), navegável pelo botão
 * BOOT (a ES3N28P não tem touch — ver hal/gpio_hal.h).
 *
 * DC 0.2 do roadmap: interface navegável, sem ainda estar ligada ao Gateway.
 */
#include "ui_manager.h"
#include "app_config.h"
#include "hal/lcd_hal.h"
#include "hal/gpio_hal.h"

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

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, DC_COLOR_TEXT, 0);
    lv_obj_center(lbl);

    return card;
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
    }
    dc_ui_highlight_menu(s_menu_focus_idx);

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
            if (ev == DC_BTN_EVENT_SHORT_PRESS) {
                s_menu_focus_idx = (s_menu_focus_idx + 1) % DC_MENU_COUNT;
                dc_ui_highlight_menu(s_menu_focus_idx);
            } else if (ev == DC_BTN_EVENT_LONG_PRESS) {
                ESP_LOGI(TAG, "Item selecionado: %s (acao a implementar na fase 0.4 — Gateway)",
                         s_menu_labels[s_menu_focus_idx]);
                dc_gpio_hal_set_led(0, 60, 255); /* feedback visual rápido no LED */
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

    if (lvgl_port_lock(0)) {
        dc_ui_build_home_screen();
        lvgl_port_unlock();
    }

    xTaskCreatePinnedToCore(dc_ui_task, "dc_ui_task",
                             DC_TASK_UI_STACK_SIZE, NULL,
                             DC_TASK_UI_PRIORITY, NULL, DC_TASK_UI_CORE);

    ESP_LOGI(TAG, "UI Manager pronto — ecrã Home carregado, navegacao pelo botao BOOT");
    return ESP_OK;
}
