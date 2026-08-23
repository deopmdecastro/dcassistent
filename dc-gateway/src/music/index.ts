import { config } from "../core/config/index.js";
import { eventBus } from "../core/events/index.js";
import { toolRegistry } from "../ai/tools/registry.js";
import { MockMusicProvider } from "./mock-provider.js";
import type { MusicProvider } from "./provider.js";
import { SpotifyMusicProvider } from "./spotify/spotify-provider.js";
import { spotifyTokenStore } from "./spotify/spotify-auth.js";

export * from "./provider.js";
export { getAuthorizeUrl } from "./spotify/spotify-auth.js";

function createMusicProvider(): MusicProvider {
  if (config.music.provider === "spotify") {
    return new SpotifyMusicProvider(spotifyTokenStore);
  }
  return new MockMusicProvider();
}

export const musicProvider = createMusicProvider();

async function emitStateChange(state: Awaited<ReturnType<MusicProvider["getState"]>>) {
  eventBus.emitEvent("music.state_changed", { track: state.track, isPlaying: state.isPlaying });
  return state;
}

/**
 * Regista as tools de musica para a IA (secção 8): spotify_search,
 * spotify_play, spotify_pause, spotify_next, spotify_previous,
 * spotify_set_volume. Nomeadas com prefixo spotify_ por serem hoje o
 * unico provider, mas a IA fala sempre com `musicProvider`, nunca com
 * o Spotify diretamente — trocar de provider nao muda os nomes de tool.
 */
export function registerMusicTools(): void {
  toolRegistry.register(
    {
      name: "spotify_search",
      description: "Pesquisa musicas por titulo ou artista.",
      parameters: {
        type: "object",
        properties: { query: { type: "string" } },
        required: ["query"],
      },
    },
    async (args) => musicProvider.search(String(args.query)),
  );

  toolRegistry.register(
    {
      name: "spotify_play",
      description: "Reproduz uma musica (por id) ou retoma a reproducao atual.",
      parameters: {
        type: "object",
        properties: { trackId: { type: "string" } },
      },
    },
    async (args) => emitStateChange(await musicProvider.play(args.trackId ? String(args.trackId) : undefined)),
  );

  toolRegistry.register(
    { name: "spotify_pause", description: "Pausa a reproducao atual.", parameters: { type: "object", properties: {} } },
    async () => emitStateChange(await musicProvider.pause()),
  );

  toolRegistry.register(
    { name: "spotify_next", description: "Avanca para a proxima musica.", parameters: { type: "object", properties: {} } },
    async () => emitStateChange(await musicProvider.next()),
  );

  toolRegistry.register(
    { name: "spotify_previous", description: "Volta para a musica anterior.", parameters: { type: "object", properties: {} } },
    async () => emitStateChange(await musicProvider.previous()),
  );

  toolRegistry.register(
    {
      name: "spotify_set_volume",
      description: "Ajusta o volume (0-100).",
      parameters: {
        type: "object",
        properties: { percent: { type: "number" } },
        required: ["percent"],
      },
    },
    async (args) => emitStateChange(await musicProvider.setVolume(Number(args.percent))),
  );
}
