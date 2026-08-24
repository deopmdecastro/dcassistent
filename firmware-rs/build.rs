fn main() {
    embuild::espidf::sysenv::output();

    // Compila os ficheiros .slint (ui/main.slint) para Rust. Só é usado
    // pelo binário `dc-os` (a lib gerada é `include!`d em src/bin/dc_os.rs),
    // mas corre sempre — não tem custo relevante nem impacto no binário
    // `dc-firmware-rs` (validação de hardware), que não a inclui.
    slint_build::compile("ui/main.slint").expect("falha a compilar os ficheiros .slint em ui/");
}
