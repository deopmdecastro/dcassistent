import type { MusicProvider, PlaybackState, Track } from "./provider.js";

const SAMPLE_TRACKS: Track[] = [
  { id: "mock-1", title: "Blinding Lights", artist: "The Weeknd", album: "After Hours", durationMs: 200_000 },
  { id: "mock-2", title: "Starboy", artist: "The Weeknd", album: "Starboy", durationMs: 230_000 },
];

export class MockMusicProvider implements MusicProvider {
  readonly name = "mock";
  private state: PlaybackState = { isPlaying: false, volumePercent: 70 };

  async search(query: string): Promise<Track[]> {
    const q = query.toLowerCase();
    return SAMPLE_TRACKS.filter(
      (t) => t.title.toLowerCase().includes(q) || t.artist.toLowerCase().includes(q),
    );
  }

  async play(trackId?: string): Promise<PlaybackState> {
    const track = trackId ? SAMPLE_TRACKS.find((t) => t.id === trackId) : (this.state.track ?? SAMPLE_TRACKS[0]);
    this.state = { ...this.state, isPlaying: true, track, positionMs: 0 };
    return this.state;
  }

  async pause(): Promise<PlaybackState> {
    this.state = { ...this.state, isPlaying: false };
    return this.state;
  }

  async next(): Promise<PlaybackState> {
    const idx = SAMPLE_TRACKS.findIndex((t) => t.id === this.state.track?.id);
    const track = SAMPLE_TRACKS[(idx + 1) % SAMPLE_TRACKS.length];
    this.state = { ...this.state, track, positionMs: 0, isPlaying: true };
    return this.state;
  }

  async previous(): Promise<PlaybackState> {
    const idx = SAMPLE_TRACKS.findIndex((t) => t.id === this.state.track?.id);
    const track = SAMPLE_TRACKS[(idx - 1 + SAMPLE_TRACKS.length) % SAMPLE_TRACKS.length];
    this.state = { ...this.state, track, positionMs: 0, isPlaying: true };
    return this.state;
  }

  async setVolume(percent: number): Promise<PlaybackState> {
    this.state = { ...this.state, volumePercent: Math.max(0, Math.min(100, percent)) };
    return this.state;
  }

  async getState(): Promise<PlaybackState> {
    return this.state;
  }
}
