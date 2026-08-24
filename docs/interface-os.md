# DC — Interface como Sistema Operativo

> Este documento define a **arquitetura de interface** da DC: a aplicação deixa de ser um
> dashboard com páginas numeradas e passa a ser um **mini sistema operativo industrial/inteligente**,
> onde a primeira tela é o ambiente principal e cada funcionalidade é uma **aplicação**.

---

## 1. Conceito

A DC **não** se comporta como um dashboard tradicional (várias páginas + menu de navegação numérica).

A ideia principal é a de um **sistema operativo dedicado**:

- A primeira tela é o **ecrã inicial / ambiente do sistema** (launcher).
- **Não existe** navegação por números (Tela 1, Tela 2, Tela 3…).
- **Não existe** menu lateral tradicional para alternar entre páginas.
- As funcionalidades são acedidas através de **aplicações**.
- Cada aplicação tem o seu próprio **ícone, nome e finalidade**.
- A experiência é semelhante a um sistema operativo ou tablet industrial.

## 2. Fluxo de navegação

```
Sistema inicia
      ↓
Ecrã inicial (launcher)
      ↓
Utilizador escolhe uma aplicação
      ↓
Abre o ambiente da aplicação
      ↓
Usa as funções ou definições específicas
      ↓
Volta ao ecrã inicial
```

**Não queremos:**

```
Tela 1 → Tela 2 → Tela 3 → Tela 4
```

**Queremos:**

```
Sistema Operativo → Aplicações → Funcionalidades dentro de cada aplicação
```

## 3. Estrutura final

```
Ecrã de bloqueio / Início
      ↓
Ecrã principal do sistema (launcher)
      ↓
Aplicações
  ├── DC Assistant
  ├── Controlo
  ├── Monitorização
  ├── Alarmes
  ├── Agenda
  ├── Música
  ├── Loja
  └── Definições
      ↓
Cada aplicação possui as suas próprias telas, funções e configurações
```

## 4. Registo de aplicações (App Registry)

Todas as aplicações são registadas numa estrutura única (id, nome, ícone, finalidade,
componente/ecrã e esquema de definições). A **Loja** e a secção **Definições → Aplicações**
leem este mesmo registo — é a fonte de verdade do sistema.

| id            | nome          | ícone      | finalidade                                   | definições próprias        |
|---------------|---------------|------------|----------------------------------------------|----------------------------|
| `assistant`   | DC Assistant  | 🤖         | Assistente inteligente (conversa, estado, alarmes, sugestões, histórico) | Sim (app-level) |
| `controlo`    | Controlo      | 🎛️         | Comandos do sistema (ligar, desligar, manual, automático, setpoints, modos) | Sim (setpoints) |
| `monitorizacao` | Monitorização | 📊       | Visualização em tempo real (sensores, gráficos, histórico) | Sim |
| `alarmes`     | Alarmes       | 🔔         | Central de alarmes e eventos                  | Sim |
| `agenda`      | Agenda        | 📅         | Tarefas, eventos, rotinas, manutenções        | Sim |
| `musica`      | Música        | 🎵         | Entretenimento e controlo multimédia (Spotify) | Sim |
| `loja`        | Loja          | 🛍️         | App Store — expandir o sistema               | Não |
| `definicoes`  | Definições    | ⚙️         | Definições globais do sistema                | Não (é o sistema) |

## 5. Aplicações

### 5.1 DC Assistant

Assistente inteligente do sistema, com **ambiente e definições próprias**.

Funcionalidades:

- Conversa com o assistente
- Consulta do estado do sistema
- Análise de alarmes
- Sugestões automáticas
- Resumo de eventos
- Comandos por texto
- Comandos rápidos
- Histórico de conversas
- Integração com dados e equipamentos do sistema

**Definições do DC Assistant** (pertencem apenas à aplicação, **não** se misturam com as
definições globais):

- Nome do assistente
- Idioma
- Voz
- Personalidade / comportamento
- Nível de notificações
- Permissões para executar comandos
- Integrações disponíveis
- API / modelo de IA utilizado
- Privacidade e histórico

### 5.2 Controlo

Responsável pelos comandos do sistema:

- Ligar / Desligar
- Manual / Automático
- Setpoints
- Modos de funcionamento

### 5.3 Monitorização

Visualização em tempo real:

- Temperaturas
- Humidade
- Sensores
- Estado dos equipamentos
- Gráficos
- Histórico

### 5.4 Alarmes

Central de alarmes e eventos:

- Alarmes ativos
- Histórico
- Prioridade
- Reconhecer alarme
- Silenciar
- Detalhes do problema

### 5.5 Agenda

Gestão de tarefas e eventos:

- Calendário
- Agendamentos
- Rotinas
- Manutenções
- Eventos programados

### 5.6 Música / Spotify

Aplicação dedicada ao entretenimento e controlo multimédia.

### 5.7 Loja de aplicações

Permite expandir o sistema no futuro. Categorias:

- Utilitários
- Automação
- Música
- Produtividade
- Monitorização
- Segurança
- Ferramentas técnicas
- Personalização

Cada aplicação da loja apresenta: nome, ícone, descrição, versão, tamanho,
estado (instalada/não instalada) e botões **instalar / atualizar / remover**.

> Inicialmente a instalação real ainda não existe, mas a arquitetura fica preparada
> para que a loja possa gerir aplicações futuramente (o App Registry é a base).

### 5.8 Definições (globais do sistema)

Aplicação separada que controla tudo o que é **global** no sistema operativo.

Categorias:

- **Sistema** — nome do dispositivo, data e hora, idioma, região, atualizações,
  reiniciar sistema, desligar sistema, informações do dispositivo
- **Interface** — tema claro/escuro, brilho, volume, papel de parede, tamanho dos
  textos, organização das aplicações, preferências do ecrã inicial
- **Rede** — Wi-Fi, Bluetooth, Ethernet, estado da ligação, configuração IP,
  serviços de rede
- **Segurança** — utilizadores, PIN/palavra-passe, bloqueio do sistema, permissões,
  sessões ativas
- **Hardware** — estado do ESP32, sensores, entradas e saídas, relés, dispositivos
  conectados, diagnóstico
- **Aplicações** — aplicações instaladas, permissões das aplicações, notificações,
  armazenamento, atualizações, desinstalar/desativar aplicações

## 6. Princípio das definições

| Tipo                       | Âmbito          | Exemplos                                        |
|----------------------------|-----------------|-------------------------------------------------|
| **Definições globais**     | Todo o sistema  | Wi-Fi, idioma, tema, data e hora, segurança, hardware |
| **Definições da aplicação**| Só uma aplicação | DC Assistant → definições próprias; Spotify → definições próprias; Controlo → setpoints e configurações próprias |

Esta separação mantém o sistema organizado, escalável e semelhante a um verdadeiro
sistema operativo.

## 7. Implementação

- **`frontend-preview/index.html`** — mockup funcional do sistema operativo (launcher + aplicações).
- **`firmware/main/services/ui_manager.c`** — interface LVGL do ESP32-S3, reorganizada por aplicações.
- **`docs/interface-os.md`** — este documento (fonte de verdade da arquitetura de interface).

### 7.1 Frontend (mockup)

- Ecrã inicial = launcher com grelha de aplicações.
- Cada aplicação abre o seu ambiente; botão **voltar** regressa ao launcher.
- Navegação por `openApp(id)` / `goHome()` — sem números de tela.
- Definições do DC Assistant guardadas em namespace próprio (`assistant.`), separadas
  das definições globais (`system.`).

### 7.2 Firmware (LVGL)

- `dc_ui_screen_t` passa a representar **aplicações** (não telas numeradas).
- Home = grelha de ícones de aplicações.
- Cada aplicação tem o seu ecrã e o seu botão "voltar" para o launcher.
- O botão BOOT mantém-se como input secundário/recovery.
