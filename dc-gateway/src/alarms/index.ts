import { randomUUID } from "node:crypto";
import { toolRegistry } from "../ai/tools/registry.js";

export interface Alarm {
  id: string;
  time: string; // "HH:mm"
  label?: string;
  enabled: boolean;
}

class InMemoryAlarmStore {
  private alarms = new Map<string, Alarm>();

  create(time: string, label?: string): Alarm {
    const alarm: Alarm = { id: randomUUID(), time, label, enabled: true };
    this.alarms.set(alarm.id, alarm);
    return alarm;
  }

  toggle(id: string, enabled: boolean): Alarm | undefined {
    const alarm = this.alarms.get(id);
    if (!alarm) return undefined;
    alarm.enabled = enabled;
    return alarm;
  }

  delete(id: string): boolean {
    return this.alarms.delete(id);
  }

  list(): Alarm[] {
    return [...this.alarms.values()].sort((a, b) => a.time.localeCompare(b.time));
  }
}

export const alarmStore = new InMemoryAlarmStore();

/** Tool de alarmes (secção 11): create_alarm. */
export function registerAlarmTools(): void {
  toolRegistry.register(
    {
      name: "create_alarm",
      description: 'Cria um alarme para uma hora especifica (formato "HH:mm").',
      parameters: {
        type: "object",
        properties: { time: { type: "string" }, label: { type: "string" } },
        required: ["time"],
      },
    },
    async (args) => alarmStore.create(String(args.time), args.label ? String(args.label) : undefined),
  );
}
