# Lab 4: Tool Definitions

In Lab 3 you played the role of the agent loop by hand. You read the model's
"TOOL_CALL" output, ran the command yourself, and pasted the result back. It
worked -- but it was clunky, fragile, and entirely dependent on the model
following your text conventions.

The real tool-calling protocol solves all of that. Instead of hoping the model
formats a `TOOL_CALL:` string correctly, you **declare your tools in the API
request** and the model responds with structured JSON. No parsing ambiguity, no
guessing, no "Hey dude, can you run..." format experiments.

In this lab you will wire up real tool definitions. The model will respond with
proper `tool_calls` JSON, and you will **still act as the executor** -- typing
results back manually, just like Lab 3. The difference is that the protocol is
now machine-readable. Lab 5 will close the loop by automating execution.

---

## Background: The Real Tool-Calling Protocol

LLM APIs follow a three-part JSON protocol for tool use:

**1. You describe your tools in the request:**

```json
{
  "model": "...",
  "messages": [...],
  "tools": [{
    "type": "function",
    "function": {
      "name": "bash",
      "description": "Execute a bash command",
      "parameters": {
        "type": "object",
        "properties": {
          "command": {
            "type": "string",
            "description": "The bash command to execute"
          }
        },
        "required": ["command"]
      }
    }
  }]
}
```

**2. The model responds with `tool_calls` instead of text:**

```json
{
  "choices": [{
    "message": {
      "role": "assistant",
      "content": null,
      "tool_calls": [{
        "id": "call_abc123",
        "type": "function",
        "function": {
          "name": "bash",
          "arguments": "{\"command\": \"ls src/\"}"
        }
      }]
    }
  }]
}
```

Note: `arguments` is a JSON **string** (not an object). You parse it separately.

**3. You execute the tool and send the result back as a `tool` message:**

```json
{
  "role": "tool",
  "tool_call_id": "call_abc123",
  "content": "wjh/\nCMakeLists.txt\n[exit code: 0]"
}
```

Then you call the API again. The model either makes more tool calls or responds
with text. That's the whole protocol.

```
     You ──▶ Request + tools ──▶ LLM API
                                    │
                              ┌─────┴──────┐
                              │            │
                         tool_calls     text only
                              │            │
                        ┌─────▼─────┐      │
                        │  Display  │      │
                        │  to user  │      ▼
                        │  (you     │   Return
                        │  execute) │   to user
                        └─────┬─────┘
                              │
                        User types result
                        back as input ──────▶ next API call
```

In this lab, the "display to user" step is literal: your code formats the tool
call as readable text, and you type the result back. In Lab 5, you will replace
yourself with code.

---

## What You Need to Build

Four changes in **two files**:

| File | Change |
|------|--------|
| `OpenRouterClient.cpp` | Add `make_tools_json()` in anonymous namespace |
| `OpenRouterClient.cpp` | Modify `build_request()` to include tools |
| `OpenRouterClient.cpp` | Modify `parse_response()` to detect and format tool calls |
| `OpenRouterClient.cpp` | Optional: `DEBUG_COMMS` flag + `debug_json()` helper |
| `OpenRouterClient_ut.cpp` | Add tests for tool call formatting |

When the model responds with `tool_calls`, your code formats them as readable
text like:

```
[Tool call] bash: {"command":"ls src/"}
```

The user sees this, runs the command, and types the result back as a normal
message. The model treats the user's response as if it came from the tool
(close enough for a manual workflow).

---

## Choose your path: Hard -- just the requirements

1. Create a `make_tools_json()` function that returns a `nlohmann::json` array
   containing a single bash tool with `command` as its parameter.

2. Modify `build_request()` to include `request["tools"] = make_tools_json()`.

3. Modify `parse_response()` to detect `message["tool_calls"]` before the
   existing "no text content" error. When tool calls are present, format each
   as `[Tool call] <name>: <arguments>` and return the concatenated string
   as the `AssistantResponse`.

4. Optional: add a `constexpr bool DEBUG_COMMS = false;` flag and a
   `debug_json()` helper that dumps labeled JSON to stderr when enabled.

5. Add tests:
   - `make_tools_json()` returns a valid array with the bash tool schema
   - Tool call response JSON is formatted as readable `[Tool call]` text

Acceptance criteria:
- `cmake --build --preset debug && ctest --preset debug` passes
- Ask "What kind of project is this?" -- see `[Tool call] bash: {"command":"ls"}`
- Type a directory listing back -- model continues reasoning


## Choose your path: Medium -- guided hints

### Which functions to modify

| Function | What to do |
|----------|------------|
| `make_tools_json()` | New function in anonymous namespace. Returns `nlohmann::json` array. One entry: bash tool with `command` parameter. |
| `build_request()` | Add one line after temperature handling: `request["tools"] = make_tools_json();` |
| `parse_response()` | Before the "no text content" error check, test for `message["tool_calls"]`. Iterate the array, format each as `[Tool call] name: arguments`. |

### Key JSON structures

The tools array uses nested initializer lists. The outer array has one object
per tool. Each tool has `type: "function"` and a `function` object with `name`,
`description`, and `parameters`.

In the response, `tool_calls` is an array of objects. Each has:
- `tc["function"]["name"]` -- a string
- `tc["function"]["arguments"]` -- a JSON **string** (not parsed object)

### Approach tips

- `make_tools_json()` lives in an anonymous namespace (not a member function)
  because it has no state dependency.
- For `parse_response()`, check `message.contains("tool_calls")` AND
  `not message["tool_calls"].empty()` -- some models send an empty array.
- Build the display string by iterating `message["tool_calls"]` and
  concatenating `[Tool call] ` + name + `: ` + arguments + `\n`.
- Return it wrapped in `ChatResponse{.response = AssistantResponse{...}, ...}`.
- For `DEBUG_COMMS`, use `json.dump(2)` for pretty-printed output to stderr.


## Choose your path: Easy -- step-by-step walkthrough

### Step 1: Add `make_tools_json()`

In `src/wjh/chat/client/OpenRouterClient.cpp`, add an anonymous namespace
**before** the `namespace wjh::chat::client {` line. Put the tools function
inside:

```cpp
namespace {

constexpr bool DEBUG_COMMS = false;

void debug_json(
    std::string_view label,
    nlohmann::json const & json)
{
    if constexpr (DEBUG_COMMS) {
        std::fputs(
            std::format(
                "\n=== {} ===\n{}\n",
                label,
                json.dump(2))
                .c_str(),
            stderr);
    }
}

nlohmann::json make_tools_json()
{
    return {{
        {"type", "function"},
        {"function",
         {{"name", "bash"},
          {"description",
           "Execute a bash command. Use this to run "
           "shell commands, read/write files, compile "
           "code, run tests, etc."},
          {"parameters",
           {{"type", "object"},
            {"properties",
             {{"command",
               {{"type", "string"},
                {"description",
                 "The bash command to execute"}}}}},
            {"required", {"command"}}}}}}
    }};
}

} // anonymous namespace
```

Build to make sure it compiles (nothing calls it yet):

```bash
cmake --build --preset debug
```

### Step 2: Modify `build_request()`

Find `build_request()` in the same file. After the temperature block, add one
line:

```cpp
request["tools"] = make_tools_json();
```

The full function should now look like:

```cpp
nlohmann::json
OpenRouterClient::
build_request(
    conversation::Conversation const & conversation) const
{
    auto request = nlohmann::json{
        {"model", json_value(config_.model)},
        {"max_tokens", json_value(config_.max_tokens)},
        {"messages",
         convert_messages_to_openai(conversation)}};

    if (config_.temperature) {
        request["temperature"] =
            json_value(*config_.temperature);
    }

    request["tools"] = make_tools_json();

    return request;
}
```

### Step 3: Modify `parse_response()`

Find `parse_response()`. The current code checks for missing content and
returns an error. You need to insert a tool-call check **before** that error.

Find this block:

```cpp
// Extract text content
if (not message.contains("content")
    or message["content"].is_null())
{
    return make_error(
        "Response contains no text content");
}
```

**Insert this before it:**

```cpp
// Check for tool calls
if (message.contains("tool_calls")
    and not message["tool_calls"].empty())
{
    std::string display;
    for (auto const & tc : message["tool_calls"]) {
        auto const & fn = tc["function"];
        display +=
            "[Tool call] "
            + fn["name"].get<std::string>() + ": "
            + fn["arguments"].get<std::string>()
            + "\n";
    }
    return ChatResponse{
        .response =
            AssistantResponse{std::move(display)},
        .usage = std::move(usage)};
}
```

### Step 4: Optional -- enable `DEBUG_COMMS`

The `debug_json()` helper is already in place from Step 1. To use it, add
calls in `do_send_message()` after building the request and after parsing the
response:

```cpp
debug_json("request", request);
```

```cpp
debug_json("response", response_json);
```

To enable, change `constexpr bool DEBUG_COMMS = false;` to `true`, rebuild,
and watch the full JSON protocol scroll by on stderr.

### Build and test

```bash
cmake --build --preset debug && ctest --preset debug
```

---

## Experiments

### See a tool call in action

Start the app and ask about the project:

```
You> What kind of project is this?
```

You should see something like:

```
[Tool call] bash: {"command":"ls"}
```

The model is asking to run `ls`. In a separate terminal, run `ls` yourself and
type the result back:

```
You> Result: CMakeLists.txt CMakePresets.json AGENTS.md src/ docs/ .env
```

The model should continue reasoning and possibly request more tool calls.

### Multi-step exploration

```
You> Read Config.hpp and explain what it does.
```

The model should request:

```
[Tool call] bash: {"command":"cat src/wjh/chat/Config.hpp"}
```

Run the command and paste the result back. The model explains the file using
real data -- no hallucination.

### Compare with Lab 3

Try the same questions you used in Lab 3. Notice the difference:

- Lab 3: model outputs `TOOL_CALL: list_files(".")` -- a text convention you
  invented, formatted however the model felt like.
- Lab 4: model outputs structured `tool_calls` JSON -- a protocol defined by
  the API, consistent every time.

The model's tool usage should be more reliable now because the API enforces
the schema.

### Watch the protocol (DEBUG_COMMS)

Set `DEBUG_COMMS = true` in `OpenRouterClient.cpp`, rebuild, and repeat a
conversation. On stderr you will see the full request JSON (with your `tools`
array) and the full response JSON (with `tool_calls`). This is the raw
protocol. Everything else is presentation.

### Give wrong results

When the model asks to run `ls`, type back something false:

```
You> There are no files here. The directory is empty.
```

What does the model do? Does it trust you? Does it try another approach? This
is why tool result integrity matters -- and why Lab 5 will automate execution
instead of relying on you.

### Try denying a tool call

When the model requests a command, just say no:

```
You> I won't run that command.
```

Does the model adapt? Does it try a different approach or give up?

---

## Reflect

You now have the real tool-calling protocol wired up. The model sends
structured, schema-validated tool requests. You can see exactly what it wants
to run. The protocol is the same one used by ChatGPT, Claude, and every other
tool-using LLM.

But you are still the bottleneck. Every tool call requires you to:

1. Read the command
2. Switch to a terminal
3. Run it
4. Copy the output
5. Paste it back

For a single `ls`, that is fine. For a task that requires 10-15 tool calls
(reading files, compiling, running tests, fixing errors), it is unbearable.

In Lab 5, you will close the loop: your code will execute the tool calls
automatically, feed results back to the model, and keep going until the model
is done. The manual workflow you just experienced becomes a fully automated
agent.
