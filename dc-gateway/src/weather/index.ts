import { toolRegistry } from "../ai/tools/registry.js";
import { MockWeatherProvider, type WeatherProvider } from "./provider.js";

function createWeatherProvider(): WeatherProvider {
  // TODO: quando WEATHER_PROVIDER=openweather e houver WEATHER_API_KEY,
  // instanciar um OpenWeatherProvider aqui (nao implementado nesta fase).
  return new MockWeatherProvider();
}

export const weatherProvider = createWeatherProvider();

export function registerWeatherTools(): void {
  toolRegistry.register(
    {
      name: "get_weather",
      description: "Obtem a meteorologia atual para uma localizacao.",
      parameters: {
        type: "object",
        properties: {
          location: { type: "string", description: "Nome da cidade/localizacao." },
        },
        required: ["location"],
      },
    },
    async (args) => {
      const location = String(args.location ?? "Porto");
      return weatherProvider.getCurrent(location);
    },
  );
}
