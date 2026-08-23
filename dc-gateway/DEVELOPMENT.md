# Desenvolvimento — DC Gateway

## Requisitos

- Node.js 18+
- npm

## Setup

```bash
cd dc-gateway
cp .env.example .env
npm install
```

Por omissão (`AI_PROVIDER=mock`, `MUSIC_PROVIDER=mock`, etc.) o gateway corre sem qualquer credencial real — ideal para desenvolvimento local.

## Comandos

```bash
npm run dev        # arranca com watch (tsx)
npm run build       # compila TypeScript -> dist/
npm start           # corre a build compilada
npm run typecheck   # tsc --noEmit
npm run lint        # eslint
npm test            # vitest
```

## Ligar credenciais reais

Editar `.env` (nunca commitar este ficheiro):

- **IA (Claude)**: `AI_PROVIDER=anthropic` + `ANTHROPIC_API_KEY=...`
- **Spotify**: `MUSIC_PROVIDER=spotify` + `SPOTIFY_CLIENT_ID` + `SPOTIFY_CLIENT_SECRET` + `SPOTIFY_REDIRECT_URI`. Depois visitar `GET /api/music/spotify/login` para autorizar a conta.

Se um provider "real" for pedido sem as credenciais necessárias, o gateway regista um aviso e cai automaticamente para o provider mock (ver `src/core/config/index.ts`).

## Adicionar uma nova Tool de IA

1. Escolher/criar o módulo do serviço (ex: `src/weather/index.ts`).
2. Registar a tool com `toolRegistry.register(definition, handler)`.
3. Chamar a função `registerXTools()` a partir de `src/ai/index.ts`.

O núcleo da IA (`ChatService`, `ToolRegistry`) nunca precisa de ser alterado ao adicionar uma nova tool — é assim que a arquitetura permite, por exemplo, adicionar IoT no futuro sem tocar na IA.

## Estrutura de pastas

Ver `docs/arquitetura.md` na raiz do repositório e o `README.md` desta pasta.
