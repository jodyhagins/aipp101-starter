# AI++ 101 Starter Kit: Chat Application

A complete, working C++ chat application using the OpenRouter API. This is your starting point for the AI++ 101 workshop.

**Workshop students: [start here — install Docker, download the image, and build the project](docs/student-setup.md).**

For historical lab checkouts, use the [refreshed workshop checkpoints](docs/workshop-checkpoints.md) (`new-lab2` through `new-lab6`).

The workshop uses a prebuilt image hosted in this project's GitHub Packages.
Students need Docker and Git on their computers; the C++ build tools are included
in the image. The local-development prerequisites below apply when building
outside Docker.

## Quick Start

### Prerequisites

- C++20 compiler (GCC 13+ or Clang 17+)
- CMake 3.27+
- Ninja build system (make requires changing presets)
- OpenSSL development libraries
- ccache (optional, for faster rebuilds)

### Workshop Docker environment

For the workshop, use the instructor-provided image and the `workshop` preset.
The image preinstalls Atlas, tl::expected, nlohmann/json, cpp-httplib, dotenv-cpp,
doctest, RapidCheck, and OpenSSL development files. The workshop preset requires
installed dependencies and never downloads or builds a missing dependency.

```bash
docker pull "$WORKSHOP_IMAGE"               # Use the instructor's image digest.
./scripts/workshop.sh "$WORKSHOP_IMAGE"      # Opens a shell in the mounted checkout.
cmake --preset workshop
cmake --build --preset workshop
ctest --preset workshop
```

Keep `.build` between sessions to preserve incremental builds and ccache. See
[the workshop guide](docs/workshop.md) for image publishing, student setup,
project updates, and dependency maintenance.

### Dependencies outside the workshop

Ordinary presets prefer installed CMake packages and otherwise fetch the revisions
in `cmake/DependencySources.cmake`. Atlas tracks `main`; the other source revisions
are pinned, including the instructor's doctest and RapidCheck forks. Set
`CMAKE_PREFIX_PATH` for installations outside standard search paths.

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
| `workshop` | GCC | Debug | OFF |
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
