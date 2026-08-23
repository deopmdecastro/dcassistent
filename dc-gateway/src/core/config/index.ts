import "dotenv/config";
import { z } from "zod";

/**
 * Schema central de configuracao da DC Gateway.
 * Qualquer variavel de ambiente nova deve ser adicionada aqui,
 * nunca lida diretamente via process.env fora deste modulo.
 */
const ConfigSchema = z.object({
  port: z.coerce.number().default(4000),
  nodeEnv: z.enum(["development", "production", "test"]).default("development"),
  logLevel: z.enum(["fatal", "error", "warn", "info", "debug", "trace"]).default("info"),

  deviceAuthSecret: z.string().optional(),

  ai: z.object({
    provider: z.enum(["anthropic", "mock"]).default("mock"),
    apiKey: z.string().optional(),
    model: z.string().default("claude-sonnet-4-6"),
  }),

  music: z.object({
    provider: z.enum(["spotify", "mock"]).default("mock"),
    spotifyClientId: z.string().optional(),
    spotifyClientSecret: z.string().optional(),
    spotifyRedirectUri: z.string().optional(),
  }),

  calendar: z.object({
    provider: z.enum(["google", "mock"]).default("mock"),
    googleClientId: z.string().optional(),
    googleClientSecret: z.string().optional(),
  }),

  weather: z.object({
    provider: z.enum(["openweather", "mock"]).default("mock"),
    apiKey: z.string().optional(),
  }),

  calls: z.object({
    provider: z.enum(["mock"]).default("mock"),
  }),
});

export type AppConfig = z.infer<typeof ConfigSchema>;

function loadConfig(): AppConfig {
  const raw = {
    port: process.env.PORT,
    nodeEnv: process.env.NODE_ENV,
    logLevel: process.env.LOG_LEVEL,
    deviceAuthSecret: process.env.DEVICE_AUTH_SECRET,
    ai: {
      provider: process.env.AI_PROVIDER,
      apiKey: process.env.ANTHROPIC_API_KEY,
      model: process.env.AI_MODEL,
    },
    music: {
      provider: process.env.MUSIC_PROVIDER,
      spotifyClientId: process.env.SPOTIFY_CLIENT_ID,
      spotifyClientSecret: process.env.SPOTIFY_CLIENT_SECRET,
      spotifyRedirectUri: process.env.SPOTIFY_REDIRECT_URI,
    },
    calendar: {
      provider: process.env.CALENDAR_PROVIDER,
      googleClientId: process.env.GOOGLE_CALENDAR_CLIENT_ID,
      googleClientSecret: process.env.GOOGLE_CALENDAR_CLIENT_SECRET,
    },
    weather: {
      provider: process.env.WEATHER_PROVIDER,
      apiKey: process.env.WEATHER_API_KEY,
    },
    calls: {
      provider: process.env.CALLS_PROVIDER,
    },
  };

  const parsed = ConfigSchema.safeParse(raw);
  if (!parsed.success) {
    // eslint-disable-next-line no-console
    console.error("Configuracao invalida:", parsed.error.flatten().fieldErrors);
    throw new Error("Falha ao carregar configuracao da DC Gateway.");
  }

  // Avisos de fallback para mock quando um provider "real" foi pedido sem credenciais.
  const cfg = parsed.data;
  if (cfg.ai.provider === "anthropic" && !cfg.ai.apiKey) {
    // eslint-disable-next-line no-console
    console.warn("AI_PROVIDER=anthropic mas ANTHROPIC_API_KEY nao foi definida. A usar provider mock.");
    cfg.ai.provider = "mock";
  }
  if (cfg.music.provider === "spotify" && (!cfg.music.spotifyClientId || !cfg.music.spotifyClientSecret)) {
    // eslint-disable-next-line no-console
    console.warn("MUSIC_PROVIDER=spotify mas faltam credenciais. A usar provider mock.");
    cfg.music.provider = "mock";
  }

  return cfg;
}

export const config = loadConfig();
