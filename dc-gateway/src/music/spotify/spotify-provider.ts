import type { MusicProvider, PlaybackState, Track } from "../provider.js";
import { childLogger } from "../../core/logging/index.js";

const log = childLogger("spotify");
const SPOTIFY_API = "https://api.spotify.com/v1";

interface SpotifyTokenStore {
  getAccessToken(): Promise<string | undefined>;
}

/**
 * Provider real de Spotify (secção 8). Depende de um access token
 * valido (obtido via OAuth Authorization Code Flow — ver spotify-auth.ts).
 * Se nao houver token, os metodos lançam erro e o caller deve tratar
 * isso (ex: pedir ao utilizador para ligar a conta Spotify nas
 * Definicoes).
 */
export class SpotifyMusicProvider implements MusicProvider {
  readonly name = "spotify";

  constructor(private tokens: SpotifyTokenStore) {}

  private async request<T>(path: string, init?: RequestInit): Promise<T> {
    const accessToken = await this.tokens.getAccessToken();
    if (!accessToken) {
      throw new Error("Conta Spotify nao ligada. Vai a Definicoes > Spotify para autenticar.");
    }
    const res = await fetch(`${SPOTIFY_API}${path}`, {
      ...init,
      headers: {
        Authorization: `Bearer ${accessToken}`,
        "Content-Type": "application/json",
        ...(init?.headers ?? {}),
      },
    });
    if (!res.ok && res.status !== 204) {
      await res.text();
      log.error({ status: res.status, path }, "Erro na API do Spotify");
      throw new Error(`Spotify API error (${res.status})`);
    }
    if (res.status === 204) return undefined as T;
    return (await res.json()) as T;
  }

  async search(query: string): Promise<Track[]> {
    const data = await this.request<{ tracks: { items: SpotifyApiTrack[] } }>(
      `/search?type=track&limit=10&q=${encodeURIComponent(query)}`,
    );
    return data.tracks.items.map(mapSpotifyTrack);
  }

  async play(trackId?: string): Promise<PlaybackState> {
    await this.request("/me/player/play", {
      method: "PUT",
      body: trackId ? JSON.stringify({ uris: [`spotify:track:${trackId}`] }) : undefined,
    });
    return this.getState();
  }

  async pause(): Promise<PlaybackState> {
    await this.request("/me/player/pause", { method: "PUT" });
    return this.getState();
  }

  async next(): Promise<PlaybackState> {
    await this.request("/me/player/next", { method: "POST" });
    return this.getState();
  }

  async previous(): Promise<PlaybackState> {
    await this.request("/me/player/previous", { method: "POST" });
    return this.getState();
  }

  async setVolume(percent: number): Promise<PlaybackState> {
    await this.request(`/me/player/volume?volume_percent=${Math.round(percent)}`, { method: "PUT" });
    return this.getState();
  }

  async getState(): Promise<PlaybackState> {
    const data = await this.request<SpotifyPlaybackResponse | undefined>("/me/player");
    if (!data) return { isPlaying: false };
    return {
      isPlaying: data.is_playing,
      track: data.item ? mapSpotifyTrack(data.item) : undefined,
      positionMs: data.progress_ms ?? undefined,
      volumePercent: data.device?.volume_percent ?? undefined,
    };
  }
}

interface SpotifyApiTrack {
  id: string;
  name: string;
  artists: { name: string }[];
  album?: { name: string; images?: { url: string }[] };
  duration_ms?: number;
}

interface SpotifyPlaybackResponse {
  is_playing: boolean;
  progress_ms?: number;
  item?: SpotifyApiTrack;
  device?: { volume_percent?: number };
}

function mapSpotifyTrack(t: SpotifyApiTrack): Track {
  return {
    id: t.id,
    title: t.name,
    artist: t.artists.map((a) => a.name).join(", "),
    album: t.album?.name,
    artworkUrl: t.album?.images?.[0]?.url,
    durationMs: t.duration_ms,
  };
}
