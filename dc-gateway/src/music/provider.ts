export interface Track {
  id: string;
  title: string;
  artist: string;
  album?: string;
  artworkUrl?: string;
  durationMs?: number;
}

export interface PlaybackState {
  isPlaying: boolean;
  track?: Track;
  positionMs?: number;
  volumePercent?: number;
}

/**
 * Abstracao do provider de musica (secção 8). O Gateway e a IA falam
 * apenas com esta interface — nunca diretamente com a API do Spotify.
 */
export interface MusicProvider {
  readonly name: string;
  search(query: string): Promise<Track[]>;
  play(trackId?: string): Promise<PlaybackState>;
  pause(): Promise<PlaybackState>;
  next(): Promise<PlaybackState>;
  previous(): Promise<PlaybackState>;
  setVolume(percent: number): Promise<PlaybackState>;
  getState(): Promise<PlaybackState>;
}
