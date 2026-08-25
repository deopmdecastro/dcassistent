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
//! 4. Liga os callbacks do `MainWindow` (open-app / go-home / toggle-* /
//!    pin-press / wallpaper-select / etc.) à lógica Rust de cada app.

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

    // --- LCD ---
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

    // --- Plataforma Slint ---
    let window = MinimalSoftwareWindow::new(RepaintBufferType::ReusedBuffer);
    slint::platform::set_platform(Box::new(EspPlatform {
        window: window.clone(),
        start: Instant::now(),
    }))
    .map_err(|e| anyhow::anyhow!("falha a registar a plataforma Slint: {e:?}"))?;

    window.set_size(slint::PhysicalSize::new(320, 240));

    let ui = MainWindow::new()?;
    wire_callbacks(&ui);

    let mut line_buffer = vec![Rgb565::BLACK; 320];
    let mut last_touch_down = false;

    info!("A entrar no loop principal da UI...");
    loop {
        slint::platform::update_timers_and_animations();

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

        window.draw_if_needed(|renderer| {
            renderer.render_by_line(DisplayLineBuffer {
                display: &mut lcd,
                line_buffer: &mut line_buffer,
            });
        });

        FreeRtos::delay_ms(16);
    }
}

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

/// Estado de PIN gerido pelo Rust — buffer temporário, PIN guardado e
/// modo atual (unlock / setup / setup-confirm).
struct PinState {
    buffer: String,
    saved_pin: String,
    mode: String,  // "unlock" | "setup" | "setup-confirm"
    setup_first: String,
}

/// Liga os callbacks do `MainWindow` à lógica Rust de cada app.
fn wire_callbacks(ui: &MainWindow) {
    // --- app-opened: log + inicialização por app ---
    let ui_weak = ui.as_weak();
    ui.on_app_opened(move |id| {
        info!("App aberta: {id}");
        let _ = &ui_weak;
    });

    // --- Controlo ---
    let ui_weak = ui.as_weak();
    ui.on_controlo_toggle_power(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let new_state = !ui.get_controlo_system_on();
            ui.set_controlo_system_on(new_state);
            info!("Controlo: system_on -> {new_state}");
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

    // --- Assistant ---
    let ui_weak = ui.as_weak();
    ui.on_assistant_set_tab(move |t| {
        if let Some(ui) = ui_weak.upgrade() {
            ui.set_assistant_tab(t as i32);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_assistant_send(move |msg| {
        info!("Assistant: mensagem recebida: {msg}");
        if let Some(ui) = ui_weak.upgrade() {
            // TODO: contactar dc-gateway se gateway ligado; por agora mock
            let reply = mock_reply(&msg);
            // Adicionar mensagem do utilizador + resposta do bot
            let mut msgs = ui.get_assistant_messages().clone();
            msgs.push(ChatMessage { role: "user".into(), text: msg.to_string() });
            msgs.push(ChatMessage { role: "bot".into(), text: reply });
            ui.set_assistant_messages(msgs);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_assistant_quick(move |cmd| {
        info!("Assistant: comando rápido: {cmd}");
        if let Some(ui) = ui_weak.upgrade() {
            let reply = mock_reply(&cmd);
            let mut msgs = ui.get_assistant_messages().clone();
            msgs.push(ChatMessage { role: "user".into(), text: cmd.to_string() });
            msgs.push(ChatMessage { role: "bot".into(), text: reply });
            ui.set_assistant_messages(msgs);
        }
    });

    // --- Alarmes ---
    let ui_weak = ui.as_weak();
    ui.on_alarmes_set_tab(move |t| {
        if let Some(ui) = ui_weak.upgrade() {
            ui.set_alarmes_tab(t as i32);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_alarmes_acknowledge(move |i| {
        info!("Alarmes: reconhecer alarme #{i}");
        if let Some(ui) = ui_weak.upgrade() {
            let mut alarms = ui.get_alarms().clone();
            if (i as usize) < alarms.len() {
                alarms[i as usize].ativo = false;
                ui.set_alarms(alarms);
            }
        }
    });

    let _ = ui.as_weak();
    ui.on_alarmes_silence(move |i| {
        info!("Alarmes: silenciar alarme #{i}");
    });

    // --- Agenda ---
    let _ = ui.as_weak();
    ui.on_agenda_add_event(move |text| {
        info!("Agenda: novo evento: {text}");
        // TODO: adicionar à lista de eventos (precisa de propriedade in-out)
    });

    // --- Música ---
    let ui_weak = ui.as_weak();
    ui.on_musica_toggle_play(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let new_state = !ui.get_musica_playing();
            ui.set_musica_playing(new_state);
            info!("Música: playing -> {new_state}");
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_musica_next_track(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let tracks = [
                ("Blinding Lights", "The Weeknd"),
                ("Starboy", "The Weeknd"),
                ("Levitating", "Dua Lipa"),
            ];
            let current = ui.get_musica_track_title().to_string();
            let idx = tracks.iter().position(|(t, _)| *t == current).unwrap_or(0);
            let next = (idx + 1) % tracks.len();
            ui.set_musica_track_title(tracks[next].0.into());
            ui.set_musica_track_artist(tracks[next].1.into());
            info!("Música: próxima faixa -> {}", tracks[next].0);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_musica_prev_track(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let tracks = [
                ("Blinding Lights", "The Weeknd"),
                ("Starboy", "The Weeknd"),
                ("Levitating", "Dua Lipa"),
            ];
            let current = ui.get_musica_track_title().to_string();
            let idx = tracks.iter().position(|(t, _)| *t == current).unwrap_or(0);
            let prev = if idx == 0 { tracks.len() - 1 } else { idx - 1 };
            ui.set_musica_track_title(tracks[prev].0.into());
            ui.set_musica_track_artist(tracks[prev].1.into());
            info!("Música: faixa anterior -> {}", tracks[prev].0);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_musica_set_tab(move |t| {
        if let Some(ui) = ui_weak.upgrade() {
            ui.set_musica_tab(t as i32);
        }
    });

    // --- Loja ---
    let ui_weak = ui.as_weak();
    ui.on_loja_set_tab(move |t| {
        if let Some(ui) = ui_weak.upgrade() {
            ui.set_loja_tab(t as i32);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_loja_toggle_install(move |id| {
        info!("Loja: toggle install para app {id}");
        if let Some(ui) = ui_weak.upgrade() {
            let mut catalog = ui.get_loja_catalog().clone();
            for app in &mut catalog {
                if app.id == id.to_string() {
                    app.installed = !app.installed;
                    info!("Loja: {} -> {}", app.name, if app.installed { "instalada" } else { "removida" });
                }
            }
            ui.set_loja_catalog(catalog);
        }
    });

    // --- Lock screen / PIN ---
    let ui_weak = ui.as_weak();
    let pin_state = Rc::new(RefCell::new(PinState {
        buffer: String::new(),
        saved_pin: String::new(),
        mode: "unlock".into(),
        setup_first: String::new(),
    }));
    let pin_state_clone = pin_state.clone();
    ui.on_pin_press(move |k| {
        let Some(ui) = ui_weak.upgrade() else { return };
        let mut ps = pin_state_clone.borrow_mut();

        match k.to_string().as_str() {
            "cancel" => {
                ps.buffer.clear();
                ps.setup_first.clear();
                ui.set_lock_visible(false);
            }
            "back" => {
                ps.buffer.pop();
                ui.set_lock_pin_length(ps.buffer.len() as i32);
            }
            d => {
                if ps.buffer.len() < 4 {
                    ps.buffer.push_str(d);
                    ui.set_lock_pin_length(ps.buffer.len() as i32);
                    if ps.buffer.len() == 4 {
                        handle_pin_complete(&ui, &mut *ps);
                    }
                }
            }
        }
    });

    // --- Wallpaper picker ---
    let ui_weak = ui.as_weak();
    ui.on_wallpaper_select(move |id| {
        if let Some(ui) = ui_weak.upgrade() {
            ui.set_wallpaper_current(id.to_string());
            info!("Wallpaper: {id}");
            ui.set_wallpaper_visible(false);
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_wallpaper_close(move || {
        if let Some(ui) = ui_weak.upgrade() {
            ui.set_wallpaper_visible(false);
        }
    });
}

fn handle_pin_complete(ui: &MainWindow, ps: &mut PinState) {
    match ps.mode.as_str() {
        "setup" => {
            ps.setup_first = ps.buffer.clone();
            ps.buffer.clear();
            ps.mode = "setup-confirm".into();
            ui.set_lock_pin_length(0);
            ui.set_lock_title("Confirma o PIN".into());
            ui.set_lock_hint("Repete os 4 dígitos".into());
        }
        "setup-confirm" => {
            if ps.buffer == ps.setup_first {
                ps.saved_pin = ps.buffer.clone();
                ps.buffer.clear();
                ps.mode = "unlock".into();
                ui.set_lock_pin_length(0);
                ui.set_lock_visible(false);
                info!("PIN definido com sucesso");
            } else {
                ps.buffer.clear();
                ps.setup_first.clear();
                ps.mode = "setup".into();
                ui.set_lock_pin_length(0);
                ui.set_lock_shake(true);
                ui.set_lock_title("PINs não coincidem".into());
                ui.set_lock_hint("Tenta de novo".into());
                info!("PINs não coincidem — reintentar");
            }
        }
        _ => {
            // unlock
            if ps.buffer == ps.saved_pin || ps.saved_pin.is_empty() {
                ps.buffer.clear();
                ui.set_lock_pin_length(0);
                ui.set_lock_visible(false);
                info!("PIN correto — desbloqueado");
            } else {
                ps.buffer.clear();
                ui.set_lock_pin_length(0);
                ui.set_lock_shake(true);
                info!("PIN incorreto");
            }
        }
    }
}

fn mock_reply(text: &str) -> String {
    let t = text.to_lowercase();
    if t.contains("estado") {
        return "Tudo operacional. Rede ligada, temperatura 23.4 °C, humidade 61%, 1 alarme ativo.".into();
    }
    if t.contains("alarme") {
        return "1 alarme ativo (prioridade alta): temperatura elevada no controlo (28.1 °C).".into();
    }
    if t.contains("evento") || t.contains("agenda") {
        return "Hoje: 3 eventos — 09:00 Reunião, 11:30 Verificação, 15:00 Manutenção.".into();
    }
    if t.contains("sugest") {
        return "Sugiro baixar o setpoint do controlo em 1 °C e verificar a ventilação.".into();
    }
    if t.contains("música") || t.contains("musica") {
        return "A tocar \"Blinding Lights\" de The Weeknd.".into();
    }
    if t.contains("olá") || t.contains("ola") || t.contains("bom dia") {
        return "Olá! Sou a DC. Posso dar-te o estado do sistema, alarmes, agenda ou sugestões.".into();
    }
    format!("Recebido: \"{text}\". (Resposta simulada — ativa o Gateway nas Definições do assistente para respostas reais.)")
}
