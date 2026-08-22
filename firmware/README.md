# Firmware — ESP32-S3

O firmware do ESP32-S3 mantém-se **simples**. Ele não precisa saber o que é Spotify, calendário ou IA — apenas comunica com o **DC Gateway**.

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
