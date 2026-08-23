# Roadmap da DC

## 🟢 DC 0.1 — Hardware

Primeiro:

- ESP32-S3
- LCD
- Touch
- Wi-Fi
- Microfone
- Speaker

Testar tudo individualmente.

## 🟢 DC 0.2 — Interface

- LVGL
- Home
- Menu
- Música
- Agenda
- Definições
- Animações
- Estados da DC

## 🟢 DC 0.3 — Conectividade + Persistência + Áudio + Touch

- Wi-Fi (STA, não-bloqueante, reconexão automática)
- NVS / armazenamento persistente (settings, credenciais)
- Arquitetura de áudio (saída ES8311 funcional; captura de mic preparada, não ligada a STT ainda)
- Migração para placa ES3C28P (touch capacitivo) — navegação principal por toque, BOOT como fallback

## 🟢 DC 0.4 — Voz

```
Microfone → STT → IA → TTS → Speaker
```

## 🟢 DC 0.5 — IA

- Conversação
- Memória
- Tools
- Contexto

## 🟢 DC 0.6 — Spotify

- Login
- Reprodução
- Pausa
- Volume
- Músicas
- Playlists
- Capa

## 🟢 DC 0.7 — Agenda

- Calendário
- Eventos
- Lembretes
- Alarmes

## 🟢 DC 0.8 — Chamadas

- Contactos
- Chamadas
- Notificações
- Eventualmente vídeo

## 🔵 DC 1.0 — Assistente pessoal

Aqui já teremos uma DC realmente utilizável no dia a dia.

## 🟣 DC 2.0 — IoT

Só então:

- Luzes
- Tomadas
- Sensores
- Rega
- Bombas
- Climatização
- Segurança
- Automação
