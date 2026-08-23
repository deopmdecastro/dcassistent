export interface WeatherSnapshot {
  location: string;
  temperatureC: number;
  condition: string;
  humidityPercent?: number;
}

/** Abstracao (secção 11) — permite trocar de fornecedor de meteorologia sem tocar no resto do sistema. */
export interface WeatherProvider {
  readonly name: string;
  getCurrent(location: string): Promise<WeatherSnapshot>;
}

export class MockWeatherProvider implements WeatherProvider {
  readonly name = "mock";

  async getCurrent(location: string): Promise<WeatherSnapshot> {
    return {
      location,
      temperatureC: 22,
      condition: "Ceu limpo",
      humidityPercent: 55,
    };
  }
}
