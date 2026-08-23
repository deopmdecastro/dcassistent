/**
 * @file app_config.h
 * @brief Pinout, tamanhos e configuração global da DC V1.
 *
 * Placa: LCDWIKI ES3C28P — 2.8" IPS ESP32-S3 (variante COM touch capacitivo).
 * Mesmo pinout físico da ES3N28P; a ES3C28P tem o FT6336G soldado no mesmo
 * barramento I2C que é partilhado com o codec de áudio (ES8311).
 *
 * NOTA DE MIGRAÇÃO (DC 0.3): o projeto usava até aqui a ES3N28P (sem touch,
 * navegação por botão BOOT). Confirmado com o utilizador o upgrade para a
 * ES3C28P para permitir navegação por toque nas 4 telas principais — ver
 * docs/hardware.md. O botão BOOT mantém-se como input secundário/recovery.
 *
 * Fonte do pinout: datasheet oficial LCDWIKI (CR2025-MI6875 / CR2025-MI6872)
 * e BSP de referência (ES3C28P/ES3N28P partilham pinout).
 *
 * IMPORTANTE: confirmar com multímetro/continuidade na placa física antes do
 * primeiro boot — os valores abaixo refletem a documentação do fabricante,
 * não uma verificação direta no hardware do Manuel.
 */
#pragma once

#include "driver/gpio.h"

/* ------------------------------------------------------------------------ */
/* Identificação da placa                                                   */
/* ------------------------------------------------------------------------ */
#define DC_BOARD_NAME           "ES3C28P"
#define DC_BOARD_HAS_TOUCH      1   /* ES3C28P = 1, ES3N28P = 0 */
#define DC_BOARD_HAS_SD         1
#define DC_BOARD_HAS_RGB_LED    1   /* WS2812B */

/* ------------------------------------------------------------------------ */
/* LCD — ILI9341V, 240x320, SPI 4 fios                                      */
/* ------------------------------------------------------------------------ */
#define DC_LCD_H_RES             240
#define DC_LCD_V_RES              320
#define DC_LCD_PIN_CS            GPIO_NUM_10
#define DC_LCD_PIN_DC            GPIO_NUM_46
#define DC_LCD_PIN_SCK           GPIO_NUM_12
#define DC_LCD_PIN_MOSI          GPIO_NUM_11
#define DC_LCD_PIN_MISO          GPIO_NUM_13
#define DC_LCD_PIN_RST           -1   /* partilhado com o reset do ESP32-S3 */
#define DC_LCD_PIN_BL            GPIO_NUM_45
#define DC_LCD_SPI_HOST          SPI2_HOST
#define DC_LCD_SPI_CLOCK_HZ      (40 * 1000 * 1000)
#define DC_LCD_BL_PWM_FREQ_HZ    5000

/* ------------------------------------------------------------------------ */
/* Touch — FT6336G, I2C (montado na ES3C28P; ausente na ES3N28P)            */
/* Partilha o barramento I2C com o codec de áudio ES8311 — ambos os drivers */
/* devem usar o mesmo handle de porto I2C (DC_I2C_PORT), nunca inicializar  */
/* o barramento duas vezes.                                                 */
/* ------------------------------------------------------------------------ */
#define DC_I2C_PIN_SDA           GPIO_NUM_16   /* partilhado: touch (se existir) + codec ES8311 */
#define DC_I2C_PIN_SCL           GPIO_NUM_15
#define DC_I2C_PORT              I2C_NUM_0
#define DC_I2C_FREQ_HZ           400000
#if DC_BOARD_HAS_TOUCH
#define DC_TOUCH_PIN_RST         GPIO_NUM_18
#define DC_TOUCH_PIN_INT         GPIO_NUM_17
#define DC_TOUCH_I2C_ADDR        0x38
#endif

/* ------------------------------------------------------------------------ */
/* Codec de áudio — ES8311 (I2C addr 0x18) + amplificador FM8002E, I2S      */
/* ------------------------------------------------------------------------ */
#define DC_AUDIO_CODEC_I2C_ADDR  0x18
#define DC_AUDIO_PIN_AMP_EN      GPIO_NUM_1    /* ativo em nível baixo */
#define DC_AUDIO_PIN_MCLK        GPIO_NUM_4
#define DC_AUDIO_PIN_BCLK        GPIO_NUM_5
#define DC_AUDIO_PIN_DOUT        GPIO_NUM_6    /* ESP32-S3 -> codec (speaker) */
#define DC_AUDIO_PIN_LRCK        GPIO_NUM_7
#define DC_AUDIO_PIN_DIN         GPIO_NUM_8    /* codec -> ESP32-S3 (mic) */
#define DC_AUDIO_SAMPLE_RATE_HZ  16000
#define DC_AUDIO_BITS_PER_SAMPLE 16

/* ------------------------------------------------------------------------ */
/* Cartão MicroSD — SDIO 4 bits                                             */
/* ------------------------------------------------------------------------ */
#define DC_SD_PIN_CLK            GPIO_NUM_38
#define DC_SD_PIN_CMD            GPIO_NUM_40
#define DC_SD_PIN_D0             GPIO_NUM_39
#define DC_SD_PIN_D1             GPIO_NUM_41
#define DC_SD_PIN_D2             GPIO_NUM_48
#define DC_SD_PIN_D3             GPIO_NUM_47

/* ------------------------------------------------------------------------ */
/* Botões e LED de estado                                                   */
/*                                                                            */
/* A ES3N28P não tem touch, por isso a navegação da interface V1 é feita     */
/* pelo botão físico BOOT (curto = próximo item, longo = selecionar).        */
/* Os 4 pinos de expansão ficam reservados para um teclado físico futuro     */
/* (ex. central/wake, vol+, vol-) — "a confirmar" quando o Manuel decidir    */
/* o layout de botões externos.                                             */
/* ------------------------------------------------------------------------ */
#define DC_PIN_BTN_BOOT          GPIO_NUM_0    /* também entra em modo download se premido no boot */
#define DC_PIN_LED_RGB           GPIO_NUM_42   /* WS2812B, 1 pino */
#define DC_PIN_BAT_ADC           GPIO_NUM_9    /* leitura de tensão da bateria (divisor /2) */

/* Expansão livre (a confirmar layout de botões externos: vol+/vol-/central) */
#define DC_PIN_EXPAND_1          GPIO_NUM_2
#define DC_PIN_EXPAND_2          GPIO_NUM_3
#define DC_PIN_EXPAND_3          GPIO_NUM_14
#define DC_PIN_EXPAND_4          GPIO_NUM_21

/* ------------------------------------------------------------------------ */
/* Debug / UART0                                                            */
/* ------------------------------------------------------------------------ */
#define DC_UART_PIN_TX           GPIO_NUM_44
#define DC_UART_PIN_RX           GPIO_NUM_43

/* ------------------------------------------------------------------------ */
/* Gateway (DC Gateway) — endereço configurável em runtime via NVS          */
/* ------------------------------------------------------------------------ */
#define DC_GATEWAY_DEFAULT_URL   "http://dc-gateway.local:3000"
#define DC_GATEWAY_WS_PATH       "/ws"

/* ------------------------------------------------------------------------ */
/* Memória / tarefas (ver docs/firmware-architecture.md secção 4 e 9)       */
/* ------------------------------------------------------------------------ */
#define DC_TASK_UI_STACK_SIZE     8192
#define DC_TASK_UI_PRIORITY       5
#define DC_TASK_UI_CORE           1

#define DC_TASK_AUDIO_STACK_SIZE  4096
#define DC_TASK_AUDIO_PRIORITY    8
#define DC_TASK_AUDIO_CORE        0

#define DC_TASK_NET_STACK_SIZE    4096
#define DC_TASK_NET_PRIORITY      6
#define DC_TASK_NET_CORE          0

#define DC_LVGL_BUF_LINES         40   /* linhas por buffer de flush, em PSRAM */
