import type { ToolDefinition } from "../providers/types.js";

export type ToolHandler = (args: Record<string, unknown>) => Promise<unknown>;

export interface RegisteredTool {
  definition: ToolDefinition;
  handler: ToolHandler;
}

/**
 * Registo central de Tools da IA (secção 7 do prompt mestre).
 *
 * A IA nunca fala diretamente com Spotify, Agenda, Chamadas, etc.
 * Cada servico regista aqui as suas capacidades como uma "tool" com
 * nome, descricao e schema de parametros. A IA pede a execucao pelo
 * nome; o registry despacha para o handler correto.
 *
 * Extensibilidade: no futuro, `iot.turn_on` pode ser registada aqui
 * sem qualquer alteracao ao nucleo da IA (secção 22).
 */
export class ToolRegistry {
  private tools = new Map<string, RegisteredTool>();

  register(definition: ToolDefinition, handler: ToolHandler): void {
    if (this.tools.has(definition.name)) {
      throw new Error(`Tool "${definition.name}" ja esta registada.`);
    }
    this.tools.set(definition.name, { definition, handler });
  }

  list(): ToolDefinition[] {
    return [...this.tools.values()].map((t) => t.definition);
  }

  async execute(name: string, args: Record<string, unknown>): Promise<unknown> {
    const tool = this.tools.get(name);
    if (!tool) {
      throw new Error(`Tool "${name}" nao encontrada.`);
    }
    return tool.handler(args);
  }

  has(name: string): boolean {
    return this.tools.has(name);
  }
}

export const toolRegistry = new ToolRegistry();
