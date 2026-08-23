import pino from "pino";
import { config } from "../config/index.js";

/**
 * Logger central da aplicacao.
 * Redige automaticamente campos comuns de credenciais para evitar
 * que segredos apareçam em logs (ver secção 19 do prompt mestre).
 */
export const logger = pino({
  level: config.logLevel,
  redact: {
    paths: [
      "*.apiKey",
      "*.api_key",
      "*.token",
      "*.password",
      "*.secret",
      "*.authorization",
      "req.headers.authorization",
      "*.clientSecret",
      "*.client_secret",
    ],
    censor: "[REDACTED]",
  },
  transport:
    config.nodeEnv === "development"
      ? { target: "pino-pretty", options: { colorize: true, translateTime: "HH:MM:ss" } }
      : undefined,
});

export function childLogger(namespace: string) {
  return logger.child({ module: namespace });
}
