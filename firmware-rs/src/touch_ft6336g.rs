//! Driver mínimo para o controlador de touch FT6336G (I2C).
//!
//! Só implementa o necessário para a validação de hardware: ler se há
//! um dedo no ecrã e a coordenada (x, y) já convertida para a orientação
//! paisagem 320x240. Sem gestos, sem multi-touch — isso fica para a fase
//! da UI Slint, que vai usar este driver como base.

use embedded_hal::i2c::I2c;

const REG_TD_STATUS: u8 = 0x02; // número de pontos de toque (0..=2)
const REG_P1_XH: u8 = 0x03; // 4 bytes: XH, XL, YH, YL do primeiro ponto

#[derive(Debug, Clone, Copy)]
pub struct TouchPoint {
    /// Coordenada já rodada para paisagem 320x240 (0,0 = canto superior
    /// esquerdo em paisagem).
    pub x: u16,
    pub y: u16,
}

pub struct Ft6336g<I2C> {
    i2c: I2C,
    addr: u8,
}

impl<I2C, E> Ft6336g<I2C>
where
    I2C: I2c<Error = E>,
    E: core::fmt::Debug,
{
    pub fn new(i2c: I2C, addr: u8) -> Self {
        Self { i2c, addr }
    }

    /// Lê o estado atual do touch. `Ok(None)` = sem dedo no ecrã.
    /// Faz a conversão retrato (240x320, nativo do FT6336G nesta placa)
    /// -> paisagem (320x240), coerente com `display::rotate_to_landscape`.
    pub fn read(&mut self) -> Result<Option<TouchPoint>, E> {
        let mut status = [0u8; 1];
        self.i2c.write_read(self.addr, &[REG_TD_STATUS], &mut status)?;
        let points = status[0] & 0x0F;
        if points == 0 {
            return Ok(None);
        }

        let mut buf = [0u8; 4];
        self.i2c.write_read(self.addr, &[REG_P1_XH], &mut buf)?;
        let raw_x = (((buf[0] & 0x0F) as u16) << 8) | buf[1] as u16;
        let raw_y = (((buf[2] & 0x0F) as u16) << 8) | buf[3] as u16;

        // Painel físico é 240 (x) x 320 (y) em retrato. Rotação de 90°
        // para paisagem 320x240: x_land = y_port, y_land = (240-1) - x_port.
        // Ajustar o sinal/eixo aqui se a orientação sair espelhada na
        // validação em hardware real (é o ponto exato a confirmar nesta fase).
        let x_land = raw_y;
        let y_land = 240u16.saturating_sub(1).saturating_sub(raw_x);

        Ok(Some(TouchPoint { x: x_land, y: y_land }))
    }
}
