# DC — Assistente Pessoal Inteligente

> **DC** é uma assistente inteligente pessoal com ecrã touchscreen, voz, música, agenda, chamadas e serviços digitais, construída sobre **ESP32-S3**. A arquitetura está preparada desde o início para, numa segunda fase, controlar dispositivos **IoT** através de MQTT/API.

---

## Visão do produto

A **DC V1** é, primeiro, uma **assistente pessoal multimédia** — IA, música, agenda, chamadas e funções pessoais — e só depois evolui para IoT.

## Estrutura do projeto

```
dcassistent/
├── README.md
├── docs/
│   ├── arquitetura.md      # Arquitetura HW/SW e Gateway
│   └── roadmap.md          # Roadmap 0.1 → 2.0
├── firmware/               # ESP32-S3 (interface, áudio, Wi-Fi, BT)
└── dc-gateway/             # O "cérebro" (IA, música, agenda, chamadas...)
```

## Documentação

- [Arquitetura](docs/arquitetura.md)
- [Roadmap](docs/roadmap.md)
- [Firmware ESP32-S3](firmware/README.md)
- [DC Gateway](dc-gateway/README.md)

## Funções principais da V1

1. 🤖 **IA / conversa** — perguntar, responder por voz, conversa pelo touchscreen, histórico, wake word "Olá DC"
2. 🎵 **Música** — Spotify, play/pause, próxima/anterior, volume, escolher música/artista, capa no ecrã
3. 📅 **Agenda** — compromissos, criar compromisso, lembretes, "o que tenho hoje?"
4. 📞 **Chamadas** — contactos, iniciar/receber chamadas, histórico
5. ⏰ **Funções pessoais** — alarmes, temporizadores, relógio, cronómetro, meteorologia, notícias, calculadora, notas
6. ⚙️ **Configurações** — Wi-Fi, Bluetooth, volume, brilho, idioma, conta, Spotify, Google/Apple Calendar

## Frontend Preview (`frontend-preview/index.html`)

Mockup interativo do sistema operativo DC (launcher + apps), 100% estático,
com estado persistido em `localStorage`. Corre localmente com qualquer
servidor estático (ex.: `python3 -m http.server 3000`).

### Aplicações do sistema (launcher)
DC Assistant · Controlo · Monitorização · Alarmes · Agenda · Música · Loja · Definições

### Aplicações da Loja — **agora totalmente funcionais**
Instaláveis a partir da app **Loja**; deixaram de ser ecrãs "Em breve":

| App | Funcionalidades |
|-----|-----------------|
| ☀️ **Clima / Meteo** | Tempo atual, seletor de cidade (Lisboa, Porto, Faro, Coimbra, Braga), previsão horária e de 5 dias, humidade/vento/UV/sensação |
| 📝 **Notas** | Criar, editar, apagar e colorir notas — persistidas no dispositivo |
| 📞 **Chamadas** | Contactos, favoritos, histórico (recebida/perdida/efetuada) e teclado de marcação |
| 📁 **Ficheiros** | Navegação por pastas (Documentos, Imagens, Música, Sistema) e barra de armazenamento |
| ✉️ **E-mail** | Caixa de entrada, leitura de mensagem, marcar como lida e apagar |
| 🛡️ **Segurança** | Armar/desarmar sistema, 4 câmaras (REC), estado dos sensores |
| 💧 **Rega** | Controlo por zonas, humidade do solo, ligar/parar tudo, programação |
| 📊 **Relatórios** | KPIs do sistema, gráfico de atividade semanal, registos e exportação |

### Definições
Acrescentadas gestão real de **Permissões das aplicações** e **Notificações**
(antes eram apenas "Em breve").

### Armazenamento (frontend-preview)
Estado guardado em `localStorage` sob os prefixos `dc_system_settings`,
`dc_assistant_settings`, `dc_store_installed`, `dc_app_*` (notas, clima,
contactos, e-mails, rega, permissões, notificações).

### Como testar
```bash
cd frontend-preview
python3 -m http.server 3000
# abrir http://localhost:3000/index.html
```
