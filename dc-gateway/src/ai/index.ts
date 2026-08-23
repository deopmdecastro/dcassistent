import { createAiProvider } from "./providers/index.js";
import { ChatService } from "./chat/chat-service.js";
import { registerCalendarTools } from "../calendar/index.js";
import { registerMusicTools } from "../music/index.js";
import { registerCallTools } from "../calls/index.js";
import { registerWeatherTools } from "../weather/index.js";
import { registerNoteTools } from "../notes/index.js";
import { registerAlarmTools } from "../alarms/index.js";
import { registerTimerTools } from "../timers/index.js";

export { toolRegistry } from "./tools/registry.js";
export type { ChatReply } from "./chat/chat-service.js";

/**
 * Ponto unico onde todos os modulos registam as suas tools na IA
 * (secção 7). Adicionar um novo modulo (ex: IoT no futuro) significa
 * apenas chamar aqui a sua funcao `registerXTools()` — o nucleo da IA
 * (ChatService, ToolRegistry) nao precisa de ser alterado.
 */
function registerAllTools(): void {
  registerCalendarTools();
  registerMusicTools();
  registerCallTools();
  registerWeatherTools();
  registerNoteTools();
  registerAlarmTools();
  registerTimerTools();
  // Futuro: registerIotTools();
}

registerAllTools();

export const chatService = new ChatService(createAiProvider());
