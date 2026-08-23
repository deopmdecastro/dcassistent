import { randomUUID } from "node:crypto";
import { eventBus } from "../core/events/index.js";

export interface AppNotification {
  id: string;
  title: string;
  body?: string;
  createdAt: string;
  read: boolean;
}

class InMemoryNotificationStore {
  private notifications: AppNotification[] = [];

  create(title: string, body?: string): AppNotification {
    const notification: AppNotification = {
      id: randomUUID(),
      title,
      body,
      createdAt: new Date().toISOString(),
      read: false,
    };
    this.notifications.unshift(notification);
    eventBus.emitEvent("notification.created", { id: notification.id, title, body });
    return notification;
  }

  list(): AppNotification[] {
    return this.notifications;
  }

  markRead(id: string): void {
    const n = this.notifications.find((x) => x.id === id);
    if (n) n.read = true;
  }
}

export const notificationStore = new InMemoryNotificationStore();
