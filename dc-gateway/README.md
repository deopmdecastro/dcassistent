# DC Gateway

O **Gateway** é o verdadeiro "cérebro" da DC. Adiciona funcionalidades sem mexer constantemente no firmware.

## Estrutura

```
dc-gateway/

├── AI
│   ├── chat
│   ├── memory
│   └── tools
│
├── MUSIC
│   └── Spotify
│
├── CALENDAR
│   └── Agenda
│
├── CALLS
│   └── Chamadas
│
├── WEATHER
│
├── REMINDERS
│
├── NOTIFICATIONS
│
└── IOT
    └── futuro
```

## Serviços

| Módulo | Descrição |
|---|---|
| AI | Conversação, memória e ferramentas (tools) |
| MUSIC | Integração Spotify (play/pause, próxima/anterior, volume, capa) |
| CALENDAR | Agenda, eventos, lembretes (Google/Apple Calendar) |
| CALLS | Contactos, iniciar/receber chamadas, histórico |
| WEATHER | Meteorologia |
| REMINDERS | Lembretes |
| NOTIFICATIONS | Notificações |
| IOT | Futuro — MQTT, sensores, relés, bombas |

## Tools da IA

A IA chama ferramentas sem saber como cada sistema funciona:

- `get_calendar_events()` / `create_calendar_event()` / `create_reminder()` → Agenda
- `spotify_search()` / `spotify_play()` / `spotify_pause()` / `spotify_next()` / `spotify_previous()` / `spotify_set_volume()` → Música
- `make_call("João")` → Chamadas
- `get_weather()` → Meteorologia
- `create_note()` → Notas
- `create_alarm()` → Alarmes
- `create_timer()` → Temporizadores
- `iot.turn_on("pump_1")` → IoT (futuro, via MQTT)

## Estado da implementação (V1 — Fase 3)

✅ Implementado nesta fase:
- Core (config validada, logging com redação de segredos, event bus, estado global)
- Sistema de Tools (`ToolRegistry`) — a peça central que desacopla a IA dos serviços
- IA: provider Anthropic (Claude, com tool use nativo) + provider mock
- Música: provider Spotify (OAuth + Web API) + provider mock
- Agenda: eventos e lembretes (em memória)
- Chamadas: contactos + gestão de chamadas (mock, arquitetura pronta para VoIP)
- Funções pessoais: notas, alarmes, temporizadores, notificações
- API REST modular (`/api/...`), documentada em [`API.md`](./API.md)

⏳ Pendente para as próximas fases:
- Persistência em base de dados (atualmente tudo em memória — reinicia ao reiniciar o servidor)
- Interface touchscreen do ESP32-S3 (firmware LVGL)
- Voz (STT/TTS)
- Autenticação de utilizador/dispositivo real
- WebSocket/MQTT para estado em tempo real entre ESP32 e Gateway
- Wake word

## Como correr localmente

```bash
cd dc-gateway
cp .env.example .env   # por omissão usa providers mock, sem credenciais
npm install
npm run dev             # http://localhost:4000
```

Ver [`DEVELOPMENT.md`](./DEVELOPMENT.md) para mais detalhes.
