# DC firmware-rs — Fase 0.1: validação de hardware

Novo firmware em **Rust + esp-idf-hal/esp-idf-svc**, que vai substituir
`firmware/` (C/ESP-IDF/LVGL) progressivamente. Escolhido em vez do backend
C++ MCU do Slint por ser o caminho oficialmente suportado do Slint para
microcontroladores.

Este crate, nesta fase, **não tem UI nem Wi-Fi** — só valida três coisas,
conforme pedido antes de avançar para a interface Slint:

1. **Boot** do ESP32-S3 (placa ES3C28P) em Rust/esp-idf.
2. **LCD** ILI9341 via SPI, forçado para paisagem 320×240 por rotação de
   software (o painel físico é 240×320 em retrato) — desenha 4 barras de
   cor + moldura branca.
3. **Touch** FT6336G via I2C partilhado — imprime coordenadas no log
   série sempre que deteta um dedo no ecrã.

Um LED de estado (WS2812B, GPIO42) dá feedback visual sem precisar de
porta série: amarelo = a inicializar, verde = LCD ok / sem toque, azul =
toque detetado, vermelho = falha de inicialização.

## Estado

⚠️ **Não compilado nem testado em hardware real.** Este código foi escrito
sem acesso a um toolchain Xtensa nem à placa física — precisa de ser
validado numa máquina com `espup` instalado e a placa ES3C28P ligada por
USB-C antes de seguir para a fase seguinte (UI Slint).

Dois pontos deliberadamente marcados no código para confirmar durante essa
validação (ambos são "sinais visuais óbvios", não bugs escondidos):

- **`color_order(ColorOrder::Bgr)`** em `src/main.rs` — se as cores das
  barras de teste saírem trocadas (vermelho ↔ azul), mudar para `Rgb`.
- **Conversão retrato→paisagem** em `src/touch_ft6336g.rs` (`read()`) — se
  o ponto de toque aparecer espelhado ou rodado 90° na direção errada,
  ajustar essa fórmula (está isolada e comentada exatamente para isso).

## Build

```bash
# Uma vez, numa máquina de desenvolvimento (não neste ambiente):
cargo install espup espflash ldproxy
espup install
. $HOME/export-esp.sh   # ou o equivalente gerado pelo espup

cd firmware-rs
cargo build --release
cargo espflash flash --release --monitor
```

## Próximos passos (depois de validado)

1. Confirmar visualmente o padrão de teste e os logs de touch na placa real.
2. Ajustar `color_order` / rotação do touch se necessário (ver acima).
3. Adicionar `esp-idf-svc` Wi-Fi (`net_manager` equivalente) — só depois
   disso entra a UI Slint (`docs/interface-os.md`), que corre sobre esta
   base já validada.
4. `firmware/` (C) mantém-se intacto até esta base Rust estar validada em
   hardware e a UI Slint tiver o launcher + pelo menos uma aplicação a
   funcionar.
