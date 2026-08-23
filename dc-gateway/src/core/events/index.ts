import { EventEmitter } from "node:events";

/**
 * Eventos centrais da DC, partilhados entre modulos (AI, musica, agenda,
 * chamadas, dispositivo...) sem que estes precisem de se conhecer
 * diretamente. Preparado para futuramente incluir eventos IoT
 * (ex: "iot.device.state_changed") sem alterar o core.
 */
export type DcEventMap = {
  "assistant.state_changed": { state: AssistantState; deviceId?: string };
  "music.state_changed": { deviceId?: string; track?: unknown; isPlaying: boolean };
  "calendar.event_created": { eventId: string };
  "calendar.reminder_due": { reminderId: string };
  "calls.incoming": { contactId: string };
  "device.status": { deviceId: string; status: "online" | "offline" | "connecting" | "error" };
  "notification.created": { id: string; title: string; body?: string };
};

export type AssistantState =
  | "idle"
  | "listening"
  | "processing"
  | "speaking"
  | "music"
  | "calling"
  | "error"
  | "offline";

class TypedEventBus extends EventEmitter {
  emitEvent<K extends keyof DcEventMap>(event: K, payload: DcEventMap[K]): void {
    this.emit(event, payload);
  }

  onEvent<K extends keyof DcEventMap>(event: K, listener: (payload: DcEventMap[K]) => void): void {
    this.on(event, listener);
  }
}

export const eventBus = new TypedEventBus();
