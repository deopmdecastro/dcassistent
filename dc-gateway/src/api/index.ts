import { Router } from "express";
import { assistantRouter } from "./assistant-routes.js";
import { musicRouter } from "./music-routes.js";
import { calendarRouter } from "./calendar-routes.js";
import {
  callsRouter,
  weatherRouter,
  notesRouter,
  alarmsRouter,
  timersRouter,
  notificationsRouter,
  devicesRouter,
} from "./misc-routes.js";

/**
 * Organizacao de endpoints (secção 21):
 * /api/assistant, /api/music, /api/calendar, /api/calls, /api/weather,
 * /api/notes, /api/alarms, /api/timers, /api/notifications, /api/devices.
 * Ver dc-gateway/API.md para documentacao detalhada.
 */
export const apiRouter = Router();

apiRouter.use("/assistant", assistantRouter);
apiRouter.use("/music", musicRouter);
apiRouter.use("/calendar", calendarRouter);
apiRouter.use("/calls", callsRouter);
apiRouter.use("/weather", weatherRouter);
apiRouter.use("/notes", notesRouter);
apiRouter.use("/alarms", alarmsRouter);
apiRouter.use("/timers", timersRouter);
apiRouter.use("/notifications", notificationsRouter);
apiRouter.use("/devices", devicesRouter);
