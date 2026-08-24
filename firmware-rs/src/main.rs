//! DC — Fase 0.1 (Rust): validação isolada de hardware.
//!
//! Objetivo desta fase (pedido explícito antes da reescrita da UI em
//! Slint): confirmar que o display, o touch e o boot do dispositivo
//! funcionam, na placa ES3C28P (ESP32-S3 + ILI9341 + FT6336G).
//!
//! Não há UI, não há Wi-Fi, não há Gateway aqui — só drivers +
//! `embedded-graphics` para desenhar um padrão de teste e ler o touch.
//! Este crate (`firmware-rs/`) é a base sobre a qual a UI Slint (fase
//! seguinte) vai ser construída; substitui progressivamente `firmware/`
//! (C/ESP-IDF/LVGL), que fica intacto até a nova base estar validada.
//!
//! IMPORTANTE: este código não foi compilado nem testado em hardware
//! real nesta sessão — não há placa física nem toolchain Xtensa
//! disponíveis no ambiente onde foi escrito. Precisa de `cargo build
//! --release` + `espflash flash` numa máquina com `espup` instalado
//! antes de ser considerado validado. Ver `firmware-rs/README.md`.

use dc_firmware_rs::{display, pins, touch_ft6336g};

use display_interface_spi::SPIInterface;
use embedded_graphics::pixelcolor::Rgb565;
use esp_idf_hal::delay::{Ets, FreeRtos};
use esp_idf_hal::gpio::PinDriver;
use esp_idf_hal::i2c::{I2cConfig, I2cDriver};
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::prelude::*;
use esp_idf_hal::spi::{config::Config as SpiConfig, SpiDeviceDriver, SpiDriver, SpiDriverConfig};
use log::{error, info, warn};
use mipidsi::{options::ColorOrder, Builder};
use touch_ft6336g::Ft6336g;
use ws2812_esp32_rmt_driver::driver::color::LedPixelColorGrb24;
use ws2812_esp32_rmt_driver::LedPixelEsp32Rmt;

// Estado do LED de status (WS2812B) durante a validação — dá feedback
// visual sem precisar de porta série ligada.
const LED_BOOTING: (u8, u8, u8) = (40, 40, 0); // amarelo: a inicializar
const LED_LCD_OK: (u8, u8, u8) = (0, 40, 0); // verde: LCD ok, à espera de touch
const LED_TOUCH_EVENT: (u8, u8, u8) = (0, 0, 60); // azul: toque detetado
const LED_ERROR: (u8, u8, u8) = (60, 0, 0); // vermelho: falha de inicialização

fn main() -> anyhow::Result<()> {
    // Obrigatório em todos os binários esp-idf-sys "std".
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    info!("DC firmware-rs — Fase 0.1: validação de hardware ({})", pins::BOARD_NAME);

    let peripherals = Peripherals::take()?;

    let mut status_led = LedPixelEsp32Rmt::<LedPixelColorGrb24, _>::new(
        peripherals.rmt.channel0,
        pins::PIN_LED_RGB as u32,
    )?;
    set_led(&mut status_led, LED_BOOTING)?;

    // --- Backlight (PWM simplificado para nível fixo nesta fase) ---
    let mut backlight = PinDriver::output(unsafe {
        esp_idf_hal::gpio::AnyOutputPin::new(pins::LCD_PIN_BL)
    })?;
    backlight.set_high()?;

    // --- Barramento SPI do LCD ---
    let spi_driver = SpiDriver::new(
        peripherals.spi2,
        peripherals.pins.gpio12, // SCK
        peripherals.pins.gpio11, // MOSI
        Some(peripherals.pins.gpio13), // MISO
        &SpiDriverConfig::new(),
    )?;
    let cs = unsafe { esp_idf_hal::gpio::AnyOutputPin::new(pins::LCD_PIN_CS) };
    let spi_config = SpiConfig::new().baudrate(pins::LCD_SPI_CLOCK_HZ.Hz());
    let spi_device = SpiDeviceDriver::new(spi_driver, Some(cs), &spi_config)?;

    let dc = PinDriver::output(unsafe { esp_idf_hal::gpio::AnyOutputPin::new(pins::LCD_PIN_DC) })?;
    let di = SPIInterface::new(spi_device, dc);

    info!("A inicializar LCD ILI9341 (paisagem 320x240 via rotação de software)...");
    let lcd_result = Builder::new(mipidsi::models::ILI9341Rgb565, di)
        .display_size(pins::LCD_H_RES_PHYSICAL, pins::LCD_V_RES_PHYSICAL)
        .rotation(display::ROTATION)
        .color_order(ColorOrder::Bgr) // ajustar para Rgb se as cores saírem trocadas na validação
        .reset_pin(mipidsi::NoResetPin) // LCD_RST partilhado com o reset do ESP32-S3 (ver app_config.h original)
        .init(&mut Ets);

    let mut lcd = match lcd_result {
        Ok(lcd) => {
            info!("LCD inicializado com sucesso.");
            set_led(&mut status_led, LED_LCD_OK)?;
            lcd
        }
        Err(e) => {
            error!("Falha a inicializar o LCD: {:?}", e);
            set_led(&mut status_led, LED_ERROR)?;
            return Err(anyhow::anyhow!("falha na inicialização do LCD"));
        }
    };

    display::draw_test_pattern(&mut lcd);
    info!("Padrão de teste desenhado — confirma visualmente: 4 barras de cor + moldura branca, sem corte nas bordas.");

    // --- I2C partilhado (touch FT6336G) ---
    let i2c_config = I2cConfig::new().baudrate(pins::I2C_FREQ_HZ.Hz());
    let i2c = I2cDriver::new(
        peripherals.i2c0,
        peripherals.pins.gpio16, // SDA
        peripherals.pins.gpio15, // SCL
        &i2c_config,
    );

    let mut touch = match i2c {
        Ok(i2c) => Some(Ft6336g::new(i2c, pins::TOUCH_I2C_ADDR)),
        Err(e) => {
            warn!("Falha a abrir o barramento I2C do touch: {:?}", e);
            None
        }
    };

    if touch.is_none() {
        warn!("Touch indisponível — validação continua só com LCD. Verificar fiação I2C (SDA=GPIO16, SCL=GPIO15).");
    }

    info!("Boot validado. A entrar no loop de leitura de touch (Ctrl+C / reset para sair)...");

    loop {
        if let Some(t) = touch.as_mut() {
            match t.read() {
                Ok(Some(point)) => {
                    info!("Touch: x={} y={}", point.x, point.y);
                    let _ = set_led(&mut status_led, LED_TOUCH_EVENT);
                }
                Ok(None) => {
                    let _ = set_led(&mut status_led, LED_LCD_OK);
                }
                Err(e) => {
                    warn!("Erro a ler touch: {:?}", e);
                }
            }
        }
        FreeRtos::delay_ms(50);
    }
}

fn set_led<T>(led: &mut LedPixelEsp32Rmt<LedPixelColorGrb24, T>, (r, g, b): (u8, u8, u8)) -> anyhow::Result<()>
where
    T: esp_idf_hal::rmt::RmtChannel,
{
    use ws2812_esp32_rmt_driver::driver::color::LedPixelColor;
    let pixel = LedPixelColorGrb24::new_with_rgb(r, g, b);
    led.write_blocking(std::iter::once(pixel))?;
    Ok(())
}
