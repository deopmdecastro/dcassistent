# Firmware — ESP32-S3

O firmware do ESP32-S3 mantém-se **simples**. Ele não precisa saber o que é Spotify, calendário ou IA — apenas comunica com o **DC Gateway**.

## Placa: LCDWIKI ES3C28P (atualizado na DC 0.3 — antes era ES3N28P)

- **Módulo**: ESP32-S3 (dual-core LX7 @ 240 MHz), 8 MB PSRAM Octal, 16 MB Flash QSPI.
- **LCD**: 2.8" IPS, 240×320, driver ILI9341V, SPI 4 fios.
- **Touch capacitivo FT6336G** (I2C, partilhado com o codec de áudio) — a ES3C28P tem-no soldado; a ES3N28P (placa anterior) não. Navegação principal agora por toque; o **botão BOOT** continua disponível como input secundário/recovery.
- Áudio: codec ES8311 (I2C) + amplificador FM8002E, mic MEMS, I2S.
- MicroSD (SDIO), LED RGB WS2812B, bateria LiPo com TP4054.
- Pinout completo confirmado em [`../docs/hardware.md`](../docs/hardware.md) e em [`main/app_config.h`](main/app_config.h).

## Estado do código

- **DC 0.1 (HAL)**: LCD + backlight + botão + LED — implementado (`main/hal/`).
- **DC 0.2 (Interface LVGL)**: ecrã Home + menu, mais Now Playing/Definições/placeholders — implementado (`main/services/ui_manager.c`).
- **DC 0.3 (Wi-Fi + NVS + Áudio + Touch)**:
  - Touch FT6336G ligado ao LVGL (`main/hal/touch_hal.c`, escrito mas **não testado em hardware/toolchain** nesta sessão de trabalho — reveja com `idf.py build` antes do primeiro flash).
  - Armazenamento persistente modular (`main/storage/storage_manager.c` sobre NVS + `main/storage/settings_manager.c` tipado: Wi-Fi, brilho, volume, tema, favoritos).
  - Wi-Fi não bloqueante com reconexão controlada (`main/wifi/wifi_manager.c`): liga automaticamente à última rede guardada, expõe estados (Desligado/A ligar/Ligado/Sem rede/Erro) à UI.
  - Arquitetura de áudio (`main/audio/`): saída via ES8311+I2S funcional (por escrever/testar em hardware), controlo de volume, feedback sonoro em toques da UI. Captura de microfone é um **stub documentado** (`audio_input.c` devolve `ESP_ERR_NOT_SUPPORTED`) — full-duplex com STT/wake-word fica para uma fase posterior, não foi simulado.
- **DC 0.4+ (Gateway completo, OTA, voz)**: por implementar — estrutura já preparada em `docs/firmware-architecture.md`.

## Compilar (ESP-IDF ≥ 5.3)

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor   # ajustar a porta série
```

As dependências (LVGL, esp_lvgl_port, esp_lcd_ili9341, esp_lcd_touch(_ft6336), esp_codec_dev, led_strip) são geridas automaticamente pelo ESP Component Registry via `main/idf_component.yml`. **Nota:** as versões de `esp_lcd_touch`, `esp_lcd_touch_ft6336` e `esp_codec_dev` no `idf_component.yml` não foram validadas contra o registry nesta sessão (sem acesso de rede ao registry no ambiente onde este código foi escrito) — confirme no primeiro `idf.py build` e ajuste se necessário.

## Entrada

- Microfone (captura ainda não ligada a um consumidor — ver DC 0.3 acima)
- Touch (ES3C28P)
- Botão BOOT (sempre disponível, fallback)

## Processamento local

- Interface
- Áudio
- Wi-Fi
- Bluetooth (por implementar)
- Wake Word (por implementar)
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
- **DC 0.3** — Wi-Fi + NVS + Áudio + Touch (ES3C28P) ✅ implementado nesta sessão, pendente de build/teste em hardware
- **DC 0.4** — Voz (STT → IA → TTS) + IA (conversação, memória, tools)
- **DC 0.5** — Spotify
- **DC 0.6** — Agenda
- **DC 0.7** — Chamadas

