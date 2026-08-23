import { config } from "../../core/config/index.js";
import { childLogger } from "../../core/logging/index.js";

const log = childLogger("spotify-auth");
const SPOTIFY_ACCOUNTS = "https://accounts.spotify.com";

const SCOPES = [
  "user-read-playback-state",
  "user-modify-playback-state",
  "user-read-currently-playing",
].join(" ");

/**
 * Fluxo OAuth Authorization Code do Spotify (secção 8: "autenticacao",
 * "ligacao da conta"). Os tokens sao guardados apenas em memoria nesta
 * fase — numa fase seguinte devem ser persistidos por utilizador na
 * base de dados (secção 20), nunca em texto simples.
 */
export function getAuthorizeUrl(state: string): string {
  const params = new URLSearchParams({
    response_type: "code",
    client_id: config.music.spotifyClientId ?? "",
    scope: SCOPES,
    redirect_uri: config.music.spotifyRedirectUri ?? "",
    state,
  });
  return `${SPOTIFY_ACCOUNTS}/authorize?${params.toString()}`;
}

interface TokenResponse {
  access_token: string;
  refresh_token?: string;
  expires_in: number;
}

class InMemorySpotifyTokenStore {
  private accessToken?: string;
  private refreshToken?: string;
  private expiresAt = 0;

  async exchangeCode(code: string): Promise<void> {
    const body = new URLSearchParams({
      grant_type: "authorization_code",
      code,
      redirect_uri: config.music.spotifyRedirectUri ?? "",
    });
    await this.requestToken(body);
  }

  private async refresh(): Promise<void> {
    if (!this.refreshToken) throw new Error("Sem refresh token do Spotify.");
    const body = new URLSearchParams({
      grant_type: "refresh_token",
      refresh_token: this.refreshToken,
    });
    await this.requestToken(body);
  }

  private async requestToken(body: URLSearchParams): Promise<void> {
    const basicAuth = Buffer.from(
      `${config.music.spotifyClientId}:${config.music.spotifyClientSecret}`,
    ).toString("base64");

    const res = await fetch(`${SPOTIFY_ACCOUNTS}/api/token`, {
      method: "POST",
      headers: {
        Authorization: `Basic ${basicAuth}`,
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body,
    });

    if (!res.ok) {
      log.error({ status: res.status }, "Falha ao obter token do Spotify");
      throw new Error("Falha na autenticacao Spotify.");
    }

    const data = (await res.json()) as TokenResponse;
    this.accessToken = data.access_token;
    if (data.refresh_token) this.refreshToken = data.refresh_token;
    this.expiresAt = Date.now() + data.expires_in * 1000 - 30_000;
  }

  async getAccessToken(): Promise<string | undefined> {
    if (this.accessToken && Date.now() < this.expiresAt) return this.accessToken;
    if (this.refreshToken) {
      await this.refresh();
      return this.accessToken;
    }
    return undefined;
  }
}

export const spotifyTokenStore = new InMemorySpotifyTokenStore();
