import { eventBus } from "../core/events/index.js";

export type DeviceStatus = "online" | "offline" | "connecting" | "error";

export interface DeviceInfo {
  id: string;
  name?: string;
  status: DeviceStatus;
  lastSeenAt: string;
}

/**
 * Canais de comunicacao ESP32 <-> Gateway (secção 14).
 * Para a V1 usa-se REST para pedidos pontuais (chat, tools) e um canal
 * de eventos (aqui simplificado; pode evoluir para WebSocket/MQTT) para
 * estado em tempo real: device_status, screen_state, touch_event,
 * audio_state, assistant_state, notifications, music_state, calendar_state.
 */
class DeviceRegistry {
  private devices = new Map<string, DeviceInfo>();

  upsert(id: string, patch: Partial<Omit<DeviceInfo, "id">>): DeviceInfo {
    const existing = this.devices.get(id);
    const device: DeviceInfo = {
      id,
      name: patch.name ?? existing?.name,
      status: patch.status ?? existing?.status ?? "connecting",
      lastSeenAt: new Date().toISOString(),
    };
    this.devices.set(id, device);
    eventBus.emitEvent("device.status", { deviceId: id, status: device.status });
    return device;
  }

  get(id: string): DeviceInfo | undefined {
    return this.devices.get(id);
  }

  list(): DeviceInfo[] {
    return [...this.devices.values()];
  }
}

export const deviceRegistry = new DeviceRegistry();
