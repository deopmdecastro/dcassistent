import { randomUUID } from "node:crypto";
import { toolRegistry } from "../ai/tools/registry.js";

export type TimerStatus = "running" | "paused" | "cancelled" | "done";

export interface Timer {
  id: string;
  durationSeconds: number;
  remainingSeconds: number;
  status: TimerStatus;
  label?: string;
}

class InMemoryTimerStore {
  private timers = new Map<string, Timer>();

  create(durationSeconds: number, label?: string): Timer {
    const timer: Timer = {
      id: randomUUID(),
      durationSeconds,
      remainingSeconds: durationSeconds,
      status: "running",
      label,
    };
    this.timers.set(timer.id, timer);
    return timer;
  }

  pause(id: string): Timer | undefined {
    const t = this.timers.get(id);
    if (t) t.status = "paused";
    return t;
  }

  cancel(id: string): Timer | undefined {
    const t = this.timers.get(id);
    if (t) t.status = "cancelled";
    return t;
  }

  list(): Timer[] {
    return [...this.timers.values()];
  }
}

export const timerStore = new InMemoryTimerStore();

/** Tool de temporizadores (secção 11): create_timer. */
export function registerTimerTools(): void {
  toolRegistry.register(
    {
      name: "create_timer",
      description: "Cria e inicia um temporizador.",
      parameters: {
        type: "object",
        properties: { durationSeconds: { type: "number" }, label: { type: "string" } },
        required: ["durationSeconds"],
      },
    },
    async (args) => timerStore.create(Number(args.durationSeconds), args.label ? String(args.label) : undefined),
  );
}
