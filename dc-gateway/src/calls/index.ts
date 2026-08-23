import { toolRegistry } from "../ai/tools/registry.js";
import { eventBus } from "../core/events/index.js";
import { contactStore } from "./contacts/store.js";
import { callProvider } from "./call-management/provider.js";

export { contactStore } from "./contacts/store.js";
export { callProvider } from "./call-management/provider.js";
export type { Contact } from "./contacts/store.js";
export type { CallSession, CallStatus } from "./call-management/provider.js";

/**
 * Regista a tool make_call (secção 10). Ex: make_call("Joao").
 */
export function registerCallTools(): void {
  toolRegistry.register(
    {
      name: "make_call",
      description: "Inicia uma chamada para um contacto pelo nome.",
      parameters: {
        type: "object",
        properties: { contactName: { type: "string" } },
        required: ["contactName"],
      },
    },
    async (args) => {
      const contact = contactStore.findByName(String(args.contactName));
      if (!contact) {
        return { error: `Contacto "${args.contactName}" nao encontrado.` };
      }
      const session = await callProvider.startCall(contact.id);
      eventBus.emitEvent("calls.incoming", { contactId: contact.id });
      return session;
    },
  );
}
