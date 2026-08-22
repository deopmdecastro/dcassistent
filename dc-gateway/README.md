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

- `get_calendar_events()` → Agenda
- `spotify_play()` → Música
- `make_call("João")` → Chamadas
- `iot.turn_on("pump_1")` → IoT (futuro, via MQTT)
