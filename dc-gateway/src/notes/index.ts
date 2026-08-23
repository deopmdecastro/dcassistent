import { randomUUID } from "node:crypto";
import { toolRegistry } from "../ai/tools/registry.js";

export interface Note {
  id: string;
  title: string;
  content: string;
  createdAt: string;
  updatedAt: string;
}

class InMemoryNoteStore {
  private notes = new Map<string, Note>();

  create(title: string, content: string): Note {
    const now = new Date().toISOString();
    const note: Note = { id: randomUUID(), title, content, createdAt: now, updatedAt: now };
    this.notes.set(note.id, note);
    return note;
  }

  update(id: string, patch: Partial<Pick<Note, "title" | "content">>): Note | undefined {
    const existing = this.notes.get(id);
    if (!existing) return undefined;
    const updated = { ...existing, ...patch, updatedAt: new Date().toISOString() };
    this.notes.set(id, updated);
    return updated;
  }

  delete(id: string): boolean {
    return this.notes.delete(id);
  }

  list(): Note[] {
    return [...this.notes.values()].sort((a, b) => b.updatedAt.localeCompare(a.updatedAt));
  }
}

export const noteStore = new InMemoryNoteStore();

/** Tools de notas (secção 11): create_note. */
export function registerNoteTools(): void {
  toolRegistry.register(
    {
      name: "create_note",
      description: "Cria uma nova nota pessoal.",
      parameters: {
        type: "object",
        properties: { title: { type: "string" }, content: { type: "string" } },
        required: ["title", "content"],
      },
    },
    async (args) => noteStore.create(String(args.title), String(args.content)),
  );
}
