import express from "express";
import cors from "cors";
import { config } from "./core/config/index.js";
import { logger } from "./core/logging/index.js";
import { apiRouter } from "./api/index.js";
import "./ai/index.js"; // regista tools e inicializa o chat service

const app = express();

app.use(cors());
app.use(express.json());

app.use((req, _res, next) => {
  logger.debug({ method: req.method, path: req.path }, "request");
  next();
});

app.get("/health", (_req, res) => {
  res.json({ status: "ok", service: "dc-gateway", env: config.nodeEnv });
});

app.use("/api", apiRouter);

// Handler de erros central — nunca expor stack traces ou segredos ao cliente.
app.use((err: Error, _req: express.Request, res: express.Response, _next: express.NextFunction) => {
  logger.error({ err }, "Erro nao tratado");
  res.status(500).json({ error: "Erro interno do servidor." });
});

app.listen(config.port, () => {
  logger.info(`DC Gateway a correr na porta ${config.port} (env: ${config.nodeEnv})`);
  logger.info(`AI provider: ${config.ai.provider} | Music provider: ${config.music.provider}`);
});
