import { config } from "../../core/config/index.js";
import { AnthropicAiProvider } from "./anthropic-provider.js";
import { MockAiProvider } from "./mock-provider.js";
import type { AiProvider } from "./types.js";

export * from "./types.js";

export function createAiProvider(): AiProvider {
  if (config.ai.provider === "anthropic" && config.ai.apiKey) {
    return new AnthropicAiProvider(config.ai.apiKey, config.ai.model);
  }
  return new MockAiProvider();
}
