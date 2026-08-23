# Firmware — ESP32-S3

O firmware do ESP32-S3 mantém-se **simples**. Ele não precisa saber o que é Spotify, calendário ou IA — apenas comunica com o **DC Gateway**.

## Placa confirmada: LCDWIKI ES3N28P

- **Módulo**: ESP32-S3 (dual-core LX7 @ 240 MHz), 8 MB PSRAM Octal, 16 MB Flash QSPI.
- **LCD**: 2.8" IPS, 240×320, driver ILI9341V, SPI 4 fios.
- **Sem touch capacitivo** (a ES3C28P tem; a ES3N28P não). A interface V1 navega-se pelo **botão BOOT** (toque curto = próximo item, toque longo = selecionar).
- Áudio: codec ES8311 (I2C) + amplificador FM8002E, mic MEMS, I2S.
- MicroSD (SDIO), LED RGB WS2812B, bateria LiPo com TP4054.
- Pinout completo confirmado em [`../docs/hardware.md`](../docs/hardware.md) e em [`main/app_config.h`](main/app_config.h).

## Estado do código

- **DC 0.1 (HAL)**: LCD + backlight + botão + LED — implementado (`main/hal/`).
- **DC 0.2 (Interface LVGL)**: ecrã Home + menu navegável, mais os sub-ecrãs Now Playing, Definições (brilho já ajustável de verdade) e placeholder Agenda/Chamadas — implementado (`main/services/ui_manager.c`).
- **DC 0.3+ (áudio, Wi-Fi, Gateway, OTA)**: por implementar — estrutura já preparada em `docs/firmware-architecture.md`.

## Compilar (ESP-IDF ≥ 5.3)

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor   # ajustar a porta série
```

As dependências (LVGL, esp_lvgl_port, esp_lcd_ili9341, led_strip) são geridas automaticamente pelo ESP Component Registry via `main/idf_component.yml`.

## Entrada

- Microfone
- Touch
- Botões

## Processamento local

- Interface
- Áudio
- Wi-Fi
- Bluetooth
- Wake Word
- Estados

## Saída

- LCD
- Speaker
- LED

## Comunicação

O ESP32 comunica com o **DC Gateway** (por Wi-Fi/API), que é o verdadeiro "cérebro".

```
Voz → ESP32 → Gateway → Serviço → Resposta → Speaker
```

## Roadmap de firmware

- **DC 0.1** — Hardware (LCD, Touch, Wi-Fi, Microfone, Speaker)
- **DC 0.2** — Interface LVGL (Home, Menu, Música, Agenda, Definições)
- **DC 0.3** — Voz (STT → IA → TTS)
- **DC 0.4** — IA (conversação, memória, tools)
- **DC 0.5** — Spotify
- **DC 0.6** — Agenda
- **DC 0.7** — Chamadas
