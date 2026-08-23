import { describe, expect, it, beforeEach } from "vitest";
import { ToolRegistry } from "../src/ai/tools/registry.js";
import { InMemoryCalendarStore } from "../src/calendar/events/store.js";

describe("ToolRegistry", () => {
  let registry: ToolRegistry;

  beforeEach(() => {
    registry = new ToolRegistry();
  });

  it("regista e lista tools", () => {
    registry.register(
      { name: "ping", description: "test", parameters: { type: "object", properties: {} } },
      async () => "pong",
    );
    expect(registry.list()).toHaveLength(1);
    expect(registry.has("ping")).toBe(true);
  });

  it("nao permite registar a mesma tool duas vezes", () => {
    const def = { name: "dup", description: "test", parameters: { type: "object", properties: {} } };
    registry.register(def, async () => null);
    expect(() => registry.register(def, async () => null)).toThrow();
  });

  it("executa a tool registada e devolve o resultado", async () => {
    registry.register(
      { name: "echo", description: "test", parameters: { type: "object", properties: {} } },
      async (args) => args.value,
    );
    const result = await registry.execute("echo", { value: "ola" });
    expect(result).toBe("ola");
  });

  it("lanca erro ao executar tool inexistente", async () => {
    await expect(registry.execute("nao-existe", {})).rejects.toThrow();
  });
});

describe("InMemoryCalendarStore", () => {
  it("cria, lista e apaga eventos", () => {
    const store = new InMemoryCalendarStore();
    const event = store.create({ title: "Reuniao", startsAt: "2030-01-01T10:00:00.000Z" });
    expect(store.get(event.id)).toEqual(event);

    const upcoming = store.listUpcoming();
    expect(upcoming.some((e) => e.id === event.id)).toBe(true);

    expect(store.delete(event.id)).toBe(true);
    expect(store.get(event.id)).toBeUndefined();
  });
});
