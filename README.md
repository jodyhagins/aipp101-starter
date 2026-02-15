# AI++ 101 Starter Kit: Chat Application

A complete, working C++ chat application using the OpenRouter API. This is your starting point for the AI++ 101 workshop.

## Quick Start

### Prerequisites

- C++20 compiler (GCC 13+ or Clang 17+)
- CMake 3.27+
- Ninja build system (make requires changing presets)
- OpenSSL development libraries
- ccache (optional, for faster rebuilds)

### Setup

1. Copy `.env.example` to `.env` and add your OpenRouter API key:
   ```bash
   cp .env.example .env
   # Edit .env and set OPENROUTER_API_KEY=sk-or-v1-your-key-here
   ```

2. Build:
   ```bash
   cmake --preset debug
   cmake --build --preset debug
   ```

**NOTE:** 'debug' is an alias for 'debug-clang'. See `CMakePresets.json` for other available presets, or add your own.

3. Run tests:
   ```bash
   ctest --preset debug
   ```

4. Run the chat app:
   ```bash
   .build/debug-clang/src/wjh/apps/chat/chat_app
   ```

### Command-Line Options

```
-m, --model <id>           Model ID (default: anthropic/claude-sonnet-4)
-s, --system-prompt <text>  System prompt
-t, --max-tokens <n>        Max response tokens (default: 4096)
--show-config               Display resolved config and exit
-h, --help                  Show help
```

### REPL Commands

- `/exit`, `/quit` - Exit the chat
- `/clear` - Clear conversation history
- `/help` - Show available commands

## Build Presets

| Preset | Compiler | Build Type | ASan |
|--------|----------|------------|------|
| `debug` | Clang | Debug | ON |
| `release` | Clang | Release | ON |
| `debug-gcc` | GCC | Debug | OFF |
| `debug-clang` | Clang | Debug | ON |
| `release-gcc` | GCC | Release | OFF |
| `release-clang` | Clang | Release | ON |

### Full Verification

```bash
./scripts/verify-all.sh              # Debug builds (gcc + clang)
./scripts/verify-all.sh --all        # All variants
```

## Project Structure

```
starter/
├── src/wjh/
│   ├── chat/               # Core chat library
│   │   ├── types.atlas      # Strong type definitions
│   │   ├── Result.hpp       # Error handling
│   │   ├── Config.hpp/cpp   # Configuration resolution
│   │   ├── CommandLine.hpp/cpp  # CLI argument parsing
│   │   ├── ChatLoop.hpp/cpp # Main chat loop
│   │   ├── client/          # HTTP + OpenRouter client
│   │   ├── conversation/    # Message + Conversation
│   │   └── tests/           # Unit tests
│   ├── apps/chat/           # Executable
│   └── testing/             # Test utilities (MockClient)
└── cmake/                   # Build modules
```

## Configuration Priority

Settings are resolved in this order (highest to lowest precedence):

1. Command-line arguments
2. `.env.local` (gitignored)
3. `.env` (project config)
4. `~/.config/aipp101_chat/.env` (user preferences)
5. Built-in defaults

## Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `OPENROUTER_API_KEY` | Yes | - | Your OpenRouter API key |
| `LLM_MODEL` | No | `anthropic/claude-sonnet-4` | Model identifier |
| `MAX_TOKENS` | No | `4096` | Maximum response tokens |
| `SYSTEM_PROMPT` | No | - | System prompt text |
