//! DC — Fase 2: sistema operativo da interface, em Slint.
//!
//! Este binário só deve ser usado **depois** de `dc-firmware-rs` (fase
//! 0.1) ter sido validado em hardware real (ver README.md). Reaproveita
//! os drivers de LCD/touch já validados (`dc_firmware_rs::{display,
//! touch_ft6336g}`) e liga-os ao `slint::platform::software_renderer`.
//!
//! IMPORTANTE — tal como a fase 0.1, este ficheiro **não foi compilado
//! nem testado**: não há toolchain Xtensa nem placa física disponíveis
//! no ambiente onde foi escrito. A integração `slint::platform::Platform`
//! para MCU é a parte mais sensível a detalhes de versão do Slint — se
//! o build falhar por causa da API do `software_renderer`, é o primeiro
//! sítio a rever (comparar com os exemplos oficiais `slint-mcu-*` /
//! `examples/mcu-board-support` do repositório do Slint para a versão
//! fixada no Cargo.toml).
//!
//! O que este binário faz:
//! 1. Inicializa LCD + touch (mesmos drivers da fase 0.1).
//! 2. Regista um `slint::platform::Platform` que desenha por software e
//!    envia cada linha alterada para o LCD via SPI.
//! 3. Em cada iteração do loop: lê o touch, converte para evento de
//!    ponteiro Slint, corre `slint::platform::update_timers_and_animations()`,
//!    desenha se necessário.
//! 4. Liga os callbacks do `MainWindow` (open-app / go-home / toggle-*) à
//!    lógica Rust de cada app — por agora só "Controlo" tem estado real
//!    (ligar/desligar, modo); as restantes ainda mostram o placeholder
//!    definido em `ui/main.slint`.

use dc_firmware_rs::{display, pins, touch_ft6336g::Ft6336g};
use display_interface_spi::SPIInterface;
use embedded_graphics::pixelcolor::Rgb565;
use embedded_graphics::prelude::*;
use esp_idf_hal::delay::{Ets, FreeRtos};
use esp_idf_hal::gpio::PinDriver;
use esp_idf_hal::i2c::{I2cConfig, I2cDriver};
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::prelude::*;
use esp_idf_hal::spi::{config::Config as SpiConfig, SpiDeviceDriver, SpiDriver, SpiDriverConfig};
use log::{info, warn};
use mipidsi::{options::ColorOrder, Builder};
use slint::platform::software_renderer::{MinimalSoftwareWindow, RepaintBufferType};
use slint::platform::{Platform, WindowEvent};
use std::cell::RefCell;
use std::rc::Rc;
use std::time::Instant;

slint::include_modules!();

/// Ponte entre o relógio do Slint e o `Instant` do std (disponível porque
/// `esp-idf-hal`/`esp-idf-svc` correm em modo "std").
struct EspPlatform {
    window: Rc<MinimalSoftwareWindow>,
    start: Instant,
}

impl Platform for EspPlatform {
    fn create_window_adapter(
        &self,
    ) -> Result<Rc<dyn slint::platform::WindowAdapter>, slint::PlatformError> {
        Ok(self.window.clone())
    }

    fn duration_since_start(&self) -> core::time::Duration {
        self.start.elapsed()
    }
}

fn main() -> anyhow::Result<()> {
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();
    info!("DC OS (Slint) — a arrancar ({})", pins::BOARD_NAME);

    let peripherals = Peripherals::take()?;

    // --- Backlight ---
    let mut backlight =
        PinDriver::output(unsafe { esp_idf_hal::gpio::AnyOutputPin::new(pins::LCD_PIN_BL) })?;
    backlight.set_high()?;

    // --- LCD (mesmos parâmetros da fase 0.1 — ver notas de color_order/rotação no README) ---
    let spi_driver = SpiDriver::new(
        peripherals.spi2,
        peripherals.pins.gpio12,
        peripherals.pins.gpio11,
        Some(peripherals.pins.gpio13),
        &SpiDriverConfig::new(),
    )?;
    let cs = unsafe { esp_idf_hal::gpio::AnyOutputPin::new(pins::LCD_PIN_CS) };
    let spi_config = SpiConfig::new().baudrate(pins::LCD_SPI_CLOCK_HZ.Hz());
    let spi_device = SpiDeviceDriver::new(spi_driver, Some(cs), &spi_config)?;
    let dc = PinDriver::output(unsafe { esp_idf_hal::gpio::AnyOutputPin::new(pins::LCD_PIN_DC) })?;
    let di = SPIInterface::new(spi_device, dc);

    let mut lcd = Builder::new(mipidsi::models::ILI9341Rgb565, di)
        .display_size(pins::LCD_H_RES_PHYSICAL, pins::LCD_V_RES_PHYSICAL)
        .rotation(display::ROTATION)
        .color_order(ColorOrder::Bgr)
        .reset_pin(mipidsi::NoResetPin)
        .init(&mut Ets)
        .map_err(|e| anyhow::anyhow!("falha a inicializar o LCD: {e:?}"))?;
    info!("LCD inicializado (320x240 paisagem).");

    // --- Touch ---
    let i2c_config = I2cConfig::new().baudrate(pins::I2C_FREQ_HZ.Hz());
    let i2c = I2cDriver::new(
        peripherals.i2c0,
        peripherals.pins.gpio16,
        peripherals.pins.gpio15,
        &i2c_config,
    )?;
    let mut touch = Ft6336g::new(i2c, pins::TOUCH_I2C_ADDR);

    // --- Plataforma Slint (renderer por software, sem GPU) ---
    let window = MinimalSoftwareWindow::new(RepaintBufferType::ReusedBuffer);
    slint::platform::set_platform(Box::new(EspPlatform {
        window: window.clone(),
        start: Instant::now(),
    }))
    .map_err(|e| anyhow::anyhow!("falha a registar a plataforma Slint: {e:?}"))?;

    window.set_size(slint::PhysicalSize::new(320, 240));

    let ui = MainWindow::new()?;
    wire_callbacks(&ui);

    // Buffer de linha reutilizado a cada frame (320 px * Rgb565).
    let mut line_buffer = vec![Rgb565::BLACK; 320];

    let mut last_touch_down = false;

    info!("A entrar no loop principal da UI...");
    loop {
        slint::platform::update_timers_and_animations();

        // --- Input: um único ponto de toque, sem gestos (fase inicial) ---
        match touch.read() {
            Ok(Some(point)) => {
                let pos = slint::LogicalPosition::new(point.x as f32, point.y as f32);
                if !last_touch_down {
                    window.dispatch_event(WindowEvent::PointerPressed {
                        position: pos,
                        button: slint::platform::PointerEventButton::Left,
                    });
                    last_touch_down = true;
                } else {
                    window.dispatch_event(WindowEvent::PointerMoved { position: pos });
                }
            }
            Ok(None) => {
                if last_touch_down {
                    window.dispatch_event(WindowEvent::PointerReleased {
                        position: slint::LogicalPosition::new(0.0, 0.0),
                        button: slint::platform::PointerEventButton::Left,
                    });
                    window.dispatch_event(WindowEvent::PointerExited);
                    last_touch_down = false;
                }
            }
            Err(e) => warn!("erro a ler touch: {e:?}"),
        }

        // --- Render: só desenha as linhas que mudaram (software_renderer trata disso) ---
        window.draw_if_needed(|renderer| {
            renderer.render_by_line(DisplayLineBuffer {
                display: &mut lcd,
                line_buffer: &mut line_buffer,
            });
        });

        FreeRtos::delay_ms(16); // ~60Hz de polling; ajustar depois de medir no hardware real
    }
}

/// Adaptador que recebe linhas do `software_renderer` do Slint e escreve-as
/// no LCD via `embedded-graphics` (`fill_contiguous`). Mantido pequeno de
/// propósito — otimizações de partial-refresh ficam para depois de medir
/// o desempenho real em hardware.
struct DisplayLineBuffer<'a, D> {
    display: &'a mut D,
    line_buffer: &'a mut [Rgb565],
}

impl<'a, D> slint::platform::software_renderer::LineBufferProvider for DisplayLineBuffer<'a, D>
where
    D: DrawTarget<Color = Rgb565>,
{
    type TargetPixel = Rgb565;

    fn process_line(
        &mut self,
        line: usize,
        range: core::ops::Range<usize>,
        render_fn: impl FnOnce(&mut [Self::TargetPixel]),
    ) {
        let buf = &mut self.line_buffer[range.clone()];
        render_fn(buf);
        let _ = self.display.fill_contiguous(
            &embedded_graphics::primitives::Rectangle::new(
                embedded_graphics::prelude::Point::new(range.start as i32, line as i32),
                embedded_graphics::prelude::Size::new(range.len() as u32, 1),
            ),
            buf.iter().copied(),
        );
    }
}

/// Liga os callbacks do `MainWindow` (definidos em ui/main.slint) à lógica
/// Rust de cada app. Nesta fase só "Controlo" tem estado real — as
/// restantes apps do registo mostram o placeholder "por implementar" até
/// terem o seu handler aqui (e o respetivo .slint em ui/apps/).
fn wire_callbacks(ui: &MainWindow) {
    let ui_weak = ui.as_weak();
    ui.on_app_opened(move |id| {
        info!("App aberta: {id}");
        // TODO (fase seguinte): pedir estado inicial ao dc-gateway consoante
        // o id (ex.: GET /api/controlo/state) e popular as propriedades da
        // app correspondente antes dela ficar visível.
        let _ = &ui_weak;
    });

    let ui_weak = ui.as_weak();
    ui.on_controlo_toggle_power(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let new_state = !ui.get_controlo_system_on();
            ui.set_controlo_system_on(new_state);
            info!("Controlo: system_on -> {new_state}");
            // TODO: enviar comando ao dc-gateway (REST/WS) — ver
            // docs/firmware-architecture.md secção 8 para o cliente de rede.
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_controlo_toggle_mode(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let new_state = !ui.get_controlo_auto_mode();
            ui.set_controlo_auto_mode(new_state);
            info!("Controlo: auto_mode -> {new_state}");
        }
    });
}
