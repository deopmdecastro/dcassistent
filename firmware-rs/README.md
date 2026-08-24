# DC firmware-rs

Novo firmware em **Rust + esp-idf-hal/esp-idf-svc**, que vai substituir
`firmware/` (C/ESP-IDF/LVGL) progressivamente. Escolhido em vez do backend
C++ MCU do Slint por ser o caminho oficialmente suportado do Slint para
microcontroladores.

Dois binários no mesmo crate, módulos de hardware partilhados via `src/lib.rs`:

| Binário | Ficheiro | Estado | Faz o quê |
|---|---|---|---|
| `dc-firmware-rs` | `src/main.rs` | ⚠️ não testado em hardware | **Fase 0.1** — valida só LCD/touch/boot, sem UI |
| `dc-os` | `src/bin/dc_os.rs` | ⚠️ não testado em hardware | **Fase 2** — sistema operativo da interface, em Slint (launcher + apps) |

## Fase 0.1 — `dc-firmware-rs` (validação de hardware)

Sem UI nem Wi-Fi — só valida três coisas, conforme pedido antes de avançar
para a interface Slint:

1. **Boot** do ESP32-S3 (placa ES3C28P) em Rust/esp-idf.
2. **LCD** ILI9341 via SPI, forçado para paisagem 320×240 por rotação de
   software (o painel físico é 240×320 em retrato) — desenha 4 barras de
   cor + moldura branca.
3. **Touch** FT6336G via I2C partilhado — imprime coordenadas no log
   série sempre que deteta um dedo no ecrã.

Um LED de estado (WS2812B, GPIO42) dá feedback visual sem precisar de
porta série: amarelo = a inicializar, verde = LCD ok / sem toque, azul =
toque detetado, vermelho = falha de inicialização.

**Corre isto primeiro, sempre, antes de tocar em `dc-os`.**

## Fase 2 — `dc-os` (UI Slint)

Sistema operativo da interface, construído nativamente em **Slint**
(`ui/*.slint`), sem depender do LVGL:

- `ui/main.slint` — janela raiz, navegação Home ↔ App por `current-app`
  (sem números de tela), tal como pedido.
- `ui/launcher.slint` — Home/Desktop, grelha de ícones das apps do
  `ui/app-registry.slint` (espelha a tabela de `docs/interface-os.md` §4).
- `ui/apps/*.slint` — uma app por ficheiro, com o seu próprio ecrã.
  Implementadas nesta fase: **Controlo** (com estado real), **Monitorização**
  e **Definições** (só categorias globais — ver nota abaixo). As restantes
  do registo (`assistant`, `alarmes`, `agenda`, `musica`, `loja`) ainda
  mostram um placeholder "por implementar" definido em `main.slint`, para
  o launcher nunca abrir um ecrã em branco.
- `ui/theme.slint` — paleta/tipografia/raios, pensada para aproximar a
  identidade da versão Web dentro das limitações do ecrã embarcado.
- `src/bin/dc_os.rs` — ponte entre o `slint::platform::software_renderer`
  e o LCD (via `fill_contiguous`), pump de touch → eventos de ponteiro
  Slint, e os handlers Rust dos callbacks de cada app.

**Definições globais vs. definições de app** — estrutural, não é só
convenção: `ui/apps/definicoes.slint` só tem as 6 categorias globais
(Sistema, Interface, Rede, Segurança, Hardware, Aplicações); definições
de uma app específica (ex. setpoints do Controlo) vivem dentro do próprio
ecrã dessa app (`ui/apps/controlo.slint`), nunca em `definicoes.slint`.

## Estado

⚠️ **Nada aqui foi compilado nem testado em hardware real.** Este código foi
escrito sem acesso a um toolchain Xtensa nem à placa física — precisa de
ser validado numa máquina com `espup` instalado e a placa ES3C28P ligada
por USB-C.

Pontos deliberadamente marcados no código para confirmar durante essa
validação (sinais visuais óbvios, não bugs escondidos):

- **`color_order(ColorOrder::Bgr)`** (`src/main.rs` e `src/bin/dc_os.rs`) —
  se as cores saírem trocadas (vermelho ↔ azul), mudar para `Rgb`.
- **Conversão retrato→paisagem** do touch (`src/touch_ft6336g.rs`,
  `read()`) — se o ponto tocado aparecer espelhado ou rodado 90° na
  direção errada, ajustar essa fórmula (está isolada e comentada
  exatamente para isso).
- **API do `slint::platform::software_renderer`** (`src/bin/dc_os.rs`) —
  é a parte mais sensível a mudanças de versão do Slint; se o build
  falhar aqui, comparar com os exemplos oficiais `mcu-board-support` do
  repositório do Slint para a versão fixada no `Cargo.toml`.
- **Cadência do loop** (`FreeRtos::delay_ms(16)` em `dc_os.rs`) — valor de
  partida (~60Hz de polling de touch), ajustar depois de medir FPS reais
  do renderer por software no hardware.

## Build

```bash
# Uma vez, numa máquina de desenvolvimento (não neste ambiente):
cargo install espup espflash ldproxy
espup install
. $HOME/export-esp.sh   # ou o equivalente gerado pelo espup

cd firmware-rs
cargo build --release --bin dc-firmware-rs   # fase 0.1: valida hardware primeiro
cargo espflash flash --release --bin dc-firmware-rs --monitor

# só depois de confirmar LCD/touch na fase 0.1:
cargo build --release --bin dc-os
cargo espflash flash --release --bin dc-os --monitor
```

## Próximos passos

1. Confirmar visualmente o padrão de teste e os logs de touch com
   `dc-firmware-rs` na placa real (fase 0.1).
2. Ajustar `color_order` / rotação do touch se necessário (ver acima).
3. Flashar `dc-os` e confirmar: launcher aparece, ícones respondem ao
   toque, "Controlo" liga/desliga e muda de modo, "voltar" regressa
   sempre ao launcher.
4. Implementar os ecrãs em falta (`assistant`, `alarmes`, `agenda`,
   `musica`, `loja`) — cada um com o seu `.slint` em `ui/apps/` e o
   handler correspondente em `dc_os.rs`.
5. Ligar `dc-os` ao `dc-gateway` via Wi-Fi (`esp-idf-svc` Wi-Fi + cliente
   HTTP/WS) para dados reais em vez dos placeholders atuais.
6. `firmware/` (C) mantém-se intacto até esta base Rust/Slint estar
   validada em hardware e cobrir pelo menos as apps essenciais.

