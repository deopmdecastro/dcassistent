import { randomUUID } from "node:crypto";
import { eventBus } from "../../core/events/index.js";

export interface Reminder {
  id: string;
  text: string;
  dueAt: string; // ISO
  done: boolean;
}

class InMemoryReminderStore {
  private reminders = new Map<string, Reminder>();

  create(text: string, dueAt: string): Reminder {
    const reminder: Reminder = { id: randomUUID(), text, dueAt, done: false };
    this.reminders.set(reminder.id, reminder);
    return reminder;
  }

  complete(id: string): Reminder | undefined {
    const reminder = this.reminders.get(id);
    if (!reminder) return undefined;
    reminder.done = true;
    return reminder;
  }

  listPending(): Reminder[] {
    return [...this.reminders.values()]
      .filter((r) => !r.done)
      .sort((a, b) => a.dueAt.localeCompare(b.dueAt));
  }

  /** A chamar periodicamente por um scheduler para disparar lembretes vencidos. */
  checkDue(): Reminder[] {
    const now = new Date().toISOString();
    const due = this.listPending().filter((r) => r.dueAt <= now);
    for (const r of due) {
      eventBus.emitEvent("calendar.reminder_due", { reminderId: r.id });
    }
    return due;
  }
}

export const reminderStore = new InMemoryReminderStore();
