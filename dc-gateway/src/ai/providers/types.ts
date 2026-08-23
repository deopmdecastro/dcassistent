/**
 * Abstracao de provider de IA. Qualquer fornecedor (Anthropic, mock,
 * futuramente outro) implementa esta interface, para que o resto da
 * aplicacao nunca dependa diretamente de um SDK especifico.
 */

export type ChatRole = "user" | "assistant" | "system" | "tool";

export interface ChatMessage {
  role: ChatRole;
  content: string;
  /** Preenchido quando role === "tool": resultado de uma tool call anterior. */
  toolCallId?: string;
  toolName?: string;
}

export interface ToolDefinition {
  name: string;
  description: string;
  /** JSON schema dos parametros da tool. */
  parameters: Record<string, unknown>;
}

export interface ToolCallRequest {
  id: string;
  name: string;
  arguments: Record<string, unknown>;
}

export interface AiCompletionResult {
  /** Texto de resposta, se o modelo respondeu diretamente. */
  text?: string;
  /** Tool(s) que o modelo pediu para executar, se aplicavel. */
  toolCalls?: ToolCallRequest[];
  /** Motivo de paragem, para debugging/telemetria. */
  stopReason?: string;
}

export interface AiProvider {
  readonly name: string;
  complete(params: {
    systemPrompt: string;
    messages: ChatMessage[];
    tools: ToolDefinition[];
  }): Promise<AiCompletionResult>;
}
