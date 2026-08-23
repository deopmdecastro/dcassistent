import Anthropic from "@anthropic-ai/sdk";
import type {
  AiCompletionResult,
  AiProvider,
  ChatMessage,
  ToolCallRequest,
  ToolDefinition,
} from "./types.js";

/**
 * Provider de IA que usa a API da Anthropic (Claude) com tool use nativo.
 * A IA nunca chama servicos diretamente — apenas pede a execucao de
 * "tools", que sao resolvidas pelo ToolRegistry (ver ai/tools).
 */
export class AnthropicAiProvider implements AiProvider {
  readonly name = "anthropic";
  private client: Anthropic;
  private model: string;

  constructor(apiKey: string, model: string) {
    this.client = new Anthropic({ apiKey });
    this.model = model;
  }

  async complete(params: {
    systemPrompt: string;
    messages: ChatMessage[];
    tools: ToolDefinition[];
  }): Promise<AiCompletionResult> {
    const response = await this.client.messages.create({
      model: this.model,
      max_tokens: 1024,
      system: params.systemPrompt,
      messages: params.messages
        .filter((m) => m.role === "user" || m.role === "assistant")
        .map((m) => ({ role: m.role as "user" | "assistant", content: m.content })),
      tools: params.tools.map((t) => ({
        name: t.name,
        description: t.description,
        input_schema: t.parameters as Anthropic.Tool.InputSchema,
      })),
    });

    let text: string | undefined;
    const toolCalls: ToolCallRequest[] = [];

    for (const block of response.content) {
      if (block.type === "text") {
        text = (text ?? "") + block.text;
      } else if (block.type === "tool_use") {
        toolCalls.push({
          id: block.id,
          name: block.name,
          arguments: (block.input as Record<string, unknown>) ?? {},
        });
      }
    }

    return { text, toolCalls: toolCalls.length ? toolCalls : undefined, stopReason: response.stop_reason ?? undefined };
  }
}
