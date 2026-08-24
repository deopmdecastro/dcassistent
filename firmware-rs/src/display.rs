//! Inicialização do LCD ILI9341V (SPI) via `mipidsi`, forçado para
//! paisagem 320x240 por software — o painel físico é 240x320 (retrato).
//!
//! Esta fase só desenha padrões de teste (barras de cor + moldura) para
//! confirmar que o barramento SPI, o backlight e a orientação estão
//! corretos antes de qualquer UI real.

use display_interface_spi::SPIInterface;
use embedded_graphics::{
    pixelcolor::Rgb565,
    prelude::*,
    primitives::{PrimitiveStyle, Rectangle},
};
use esp_idf_hal::delay::Ets;
use esp_idf_hal::gpio::{Output, PinDriver};
use esp_idf_hal::spi::SpiDeviceDriver;
use mipidsi::{models::ILI9341Rgb565, options::Rotation, Builder};

use crate::pins;

pub type LcdDisplay<'d> = mipidsi::Display<
    SPIInterface<SpiDeviceDriver<'d, esp_idf_hal::spi::SpiDriver<'d>>, PinDriver<'d, esp_idf_hal::gpio::AnyIOPin, Output>>,
    ILI9341Rgb565,
    mipidsi::NoResetPin,
>;

/// Desenha um padrão de teste simples: 4 barras de cor + moldura branca.
/// Serve só para confirmar visualmente (foto/vídeo do utilizador) que o
/// display está vivo e na orientação certa — 320 de largura, 240 de altura.
pub fn draw_test_pattern<D>(display: &mut D)
where
    D: DrawTarget<Color = Rgb565>,
{
    let colors = [Rgb565::RED, Rgb565::GREEN, Rgb565::BLUE, Rgb565::YELLOW];
    let bar_w = 320 / colors.len() as i32;
    for (i, color) in colors.iter().enumerate() {
        let _ = Rectangle::new(
            Point::new(i as i32 * bar_w, 0),
            Size::new(bar_w as u32, 240),
        )
        .into_styled(PrimitiveStyle::with_fill(*color))
        .draw(display);
    }
    // Moldura branca de 2px para confirmar que não há corte/desalinhamento
    // nas bordas (comum quando a offset de rotação do controlador está errada).
    let _ = Rectangle::new(Point::new(0, 0), Size::new(320, 240))
        .into_styled(PrimitiveStyle::with_stroke(Rgb565::WHITE, 2))
        .draw(display);
}

/// Nota de integração: a construção completa do `Builder` (SPI, DC, CS,
/// reset) depende dos tipos concretos gerados pelo `Peripherals::take()`
/// no `main.rs` — por isso a função de setup fica lá, não aqui, para
/// evitar genéricos excessivos nesta fase de validação. Este módulo
/// fornece a lógica de desenho e a documentação de orientação/rotação.
pub const ROTATION: Rotation = Rotation::Deg90;
