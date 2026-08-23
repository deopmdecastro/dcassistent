export type CallStatus = "idle" | "ringing" | "connected" | "ended" | "failed";

export interface CallSession {
  id: string;
  contactId: string;
  status: CallStatus;
  startedAt: string;
  endedAt?: string;
}

/**
 * Abstracao de chamadas (secção 10). Preparada para diferentes
 * fornecedores futuros (VoIP, integracao telefone, video) sem alterar
 * o resto da aplicacao.
 */
export interface CallProvider {
  readonly name: string;
  startCall(contactId: string): Promise<CallSession>;
  endCall(callId: string): Promise<CallSession>;
  getHistory(): Promise<CallSession[]>;
}

export class MockCallProvider implements CallProvider {
  readonly name = "mock";
  private sessions = new Map<string, CallSession>();

  async startCall(contactId: string): Promise<CallSession> {
    const session: CallSession = {
      id: crypto.randomUUID(),
      contactId,
      status: "ringing",
      startedAt: new Date().toISOString(),
    };
    this.sessions.set(session.id, session);
    return session;
  }

  async endCall(callId: string): Promise<CallSession> {
    const session = this.sessions.get(callId);
    if (!session) throw new Error("Chamada nao encontrada.");
    session.status = "ended";
    session.endedAt = new Date().toISOString();
    return session;
  }

  async getHistory(): Promise<CallSession[]> {
    return [...this.sessions.values()].sort((a, b) => b.startedAt.localeCompare(a.startedAt));
  }
}

export const callProvider: CallProvider = new MockCallProvider();
