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
  - Ecrã Home ampliado para 6 atalhos: Música, Agenda, Chamadas, **Voz** e **Apps** (novos) e Definições.
  - Ecrã de Voz: botão de microfone tocável com feedback sonoro real; mostra honestamente que a captura (STT) ainda não está ligada, em vez de simular uma conversa.
  - Ecrã de Apps: launcher em grelha com 8 ícones (Música, Agenda, Gestor, E-mail, Notas, Clima, Relatórios, Config.) — Música e Config. abrem os ecrãs reais existentes, os restantes mostram "Em breve".
  - Touch FT6336G ligado ao LVGL (`main/hal/touch_hal.c`, escrito mas **não testado em hardware/toolchain** nesta sessão de trabalho — reveja com `idf.py build` antes do primeiro flash).
  - Armazenamento persistente modular (`main/storage/storage_manager.c` sobre NVS + `main/storage/settings_manager.c` tipado: Wi-Fi, brilho, volume, tema, favoritos).
  - Wi-Fi não bloqueante com reconexão controlada (`main/wifi/wifi_manager.c`): liga automaticamente à última rede guardada, expõe estados (Desligado/A ligar/Ligado/Sem rede/Erro) à UI.
  - Arquitetura de áudio (`main/audio/`): saída via ES8311+I2S funcional (por escrever/testar em hardware), controlo de volume, feedback sonoro em toques da UI. Captura de microfone é um **stub documentado** (`audio_input.c` devolve `ESP_ERR_NOT_SUPPORTED`) — full-duplex com STT/wake-word fica para uma fase posterior, não foi simulado.
- **DC 0.3.1 (UI web + responsividade)** — nesta atualização:
  - Novo módulo `main/net/web_server.c` — HTTP + WebSocket com `esp_http_server`. Arranca automaticamente quando o Wi-Fi obtém IP; pára-se quando cai.
  - Nova partição SPIFFS `dcweb` (2 MB, ver `partitions.csv`) que embebe todo o `frontend-preview/` diretamente no flash. O `frontend-preview/index.html` recebeu novos breakpoints CSS específicos para o ecrã nativo 240×320 e para `orientation: portrait`, mais ajustes para touch (`(hover: none) and (pointer: coarse)`).
  - Fluxo end-to-end: `idf.py build && idf.py flash` grava app **+** SPIFFS. Depois basta o ESP32 ligar-se a uma rede Wi-Fi e o utilizador aceder a `http://<ip-do-esp32>/` no browser.
  - WebSocket em `/ws` está pronto para receber comandos (payload JSON) — cablagem à `ui_task`/`audio_task` fica para a DC 0.4.
- **DC 0.4+ (Gateway completo, OTA, voz)**: por implementar — estrutura já preparada em `docs/firmware-architecture.md`.

## Compilar (ESP-IDF ≥ 5.3)

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor   # ajustar a porta série
```

O comando `flash` grava também a partição SPIFFS `dcweb` (2 MB) com o
`frontend-preview/` inteiro. Depois de o ESP32 se ligar à Wi-Fi, o log do
monitor imprime algo como:

```
I (12345) dc_web_server: DC OS web disponivel em http://192.168.1.42/
```

Basta abrir esse endereço num browser (telemóvel, tablet ou desktop) para
ver a interface real da DC. Para atualizar só a UI sem reflashar todo o
firmware:

```bash
idf.py dcweb-flash
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
- **DC 0.3.1** — UI web servida pelo próprio ESP32 (HTTP + WS) + responsividade 240×320 ✅ implementada nesta sessão
- **DC 0.4** — Voz (STT → IA → TTS) + IA (conversação, memória, tools)
- **DC 0.5** — Spotify
- **DC 0.6** — Agenda
- **DC 0.7** — Chamadas

