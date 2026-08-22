# Arquitetura da DC

## Visão geral

```
                    DC
                     │
             ┌───────┴────────┐
             │                 │
          HARDWARE          SOFTWARE
             │                 │
        ESP32-S3             Gateway
        LCD Touch               │
        Microfone               │
        Speaker          ┌──────┼────────┐
                         │      │        │
                        IA   Música    Agenda
                         │      │        │
                         │   Spotify   Calendar
                         │
                         ├── Chamadas
                         ├── Alarmes
                         ├── Clima
                         └── Notas
```

## ESP32-S3 — Hardware simples

O ESP32 não precisa saber o que é Spotify, calendário ou IA.

### Entrada
- Microfone
- Touch
- Botões

### Processamento local
- Interface
- Áudio
- Wi-Fi
- Bluetooth
- Wake Word
- Estados

### Saída
- LCD
- Speaker
- LED

## DC Gateway — o verdadeiro "cérebro"

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

Assim, podemos adicionar funcionalidades sem mexer constantemente no firmware.

## Exemplo de utilização

> Tu: **"Coloca música do The Weeknd."**

```
Voz
 ↓
ESP32
 ↓
Gateway
 ↓
Spotify
 ↓
Música
 ↓
Speaker
```

E o LCD mostra:

```
┌─────────────────────────┐
│          DC             │
│                         │
│      🎵                 │
│                         │
│   Blinding Lights       │
│   The Weeknd            │
│                         │
│  ◀     ⏸     ▶         │
│                         │
│       ━━━━━━━━          │
└─────────────────────────┘
```

## IA + ferramentas (tools)

A IA deve ter **ferramentas (tools)**. A IA não precisa saber como cada sistema funciona — apenas chama ferramentas.

```
Utilizador
   │
   ▼
"DC, o que tenho amanhã?"
   │
   ▼
    IA
   │
   └──► get_calendar_events()
                │
                ▼
             Agenda
                │
                ▼
          "Tens 3 eventos..."
```

```
IA
 │
 └──► spotify_play()
```

```
IA
 │
 └──► make_call("João")
```

Futuramente:

```
IA
 │
 └──► iot.turn_on("pump_1")
                         │
                         ▼
                       MQTT
                         │
                         ▼
                       ESP32
```

**É aqui que a arquitetura fica poderosa:** a IA não precisa saber como cada sistema funciona. Ela apenas chama ferramentas.

## Ecrã principal da DC

```
┌──────────────────────────────┐
│  23:45              🔋 82%   │
│  Wi-Fi ●                     │
│                              │
│             DC               │
│                              │
│             ◉                │
│                              │
│       "Olá! Sou a DC"        │
│                              │
│ ──────────────────────────── │
│                              │
│ 🎵 Música    📅 Agenda       │
│                              │
│ 📞 Chamadas  ⚙ Definições   │
└──────────────────────────────┘
```

## IoT — segunda fase

```
                         DC
                          │
                       Gateway
                          │
             ┌────────────┼────────────┐
             │            │            │
             IA        Serviços       IoT
             │            │            │
          Conversa     Spotify       MQTT
                       Agenda           │
                       Chamadas       ESP32
                       Clima            │
                                    Sensores
                                    Relés
                                    Bombas
```
