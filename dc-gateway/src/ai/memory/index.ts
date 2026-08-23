import type { ChatMessage } from "../providers/types.js";

/**
 * Memoria/historico de conversa por sessao (secção 6.1).
 * Em memoria nesta fase; preparado para persistir em base de dados
 * (tabelas Conversation/Message da secção 20) numa fase seguinte.
 */
class ConversationMemory {
  private sessions = new Map<string, ChatMessage[]>();
  private readonly maxMessagesPerSession = 50;

  getHistory(sessionId: string): ChatMessage[] {
    return this.sessions.get(sessionId) ?? [];
  }

  append(sessionId: string, message: ChatMessage): void {
    const history = this.sessions.get(sessionId) ?? [];
    history.push(message);
    if (history.length > this.maxMessagesPerSession) {
      history.splice(0, history.length - this.maxMessagesPerSession);
    }
    this.sessions.set(sessionId, history);
  }

  clear(sessionId: string): void {
    this.sessions.delete(sessionId);
  }
}

export const conversationMemory = new ConversationMemory();
