import { randomUUID } from "node:crypto";

export interface CalendarEvent {
  id: string;
  title: string;
  startsAt: string; // ISO
  endsAt?: string; // ISO
  description?: string;
}

/**
 * Store em memoria para desenvolvimento local (secção 27 — mocks).
 * Numa fase seguinte, isto passa a ser persistido em base de dados
 * (secção 20) e/ou sincronizado com Google/Apple Calendar (secção 9).
 */
export class InMemoryCalendarStore {
  private events = new Map<string, CalendarEvent>();

  create(input: Omit<CalendarEvent, "id">): CalendarEvent {
    const event: CalendarEvent = { id: randomUUID(), ...input };
    this.events.set(event.id, event);
    return event;
  }

  update(id: string, patch: Partial<Omit<CalendarEvent, "id">>): CalendarEvent | undefined {
    const existing = this.events.get(id);
    if (!existing) return undefined;
    const updated = { ...existing, ...patch };
    this.events.set(id, updated);
    return updated;
  }

  delete(id: string): boolean {
    return this.events.delete(id);
  }

  get(id: string): CalendarEvent | undefined {
    return this.events.get(id);
  }

  listBetween(fromIso: string, toIso: string): CalendarEvent[] {
    return [...this.events.values()]
      .filter((e) => e.startsAt >= fromIso && e.startsAt <= toIso)
      .sort((a, b) => a.startsAt.localeCompare(b.startsAt));
  }

  listUpcoming(limit = 10): CalendarEvent[] {
    const now = new Date().toISOString();
    return [...this.events.values()]
      .filter((e) => e.startsAt >= now)
      .sort((a, b) => a.startsAt.localeCompare(b.startsAt))
      .slice(0, limit);
  }
}

export const calendarStore = new InMemoryCalendarStore();
