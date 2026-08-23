import { Router } from "express";
import { contactStore, callProvider } from "../calls/index.js";
import { weatherProvider } from "../weather/index.js";
import { noteStore } from "../notes/index.js";
import { alarmStore } from "../alarms/index.js";
import { timerStore } from "../timers/index.js";
import { notificationStore } from "../notifications/index.js";
import { deviceRegistry } from "../device/registry.js";

export const callsRouter = Router();
callsRouter.get("/contacts", (_req, res) => res.json(contactStore.list()));
callsRouter.post("/contacts", (req, res) => {
  const { name, phoneNumber } = req.body ?? {};
  if (typeof name !== "string") return res.status(400).json({ error: "name e obrigatorio." });
  res.status(201).json(contactStore.create({ name, phoneNumber }));
});
callsRouter.get("/history", async (_req, res) => res.json(await callProvider.getHistory()));
callsRouter.post("/start", async (req, res) => {
  const { contactId } = req.body ?? {};
  if (typeof contactId !== "string") return res.status(400).json({ error: "contactId e obrigatorio." });
  res.status(201).json(await callProvider.startCall(contactId));
});
callsRouter.post("/:id/end", async (req, res) => res.json(await callProvider.endCall(req.params.id)));

export const weatherRouter = Router();
weatherRouter.get("/", async (req, res) => {
  const location = String(req.query.location ?? "Porto");
  res.json(await weatherProvider.getCurrent(location));
});

export const notesRouter = Router();
notesRouter.get("/", (_req, res) => res.json(noteStore.list()));
notesRouter.post("/", (req, res) => {
  const { title, content } = req.body ?? {};
  if (typeof title !== "string" || typeof content !== "string") {
    return res.status(400).json({ error: "title e content sao obrigatorios." });
  }
  res.status(201).json(noteStore.create(title, content));
});
notesRouter.patch("/:id", (req, res) => {
  const updated = noteStore.update(req.params.id, req.body ?? {});
  if (!updated) return res.status(404).json({ error: "Nota nao encontrada." });
  res.json(updated);
});
notesRouter.delete("/:id", (req, res) => {
  if (!noteStore.delete(req.params.id)) return res.status(404).json({ error: "Nota nao encontrada." });
  res.status(204).send();
});

export const alarmsRouter = Router();
alarmsRouter.get("/", (_req, res) => res.json(alarmStore.list()));
alarmsRouter.post("/", (req, res) => {
  const { time, label } = req.body ?? {};
  if (typeof time !== "string") return res.status(400).json({ error: "time e obrigatorio." });
  res.status(201).json(alarmStore.create(time, label));
});
alarmsRouter.patch("/:id", (req, res) => {
  const updated = alarmStore.toggle(req.params.id, Boolean(req.body?.enabled));
  if (!updated) return res.status(404).json({ error: "Alarme nao encontrado." });
  res.json(updated);
});
alarmsRouter.delete("/:id", (req, res) => {
  if (!alarmStore.delete(req.params.id)) return res.status(404).json({ error: "Alarme nao encontrado." });
  res.status(204).send();
});

export const timersRouter = Router();
timersRouter.get("/", (_req, res) => res.json(timerStore.list()));
timersRouter.post("/", (req, res) => {
  const { durationSeconds, label } = req.body ?? {};
  if (typeof durationSeconds !== "number") {
    return res.status(400).json({ error: "durationSeconds (number) e obrigatorio." });
  }
  res.status(201).json(timerStore.create(durationSeconds, label));
});
timersRouter.post("/:id/pause", (req, res) => {
  const t = timerStore.pause(req.params.id);
  if (!t) return res.status(404).json({ error: "Temporizador nao encontrado." });
  res.json(t);
});
timersRouter.post("/:id/cancel", (req, res) => {
  const t = timerStore.cancel(req.params.id);
  if (!t) return res.status(404).json({ error: "Temporizador nao encontrado." });
  res.json(t);
});

export const notificationsRouter = Router();
notificationsRouter.get("/", (_req, res) => res.json(notificationStore.list()));
notificationsRouter.post("/:id/read", (req, res) => {
  notificationStore.markRead(req.params.id);
  res.status(204).send();
});

export const devicesRouter = Router();
devicesRouter.get("/", (_req, res) => res.json(deviceRegistry.list()));
devicesRouter.post("/:id/status", (req, res) => {
  const { status, name } = req.body ?? {};
  res.json(deviceRegistry.upsert(req.params.id, { status, name }));
});
