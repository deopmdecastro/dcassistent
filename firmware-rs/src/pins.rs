//! Pinout da placa ES3C28P — portado 1:1 de `firmware/main/app_config.h`.
//!
//! Fonte: datasheet LCDWIKI (CR2025-MI6875 / CR2025-MI6872).
//! IMPORTANTE (herdado do C): confirmar por continuidade no hardware físico
//! antes do primeiro boot — os valores refletem a documentação do
//! fabricante, não uma verificação direta na placa do utilizador.
//!
//! Esta fase (0.1 Rust) usa apenas os pinos de LCD, touch e o LED de
//! estado — é a validação mínima de hardware pedida antes da reescrita
//! da UI em Slint.

pub const BOARD_NAME: &str = "ES3C28P";
pub const BOARD_HAS_TOUCH: bool = true;

// --- LCD — ILI9341V, painel físico 240x320, usado em paisagem 320x240 ---
// via rotação de software (ver src/display.rs).
pub const LCD_H_RES_PHYSICAL: u16 = 240;
pub const LCD_V_RES_PHYSICAL: u16 = 320;
pub const LCD_PIN_CS: i32 = 10;
pub const LCD_PIN_DC: i32 = 46;
pub const LCD_PIN_SCK: i32 = 12;
pub const LCD_PIN_MOSI: i32 = 11;
pub const LCD_PIN_MISO: i32 = 13;
// LCD_RST no C está a -1 (partilhado com o reset do ESP32-S3, sem GPIO
// dedicado) — o driver mipidsi aceita `NoResetPin` nesse caso.
pub const LCD_PIN_BL: i32 = 45;
pub const LCD_SPI_CLOCK_HZ: u32 = 40_000_000;

// --- I2C partilhado: touch (FT6336G) + codec de áudio (ES8311) ---
// Fase 0.1 só usa o touch; o codec fica para a fase de áudio.
pub const I2C_PIN_SDA: i32 = 16;
pub const I2C_PIN_SCL: i32 = 15;
pub const I2C_FREQ_HZ: u32 = 400_000;
pub const TOUCH_PIN_RST: i32 = 18;
pub const TOUCH_PIN_INT: i32 = 17;
pub const TOUCH_I2C_ADDR: u8 = 0x38;

// --- Botão e LED de estado ---
pub const PIN_BTN_BOOT: i32 = 0;
pub const PIN_LED_RGB: i32 = 42; // WS2812B, 1 pino de dados
pub const PIN_BAT_ADC: i32 = 9;
