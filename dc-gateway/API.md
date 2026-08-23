# DC Gateway — API

Base URL local: `http://localhost:4000`

Todas as respostas são JSON. Erros seguem o formato `{ "error": "mensagem" }`.

## Health

- `GET /health` — estado do serviço.

## Assistente / Chat (`/api/assistant`)

- `GET /state` — estado global da assistente (`idle`, `listening`, `processing`, `speaking`, `music`, `calling`, `error`, `offline`).
- `POST /message` — body `{ sessionId, text }`. Envia uma mensagem à IA; a IA pode usar tools internamente antes de responder. Devolve `{ text, toolsUsed }`.
- `GET /conversations/:sessionId` — histórico da conversa.
- `DELETE /conversations/:sessionId` — limpa o histórico.

## Música (`/api/music`)

- `GET /state`, `GET /search?q=...`
- `POST /play` `{ trackId? }`, `POST /pause`, `POST /next`, `POST /previous`
- `POST /volume` `{ percent }`
- `GET /spotify/login` — redireciona para o OAuth do Spotify.
- `GET /spotify/callback` — callback do OAuth (usado pelo Spotify, não chamar diretamente).

## Agenda (`/api/calendar`)

- `GET /events?from=&to=` (opcional; sem parâmetros devolve os próximos eventos)
- `POST /events` `{ title, startsAt, endsAt?, description? }`
- `PATCH /events/:id`, `DELETE /events/:id`
- `GET /reminders`, `POST /reminders` `{ text, dueAt }`

## Chamadas (`/api/calls`)

- `GET /contacts`, `POST /contacts` `{ name, phoneNumber? }`
- `GET /history`
- `POST /start` `{ contactId }`, `POST /:id/end`

## Meteorologia (`/api/weather`)

- `GET /?location=Porto`

## Notas (`/api/notes`)

- `GET /`, `POST /` `{ title, content }`, `PATCH /:id`, `DELETE /:id`

## Alarmes (`/api/alarms`)

- `GET /`, `POST /` `{ time: "HH:mm", label? }`, `PATCH /:id` `{ enabled }`, `DELETE /:id`

## Temporizadores (`/api/timers`)

- `GET /`, `POST /` `{ durationSeconds, label? }`, `POST /:id/pause`, `POST /:id/cancel`

## Notificações (`/api/notifications`)

- `GET /`, `POST /:id/read`

## Dispositivos (`/api/devices`)

- `GET /` — lista dispositivos ESP32-S3 conhecidos.
- `POST /:id/status` `{ status, name? }` — usado pelo firmware para reportar estado.

## Notas de implementação

- Todos os dados estão em memória nesta fase (reiniciam com o servidor). A estrutura de base de dados prevista está em `docs/arquitetura.md`.
- Quando `MUSIC_PROVIDER=mock` (omissão), as tools/rotas de música devolvem dados de demonstração — não é necessário ligar a conta Spotify para desenvolver.
- O mesmo se aplica a `AI_PROVIDER=mock` — a IA responde com eco simples até `ANTHROPIC_API_KEY` ser configurada.
