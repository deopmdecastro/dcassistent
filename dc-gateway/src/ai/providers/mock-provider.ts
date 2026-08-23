import type { AiCompletionResult, AiProvider, ChatMessage, ToolDefinition } from "./types.js";

/**
 * Provider mock, usado quando nao ha credenciais reais configuradas
 * (secção 27 do prompt mestre — permite desenvolvimento local sem
 * chaves de API).
 */
export class MockAiProvider implements AiProvider {
  readonly name = "mock";

  async complete(params: {
    systemPrompt: string;
    messages: ChatMessage[];
    tools: ToolDefinition[];
  }): Promise<AiCompletionResult> {
    const lastUserMessage = [...params.messages].reverse().find((m) => m.role === "user");
    const text = lastUserMessage
      ? `[mock-ai] Recebi: "${lastUserMessage.content}". (Configura AI_PROVIDER=anthropic e ANTHROPIC_API_KEY para respostas reais.)`
      : "[mock-ai] Ola! Sou a DC em modo de desenvolvimento (sem IA real configurada).";

    return { text, stopReason: "end_turn" };
  }
}
