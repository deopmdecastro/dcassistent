import { Router } from "express";
import { calendarStore, reminderStore } from "../calendar/index.js";

export const calendarRouter = Router();

calendarRouter.get("/events", (req, res) => {
  const { from, to } = req.query;
  if (typeof from === "string" && typeof to === "string") {
    return res.json(calendarStore.listBetween(from, to));
  }
  res.json(calendarStore.listUpcoming());
});

calendarRouter.post("/events", (req, res) => {
  const { title, startsAt, endsAt, description } = req.body ?? {};
  if (typeof title !== "string" || typeof startsAt !== "string") {
    return res.status(400).json({ error: "title e startsAt sao obrigatorios." });
  }
  res.status(201).json(calendarStore.create({ title, startsAt, endsAt, description }));
});

calendarRouter.patch("/events/:id", (req, res) => {
  const updated = calendarStore.update(req.params.id, req.body ?? {});
  if (!updated) return res.status(404).json({ error: "Evento nao encontrado." });
  res.json(updated);
});

calendarRouter.delete("/events/:id", (req, res) => {
  const deleted = calendarStore.delete(req.params.id);
  if (!deleted) return res.status(404).json({ error: "Evento nao encontrado." });
  res.status(204).send();
});

calendarRouter.get("/reminders", (_req, res) => {
  res.json(reminderStore.listPending());
});

calendarRouter.post("/reminders", (req, res) => {
  const { text, dueAt } = req.body ?? {};
  if (typeof text !== "string" || typeof dueAt !== "string") {
    return res.status(400).json({ error: "text e dueAt sao obrigatorios." });
  }
  res.status(201).json(reminderStore.create(text, dueAt));
});
