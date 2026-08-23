import { childLogger } from "../../core/logging/index.js";
import { assistantState } from "../../core/state/index.js";
import { conversationMemory } from "../memory/index.js";
import { buildSystemPrompt } from "../prompts/system-prompt.js";
import { toolRegistry } from "../tools/registry.js";
import type { AiProvider, ChatMessage } from "../providers/types.js";

const log = childLogger("ai-chat");
const MAX_TOOL_ROUNDTRIPS = 4;

export interface ChatReply {
  text: string;
  toolsUsed: string[];
}

/**
 * Servico de chat da IA (secção 6.1 e 7).
 *
 * Fluxo: utilizador -> IA -> (opcional) tool call -> resultado -> IA -> resposta final.
 * A IA nunca acede diretamente aos servicos; o loop abaixo e o unico
 * lugar que liga o "cerebro" (provider) as "maos" (tools).
 */
export class ChatService {
  constructor(private provider: AiProvider) {}

  async sendMessage(sessionId: string, userText: string): Promise<ChatReply> {
    assistantState.set("processing");
    conversationMemory.append(sessionId, { role: "user", content: userText });

    const toolsUsed: string[] = [];
    let finalText = "";

    try {
      for (let round = 0; round < MAX_TOOL_ROUNDTRIPS; round++) {
        const history = conversationMemory.getHistory(sessionId);
        const result = await this.provider.complete({
          systemPrompt: buildSystemPrompt(),
          messages: history,
          tools: toolRegistry.list(),
        });

        if (result.toolCalls && result.toolCalls.length > 0) {
          for (const call of result.toolCalls) {
            log.info({ tool: call.name }, "A executar tool pedida pela IA");
            toolsUsed.push(call.name);
            try {
              const toolResult = await toolRegistry.execute(call.name, call.arguments);
              conversationMemory.append(sessionId, {
                role: "tool",
                content: JSON.stringify(toolResult),
                toolCallId: call.id,
                toolName: call.name,
              });
            } catch (err) {
              log.error({ err, tool: call.name }, "Erro ao executar tool");
              conversationMemory.append(sessionId, {
                role: "tool",
                content: JSON.stringify({ error: (err as Error).message }),
                toolCallId: call.id,
                toolName: call.name,
              });
            }
          }
          // Continua o loop para a IA processar os resultados das tools.
          continue;
        }

        finalText = result.text ?? "Desculpa, nao consegui gerar uma resposta.";
        break;
      }

      if (!finalText) {
        finalText = "Desculpa, demorei demasiado a processar esse pedido. Podes tentar reformular?";
      }

      conversationMemory.append(sessionId, { role: "assistant", content: finalText });
      assistantState.set("speaking");
      return { text: finalText, toolsUsed };
    } catch (err) {
      log.error({ err }, "Erro no ciclo de chat da IA");
      assistantState.set("error");
      throw err;
    } finally {
      // Depois de "falar", a DC volta ao estado idle (o dispositivo pode
      // sobrepor este estado durante a reproducao real do audio TTS).
      setTimeout(() => assistantState.set("idle"), 0);
    }
  }

  getHistory(sessionId: string): ChatMessage[] {
    return conversationMemory.getHistory(sessionId);
  }

  clearHistory(sessionId: string): void {
    conversationMemory.clear(sessionId);
  }
}
