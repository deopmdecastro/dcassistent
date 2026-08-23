import { Router } from "express";
import { chatService } from "../ai/index.js";
import { assistantState } from "../core/state/index.js";

export const assistantRouter = Router();

assistantRouter.get("/state", (_req, res) => {
  res.json(assistantState.get());
});

assistantRouter.post("/message", async (req, res) => {
  const { sessionId, text } = req.body ?? {};
  if (typeof sessionId !== "string" || typeof text !== "string" || !text.trim()) {
    return res.status(400).json({ error: "sessionId e text (string) sao obrigatorios." });
  }
  try {
    const reply = await chatService.sendMessage(sessionId, text);
    res.json(reply);
  } catch (err) {
    res.status(500).json({ error: "Falha ao processar a mensagem.", details: (err as Error).message });
  }
});

assistantRouter.get("/conversations/:sessionId", (req, res) => {
  res.json(chatService.getHistory(req.params.sessionId));
});

assistantRouter.delete("/conversations/:sessionId", (req, res) => {
  chatService.clearHistory(req.params.sessionId);
  res.status(204).send();
});
