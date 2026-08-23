import { Router } from "express";
import { musicProvider, getAuthorizeUrl } from "../music/index.js";
import { spotifyTokenStore } from "../music/spotify/spotify-auth.js";
import { randomUUID } from "node:crypto";

export const musicRouter = Router();

musicRouter.get("/state", async (_req, res) => {
  res.json(await musicProvider.getState());
});

musicRouter.get("/search", async (req, res) => {
  const query = String(req.query.q ?? "");
  res.json(await musicProvider.search(query));
});

musicRouter.post("/play", async (req, res) => {
  res.json(await musicProvider.play(req.body?.trackId));
});

musicRouter.post("/pause", async (_req, res) => {
  res.json(await musicProvider.pause());
});

musicRouter.post("/next", async (_req, res) => {
  res.json(await musicProvider.next());
});

musicRouter.post("/previous", async (_req, res) => {
  res.json(await musicProvider.previous());
});

musicRouter.post("/volume", async (req, res) => {
  res.json(await musicProvider.setVolume(Number(req.body?.percent ?? 50)));
});

// ---- Spotify OAuth (secção 8: autenticacao, ligacao da conta) ----
musicRouter.get("/spotify/login", (_req, res) => {
  const state = randomUUID();
  res.redirect(getAuthorizeUrl(state));
});

musicRouter.get("/spotify/callback", async (req, res) => {
  const code = String(req.query.code ?? "");
  if (!code) return res.status(400).send("Codigo de autorizacao em falta.");
  try {
    await spotifyTokenStore.exchangeCode(code);
    res.send("Conta Spotify ligada com sucesso. Podes fechar esta janela.");
  } catch {
    res.status(500).send("Falha ao ligar a conta Spotify.");
  }
});
