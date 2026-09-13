# Lab 2: Add Temperature & AGENTS.md Support

In Lab 1 you discovered that the model is non-deterministic -- the same prompt
produces different outputs. But you had no way to *control* that randomness.
You also saw that the model is eager to follow system prompt instructions, but
had to pass them on the command line every time.

In this lab you will add two features to the chat application:

1. **Temperature** -- a parameter that controls how "creative" (random) the
   model's output is. You will wire it through the full stack: CLI flag,
   environment variable, config resolution, and the API request.

2. **AGENTS.md loading** -- automatically read an `AGENTS.md` file from the
   current directory and inject its contents into the system prompt. This
   lets you configure the model's behavior per-project without typing it on
   the command line every time.

Both features follow the same pattern already established in the codebase
for model, max-tokens, and system-prompt. You are extending that pattern.

---

## Background: What is temperature?

Temperature is a float (0.0 to 2.0) that controls how the model samples
from its next-token probability distribution:

| Temperature | Behavior |
|------------|----------|
| **0.0** | Deterministic -- always picks the highest-probability token. Same input = same output every time. Best for factual Q&A, code generation, structured output. |
| **0.7** | Focused but natural -- slight variety while staying coherent. The most common "production" default for assistants and chatbots. |
| **1.0** | OpenRouter's default -- uses the model's raw probability distribution as-is. Good balance of creativity and coherence. |
| **2.0** | Maximum randomness -- the distribution is heavily flattened, so unlikely tokens get a real shot. Outputs get creative, surprising, and often incoherent. |

If you omit temperature from the request, OpenRouter defaults to 1.0.

The OpenRouter API accepts temperature as an optional float field in the
request JSON.

---

## Feature A: Temperature Support

### What you need to build

Temperature needs to flow through the same layers as the existing settings:

```
CLI (--temperature 0.7)
        |
        v
CommandLineArgs.temperature  (std::optional<Temperature>)
        |
        v
Config.temperature  (std::optional<Temperature>)
   resolved from: CLI > env (TEMPERATURE) > none
        |
        v
OpenRouterClientConfig.temperature  (std::optional<Temperature>)
        |
        v
API request JSON: {"temperature": 0.7, ...}
```

### Choose your path: Hard -- just the requirements

Add a `Temperature` strong type (float, no special features needed) and wire
it through the full stack:

1. Define the type in `types.atlas` and regenerate `types_gen.hpp` (cmake --build --preset debug)
2. Add `--temperature <value>` to CLI parsing. Validate it is a valid float.
3. Add `temperature` to `Config` and resolve it: CLI > `TEMPERATURE` env > none.
4. Show it in `print_config` when present.
5. Add `temperature` to `OpenRouterClientConfig`.
6. Include `"temperature"` in the API request JSON when present (omit when not set).
7. Pass it through in `ChatLoop.cpp`'s `run(int, char**)`.
8. Update all existing tests that construct `Config`, `CommandLineArgs`, or
   `OpenRouterClientConfig` (they will fail to compile until you add the new
   field).
9. Add new tests for temperature parsing, config resolution, and validation.
10. All tests pass: `ctest --preset debug`

Hint: look at how `system_prompt` and `max_tokens` are handled -- temperature
follows the same pattern, but it is a float parsed with `std::strtof` instead
of `std::from_chars`.


### Choose your path: Medium -- guided implementation

#### Files you will modify

| File | What to change |
|------|---------------|
| `src/wjh/chat/types.atlas` | Add `Temperature` type |
| `src/wjh/chat/CommandLine.hpp` | Add `temperature` field to `CommandLineArgs` |
| `src/wjh/chat/CommandLine.cpp` | Parse `--temperature` flag, add to help text |
| `src/wjh/chat/Config.hpp` | Add `temperature` field to `Config` |
| `src/wjh/chat/Config.cpp` | Resolve temperature: CLI > env > none, print it |
| `src/wjh/chat/client/OpenRouterClient.hpp` | Add `temperature` field to config |
| `src/wjh/chat/client/OpenRouterClient.cpp` | Include temperature in request JSON |
| `src/wjh/chat/ChatLoop.cpp` | Pass temperature when constructing the client |
| `src/wjh/chat/tests/CommandLine_ut.cpp` | Add temperature parsing tests |
| `src/wjh/chat/tests/Config_ut.cpp` | Add temperature resolution tests |
| `src/wjh/chat/tests/OpenRouterClient_ut.cpp` | Add `.temperature` to existing configs |
| `src/wjh/chat/tests/ChatLoop_ut.cpp` | Add `.temperature` to existing configs |

#### Approach

1. **Start with the type.** Add to `types.atlas`:
   ```
   [class Temperature]
   description=float
   ```
   Then regenerate: `cmake --build --preset debug` (the build system runs the
   atlas generator automatically).

2. **Work bottom-up.** Add the field to `CommandLineArgs`, then `Config`, then
   `OpenRouterClientConfig`. Each time, the compiler will tell you every
   call site that needs updating.

3. **For float parsing**, use `std::strtof` (not `std::from_chars` -- the
   latter does not support floats on all compilers). Pattern:
   ```cpp
   char * end = nullptr;
   float val = std::strtof(str.c_str(), &end);
   if (end != str.c_str() + str.size()) {
       // parse error
   }
   ```
   You will need `#include <cstdlib>` for `std::strtof`.

4. **For the API request**, only include temperature when it is set:
   ```cpp
   if (config_.temperature) {
       request["temperature"] = json_value(*config_.temperature);
   }
   ```

5. **Build the request as a local variable** instead of returning a braced
   initializer directly -- this lets you conditionally add fields.

6. **Update help text** in `CommandLine.cpp` to list the new flag and env var.


### Choose your path: Easy -- step-by-step walkthrough

Follow these steps in order. Build after each step to let the compiler guide
you.

#### Step 1: Add the Temperature type

Open `src/wjh/chat/types.atlas`. Add this block after `MaxTokens` and before
`SystemPrompt`:

```
# Temperature for LLM response randomness (0.0 to 2.0)
[class Temperature]
description=float
```

Build to regenerate `types_gen.hpp`:
```bash
cmake --build --preset debug
```

It should compile (new type exists but nothing uses it yet).

#### Step 2: Add temperature to CommandLineArgs

In `src/wjh/chat/CommandLine.hpp`, add to the `CommandLineArgs` struct:

```cpp
std::optional<Temperature> temperature;
```

Add it right after `max_tokens`.

#### Step 3: Parse --temperature in CommandLine.cpp

In `src/wjh/chat/CommandLine.cpp`, add `#include <cstdlib>` at the top (you
need it for `std::strtof`).

Then add this block in `parse_args`, right before the `"Unknown argument"`
error at the bottom:

```cpp
if (arg == "--temperature") {
    if (i + 1 >= args.size()) {
        return make_error(
            "Missing argument for {}", arg);
    }
    ++i;
    std::string arg_str{args[i]};
    char * end = nullptr;
    float temp = std::strtof(arg_str.c_str(), &end);
    if (end != arg_str.c_str() + arg_str.size()) {
        return make_error(
            "Invalid number for --temperature:"
            " '{}'",
            args[i]);
    }
    result.temperature = Temperature{temp};
    continue;
}
```

Also update the help text in `help_text()`. Add this line in the Options
section:

```
  --temperature <value>       LLM temperature (0.0-2.0)
```

And add this in the Environment variables section:

```
  TEMPERATURE                 LLM temperature override
```

#### Step 4: Add temperature to Config

In `src/wjh/chat/Config.hpp`, add to the `Config` struct, after
`system_prompt`:

```cpp
std::optional<Temperature> temperature;
```

Also add `#include <filesystem>` to the includes (you will need it in step 8).

#### Step 5: Resolve temperature in Config.cpp

In `src/wjh/chat/Config.cpp`, add `#include <format>` and `#include <fstream>`
to the includes.

In `resolve_config`, add `.temperature = std::nullopt` to the `Config`
initializer (after `.system_prompt`).

Then add this resolution block after the system prompt resolution and before
`return config;`:

```cpp
// Resolve temperature: CLI > env > none
if (args.temperature) {
    config.temperature = *args.temperature;
} else if (auto env = get_env("TEMPERATURE")) {
    char * end = nullptr;
    float val = std::strtof(env->c_str(), &end);
    if (end != env->c_str() + env->size()) {
        return make_error(
            "Invalid TEMPERATURE value: '{}'", *env);
    }
    config.temperature = Temperature{val};
}
```

In `print_config`, add this block before the system prompt output:

```cpp
if (config.temperature) {
    out << "  Temperature: " << *config.temperature << "\n";
}
```

#### Step 6: Add temperature to OpenRouterClientConfig

In `src/wjh/chat/client/OpenRouterClient.hpp`, add to
`OpenRouterClientConfig`, after `system_prompt`:

```cpp
std::optional<Temperature> temperature;
```

#### Step 7: Include temperature in the API request

In `src/wjh/chat/client/OpenRouterClient.cpp`, change `build_request` to
build the JSON as a local variable so you can conditionally add temperature:

```cpp
nlohmann::json
OpenRouterClient::
build_request(conversation::Conversation const & conversation) const
{
    auto request = nlohmann::json{
        {"model", json_value(config_.model)},
        {"max_tokens", json_value(config_.max_tokens)},
        {"messages", convert_messages_to_openai(conversation)}};

    if (config_.temperature) {
        request["temperature"] =
            json_value(*config_.temperature);
    }

    return request;
}
```

#### Step 8: Wire it through ChatLoop.cpp

In `src/wjh/chat/ChatLoop.cpp`, in the `run(int, char**)` function, change:

```cpp
auto const & config = *config_result;
```

to:

```cpp
auto config = std::move(*config_result);
```

Then, where `OpenRouterClientConfig` is constructed, add the temperature
field:

```cpp
auto client = std::make_unique<client::OpenRouterClient>(
    client::OpenRouterClientConfig{
        .api_key = config.api_key,
        .model = config.model,
        .max_tokens = config.max_tokens,
        .system_prompt = config.system_prompt,
        .temperature = config.temperature});
```

#### Step 9: Fix tests that won't compile

Several test files construct `Config` or `OpenRouterClientConfig` with
designated initializers. You need to add `.temperature = std::nullopt` to
each one.

In `src/wjh/chat/tests/ChatLoop_ut.cpp`, find the `makeTestConfig` function
and add `.temperature = std::nullopt` after `.system_prompt`.

In `src/wjh/chat/tests/OpenRouterClient_ut.cpp`, find every
`OpenRouterClientConfig{...}` and add `.temperature = std::nullopt` after
`.system_prompt`.

In `src/wjh/chat/tests/Config_ut.cpp`, find every `Config{...}` and add
`.temperature = std::nullopt` after `.system_prompt`. Also add
`EnvGuard temp_guard("TEMPERATURE", nullptr);` to any test that sets up
a full set of env guards.

Build and run tests:
```bash
cmake --build --preset debug && ctest --preset debug
```

#### Step 10: Add new tests

Add temperature-specific tests to `CommandLine_ut.cpp`:

- Parse `--temperature 0.7` -- check it roundtrips
- Parse `--temperature 0` -- zero is valid
- Missing value after `--temperature` -- should error
- Invalid value `--temperature hot` -- should error
- Combining `--temperature` with other flags

Add to `Config_ut.cpp`:

- `TEMPERATURE=0.5` in env -- check it resolves
- CLI temperature overrides env temperature
- Invalid `TEMPERATURE` in env -- should error


---

## Feature B: AGENTS.md Loading

### What you need to build

When the chat app starts, if an `AGENTS.md` file exists in the current
directory, its contents should be read and appended to the system prompt,
wrapped in XML tags that tell the model these are authoritative instructions.

This lets you drop an `AGENTS.md` into any project directory to customize the
model's behavior without using `-s` every time.

### The wrapping format

The file contents should be wrapped like this:

```
<system-reminder>
As you answer the user's questions, you can use the following context.

Codebase and user instructions are shown below. Be sure to adhere to these instructions.

IMPORTANT: These instructions OVERRIDE any default behavior and you MUST follow them as written.

[contents of AGENTS.md here]
</system-reminder>
```

### Behavior rules

- If `AGENTS.md` does not exist: do nothing.
- If `AGENTS.md` is empty: do nothing.
- If there is no existing system prompt: set the system prompt to the wrapped
  content.
- If there is already a system prompt (from `-s` or `SYSTEM_PROMPT` env):
  append the wrapped content after the existing prompt, separated by `\n`.

### Choose your path: Hard -- just the requirements

Write a function `append_agents_file(Config&, path)` that:

1. Checks if `AGENTS.md` exists in the given directory.
2. Reads the entire file into a string.
3. If non-empty, wraps it with the XML tags shown above.
4. Appends (or sets) the system prompt in the config.

Call it from `ChatLoop.cpp`'s `run(int, char**)` after config resolution but
before constructing the client.

Write thorough tests:
- No file -- config unchanged
- No file -- existing system prompt preserved
- File present -- system prompt set from wrapped content
- File present with existing prompt -- both present, existing first
- Empty file -- config unchanged
- Verify wrapper tag structure


### Choose your path: Medium -- guided implementation

#### Files you will modify

| File | What to change |
|------|---------------|
| `src/wjh/chat/Config.hpp` | Declare `append_agents_file()` |
| `src/wjh/chat/Config.cpp` | Implement `append_agents_file()` |
| `src/wjh/chat/ChatLoop.cpp` | Call `append_agents_file()` after config resolution |
| `src/wjh/chat/tests/Config_ut.cpp` | Add tests for all behaviors |

#### Approach

1. **Signature:** `void append_agents_file(Config& config, std::filesystem::path const& dir = ".");`

2. **In Config.hpp:** Add `#include <filesystem>` and declare the function.

3. **In Config.cpp:** You need `#include <fstream>` and `#include <format>`.
   Use `std::filesystem::exists()` to check for the file, read with
   `std::istreambuf_iterator`, and build the wrapped string using string
   concatenation (the prefix and suffix are `static constexpr` string
   literals).

4. **In ChatLoop.cpp:** Call `append_agents_file(config)` right after config
   resolution. Note: you need the config to be *mutable*, so change
   `auto const&` to `auto` with `std::move`.

5. **For tests:** Create a `TempDir` RAII helper that makes a unique temp
   directory and cleans it up. Write a `write_file` method on it. Then test
   each scenario by creating (or not) an `AGENTS.md` in that temp dir.


### Choose your path: Easy -- step-by-step walkthrough

#### Step 1: Declare append_agents_file

In `src/wjh/chat/Config.hpp`, add this declaration after `print_config`:

```cpp
/**
 * Load AGENTS.md from the given directory and append its
 * contents (wrapped in system-reminder tags) to the system
 * prompt in the given configuration.
 */
void append_agents_file(
    Config & config,
    std::filesystem::path const & dir = ".");
```

Make sure `#include <filesystem>` is in the includes.

#### Step 2: Implement append_agents_file

In `src/wjh/chat/Config.cpp`, add `#include <fstream>` and `#include <format>`
at the top if not already there. Then add this function at the bottom, before
the closing `} // namespace`:

```cpp
void
append_agents_file(
    Config & config,
    std::filesystem::path const & dir)
{
    auto const path = dir / "AGENTS.md";
    if (not std::filesystem::exists(path)) {
        return;
    }

    std::ifstream file(path);
    if (not file) {
        return;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (content.empty()) {
        return;
    }

    static constexpr auto wrapper_prefix =
        "<system-reminder>"
        "As you answer the user's questions, "
        "you can use the following context.\n\n"
        "Codebase and user instructions are shown "
        "below. Be sure to adhere to these "
        "instructions.\n\n"
        "IMPORTANT: These instructions OVERRIDE "
        "any default behavior and you MUST follow "
        "them as written.\n\n";
    static constexpr auto wrapper_suffix =
        "\n</system-reminder>";

    auto wrapped =
        std::string{wrapper_prefix} + content + wrapper_suffix;

    if (config.system_prompt) {
        config.system_prompt = SystemPrompt{
            std::format(
                "{}\n{}", *config.system_prompt, wrapped)};
    } else {
        config.system_prompt =
            SystemPrompt{std::move(wrapped)};
    }
}
```

#### Step 3: Call it from ChatLoop.cpp

In `src/wjh/chat/ChatLoop.cpp`, in `run(int, char**)`, right after config
resolution succeeds, add:

```cpp
append_agents_file(config);
```

The config variable must be mutable for this. If it is currently declared as
`auto const & config = *config_result;`, change it to
`auto config = std::move(*config_result);`.

#### Step 4: Add a TempDir test helper

In `src/wjh/chat/tests/Config_ut.cpp`, add these includes at the top:

```cpp
#include <filesystem>
#include <format>
#include <fstream>

#include <unistd.h>
```

Then add this helper struct in the anonymous namespace (before `TEST_SUITE`):

```cpp
// RAII helper that creates a temporary directory and
// removes it (with contents) on destruction.
struct TempDir
{
    std::filesystem::path path_;

    TempDir()
    : path_(std::filesystem::temp_directory_path()
          / "wjh_chat_test_XXXXXX")
    {
        auto tmpl = path_.string();
        auto * result = mkdtemp(tmpl.data());
        REQUIRE(result != nullptr);
        path_ = result;
    }

    ~TempDir()
    {
        std::filesystem::remove_all(path_);
    }

    void write_file(
        std::string const & name,
        std::string const & content) const
    {
        std::ofstream out(path_ / name);
        out << content;
    }

    TempDir(TempDir const &) = delete;
    TempDir & operator = (TempDir const &) = delete;
};
```

Also add a `make_test_config()` helper:

```cpp
Config
make_test_config()
{
    return Config{
        .api_key = ApiKey{"sk-test-key"},
        .model = ModelId{"test/model"},
        .max_tokens = MaxTokens{1024u},
        .system_prompt = std::nullopt,
        .temperature = std::nullopt,
        .show_config = ShowConfig{false}};
}
```

#### Step 5: Add tests

Add these test cases inside the `TEST_SUITE("Config")` block:

```cpp
TEST_CASE("append_agents_file: no file leaves config "
          "unchanged")
{
    TempDir dir;
    auto config = make_test_config();

    append_agents_file(config, dir.path_);

    CHECK_FALSE(config.system_prompt.has_value());
}

TEST_CASE("append_agents_file: no file preserves "
          "existing system prompt")
{
    TempDir dir;
    auto config = make_test_config();
    config.system_prompt =
        SystemPrompt{"existing prompt"};

    append_agents_file(config, dir.path_);

    REQUIRE(config.system_prompt.has_value());
    CHECK(*config.system_prompt
          == SystemPrompt{"existing prompt"});
}

TEST_CASE("append_agents_file: sets system prompt "
          "from file content")
{
    TempDir dir;
    dir.write_file(
        "AGENTS.md", "# Test Instructions\nDo X.");
    auto config = make_test_config();

    append_agents_file(config, dir.path_);

    REQUIRE(config.system_prompt.has_value());
    auto prompt =
        std::format("{}", *config.system_prompt);

    CHECK(prompt.starts_with("<system-reminder>"));
    CHECK(prompt.ends_with("</system-reminder>"));
    CHECK(prompt.find("# Test Instructions\nDo X.")
          != std::string::npos);
}

TEST_CASE("append_agents_file: appends to existing "
          "system prompt")
{
    TempDir dir;
    dir.write_file("AGENTS.md", "Agent rules here.");
    auto config = make_test_config();
    config.system_prompt =
        SystemPrompt{"You are helpful."};

    append_agents_file(config, dir.path_);

    REQUIRE(config.system_prompt.has_value());
    auto prompt =
        std::format("{}", *config.system_prompt);

    CHECK(prompt.starts_with("You are helpful."));
    auto wrapped_pos =
        prompt.find("<system-reminder>");
    REQUIRE(wrapped_pos != std::string::npos);
    CHECK(prompt.find("Agent rules here.")
          != std::string::npos);
}

TEST_CASE("append_agents_file: empty file leaves "
          "config unchanged")
{
    TempDir dir;
    dir.write_file("AGENTS.md", "");
    auto config = make_test_config();

    append_agents_file(config, dir.path_);

    CHECK_FALSE(config.system_prompt.has_value());
}
```

Build and run:
```bash
cmake --build --preset debug && ctest --preset debug
```

---

## Experiments

Once both features are working, try them out!

### Temperature experiments

#### Freeze it

```bash
chat_app --temperature 0
```

Ask the same question three times (with `/clear` between each):

```
Write a one-paragraph product description for a rubber duck that debugs code.
```

The responses should be nearly identical. Low temperature = predictable.

#### Melt it

```bash
chat_app --temperature 1.8
```

Same prompt. The model might get creative. It might get *weird*. It might
invent words.

#### Find the sweet spot

Try the same prompt at 0.3, 0.7, 1.0, and 1.5. Where does it feel most
natural? Where does it fall apart?

#### Temperature via environment

Set it in your `.env` file:

```
TEMPERATURE=0.5
```

Run `chat_app --show-config` and verify it shows up. Then override it on the
command line:

```bash
chat_app --temperature 0 --show-config
```

The CLI should win.

### AGENTS.md experiments

#### Create your first AGENTS.md

In the project root (where you run `chat_app`), create `AGENTS.md`:

```markdown
# Instructions

You are a helpful C++ tutor. When explaining code, always:
- Use modern C++20 syntax
- Point out potential pitfalls
- Suggest better alternatives when appropriate

Keep responses concise -- no more than 3 paragraphs.
```

Now run `chat_app` and ask: `Explain how std::vector grows when you push_back.`

Then delete (or rename) `AGENTS.md` and ask the same question. Notice the
difference in style and length.

#### Stack with a system prompt

Try using both:

```bash
chat_app -s "Always start your response with 'Ahoy!'"
```

With `AGENTS.md` present. Do both instructions apply? Which one appears first
in the prompt? (Use `--show-config` to check.)

---

## Verify

Before moving on, make sure:

- [ ] `cmake --build --preset debug` compiles with no warnings
- [ ] `ctest --preset debug` -- all tests pass
- [ ] `chat_app --temperature 0.5 --show-config` shows the temperature
- [ ] `chat_app --temperature notanumber` gives a clear error
- [ ] `TEMPERATURE=0.7` in `.env` works
- [ ] `AGENTS.md` in the current directory gets loaded into the system prompt
- [ ] Without `AGENTS.md`, the app works exactly as before
