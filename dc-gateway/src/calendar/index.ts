import { toolRegistry } from "../ai/tools/registry.js";
import { calendarStore } from "./events/store.js";
import { reminderStore } from "./reminders/store.js";

export { calendarStore } from "./events/store.js";
export type { CalendarEvent } from "./events/store.js";
export { reminderStore } from "./reminders/store.js";
export type { Reminder } from "./reminders/store.js";

/**
 * Regista as tools de agenda para a IA (secção 9):
 * get_calendar_events, create_calendar_event.
 * Exemplo de fluxo: "DC, o que tenho amanha?" -> get_calendar_events(date).
 */
export function registerCalendarTools(): void {
  toolRegistry.register(
    {
      name: "get_calendar_events",
      description:
        "Obtem os eventos da agenda entre duas datas ISO (from, to). Se omitido, devolve os proximos eventos.",
      parameters: {
        type: "object",
        properties: {
          from: { type: "string", description: "Data/hora inicial ISO 8601." },
          to: { type: "string", description: "Data/hora final ISO 8601." },
        },
      },
    },
    async (args) => {
      if (args.from && args.to) {
        return calendarStore.listBetween(String(args.from), String(args.to));
      }
      return calendarStore.listUpcoming();
    },
  );

  toolRegistry.register(
    {
      name: "create_calendar_event",
      description: "Cria um novo evento na agenda.",
      parameters: {
        type: "object",
        properties: {
          title: { type: "string" },
          startsAt: { type: "string", description: "Data/hora ISO 8601." },
          endsAt: { type: "string", description: "Data/hora ISO 8601 (opcional)." },
          description: { type: "string" },
        },
        required: ["title", "startsAt"],
      },
    },
    async (args) => {
      return calendarStore.create({
        title: String(args.title),
        startsAt: String(args.startsAt),
        endsAt: args.endsAt ? String(args.endsAt) : undefined,
        description: args.description ? String(args.description) : undefined,
      });
    },
  );

  toolRegistry.register(
    {
      name: "create_reminder",
      description: "Cria um lembrete para uma data/hora especifica.",
      parameters: {
        type: "object",
        properties: {
          text: { type: "string" },
          dueAt: { type: "string", description: "Data/hora ISO 8601." },
        },
        required: ["text", "dueAt"],
      },
    },
    async (args) => reminderStore.create(String(args.text), String(args.dueAt)),
  );
}
