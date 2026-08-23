import { randomUUID } from "node:crypto";

export interface Contact {
  id: string;
  name: string;
  phoneNumber?: string;
}

/** Store em memoria (secção 10 — arquitetura de contactos). */
export class InMemoryContactStore {
  private contacts = new Map<string, Contact>();

  create(input: Omit<Contact, "id">): Contact {
    const contact: Contact = { id: randomUUID(), ...input };
    this.contacts.set(contact.id, contact);
    return contact;
  }

  list(): Contact[] {
    return [...this.contacts.values()];
  }

  findByName(name: string): Contact | undefined {
    const q = name.toLowerCase();
    return [...this.contacts.values()].find((c) => c.name.toLowerCase().includes(q));
  }
}

export const contactStore = new InMemoryContactStore();
