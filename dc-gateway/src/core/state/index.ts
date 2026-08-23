import { eventBus, type AssistantState } from "../events/index.js";

/**
 * Estado global da assistente (secção 15 do prompt mestre).
 * Deve poder ser sincronizado entre o Gateway e o dispositivo ESP32-S3.
 */
class AssistantStateStore {
  private state: AssistantState = "idle";
  private lastChangedAt: string = new Date().toISOString();

  get(): { state: AssistantState; lastChangedAt: string } {
    return { state: this.state, lastChangedAt: this.lastChangedAt };
  }

  set(next: AssistantState, deviceId?: string): void {
    this.state = next;
    this.lastChangedAt = new Date().toISOString();
    eventBus.emitEvent("assistant.state_changed", { state: next, deviceId });
  }
}

export const assistantState = new AssistantStateStore();
export type { AssistantState };
