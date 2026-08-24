//! Módulos partilhados entre os dois binários deste crate:
//! - `src/main.rs` (fase 0.1 — validação isolada de hardware, ver README)
//! - `src/bin/dc_os.rs` (fase 2 — UI do sistema Slint)
//!
//! Mantidos aqui para não duplicar drivers/pinout entre os dois.

pub mod display;
pub mod pins;
pub mod touch_ft6336g;
