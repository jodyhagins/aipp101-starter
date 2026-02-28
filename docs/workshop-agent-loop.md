# Workshop: Adding an Agent Loop with Tool Calling

You're going to turn this chat app into an **agent** — one that can execute
bash commands on your machine.  Once it can run bash, it can read its own
source code, edit files, compile, run tests, and improve itself.

All changes are in **two files**.  The rest of the codebase is untouched.

| File | Changes |
|------|---------|
| `src/wjh/chat/client/OpenRouterClient.hpp` | Add 1 memfn declaration |
| `src/wjh/chat/client/OpenRouterClient.cpp` | Everything else |

---

## Background: How Tool Calling Works

LLM APIs follow a simple protocol for tool use:

```
          ┌──────────────────────────────────────────┐
          │                                          │
  User    │    ┌─────────┐        ┌──────────────┐   │
  Input ──┼───▶│ Request │───────▶│   LLM API    │   │
          │    │ + tools │        │              │   │
          │    └─────────┘        └──────┬───────┘   │
          │                              │           │
          │            ┌─────────────────┴────┐      │
          │            │                      │      │
          │      tool_calls?             text only   │
          │            │                      │      │
          │     ┌──────▼──────┐               │      │
          │     │ Execute the │               │      │
          │     │ tool (bash) │               │      │
          │     └──────┬──────┘               │      │
          │            │                      │      │
          │   Append result to        ┌───────▼───┐  │
          │   messages, loop ─────┐   │  Return   │  │
          │         back ─────────┘   │  to user  │  │
          │                           └───────────┘  │
          │              Agent Loop                  │
          └──────────────────────────────────────────┘
```

**The JSON protocol** has three parts:

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
          "command": { "type": "string" }
        },
        "required": ["command"]
      }
    }
  }]
}
```

**2. The model responds with tool calls instead of text:**
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

**3. You execute the tool and send the result back:**
```json
{
  "role": "tool",
  "tool_call_id": "call_abc123",
  "content": "wjh/\nCMakeLists.txt\n[exit code: 0]"
}
```

Then you call the API again.  The model either makes more tool calls or
responds with text.  That's the whole protocol.

---

## Step 1: Add a member function declaration to the header

Open **`src/wjh/chat/client/OpenRouterClient.hpp`**.

Find the `parse_response` declaration (around line 69):

```cpp
    Result<AssistantResponse> parse_response(
        nlohmann::json const & json) const;
```

**Add this directly after it:**

```cpp
    /**
     * Send a JSON request to the API and return parsed response.
     */
    Result<nlohmann::json> send_api_request(
        nlohmann::json const & request);
```

This extracts the HTTP plumbing (auth headers, POST, status check, JSON
parse) into a reusable helper so the agent loop can call it repeatedly.

> **Save the file.**  That's the only header change.

---

## Step 2: Add includes

Open **`src/wjh/chat/client/OpenRouterClient.cpp`**.

Find the includes at the top (lines 7-10):

```cpp
#include "wjh/chat/client/OpenRouterClient.hpp"

#include "wjh/chat/json_convert.hpp"
#include "wjh/chat/conversation/Message.hpp"
```

**Add these after the existing includes:**

```cpp
#include <array>
#include <cstdio>
#include <iostream>
```

---

## Step 3: Add helper functions

Still in **`OpenRouterClient.cpp`**, find the opening namespace line:

```cpp
namespace wjh::chat::client {
```

**Add this block _before_ that namespace** (between the includes and the
namespace):

```cpp
namespace {

nlohmann::json make_tools_json()
{
    return {{
        {"type", "function"},
        {"function",
         {{"name", "bash"},
          {"description",
           "Execute a bash command. Use this to run shell "
           "commands, read/write files, compile code, run "
           "tests, etc."},
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

std::string execute_bash(std::string const & command)
{
    std::string full_cmd = command + " 2>&1";
    std::array<char, 4096> buffer;
    std::string result;

    auto * pipe = popen(full_cmd.c_str(), "r");
    if (not pipe) {
        return "Error: failed to execute command";
    }

    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
        if (result.size() > 100'000) {
            result += "\n... [truncated at 100KB]";
            break;
        }
    }

    auto status = pclose(pipe);
    result +=
        "\n[exit code: " + std::to_string(WEXITSTATUS(status))
        + "]";
    return result;
}

} // anonymous namespace
```

**What these do:**

- `make_tools_json()` — returns the JSON tools array that tells the model
  "you have a bash tool."  This gets sent with every API request.
- `execute_bash()` — runs a command via `popen`, captures stdout+stderr
  (`2>&1`), truncates at 100KB so we don't blow up context, and appends
  the exit code.

---

## Step 4: Add `send_api_request()`

This member function extracts the HTTP call + error handling that currently lives
inside `do_send_message`.  The agent loop will call it on each iteration.

**Add this anywhere inside `namespace wjh::chat::client`** — a good spot
is right before the existing `do_send_message` member function (around line 96):

```cpp
Result<nlohmann::json>
OpenRouterClient::
send_api_request(nlohmann::json const & request)
{
    HttpHeaders headers{
        {HeaderName{"Authorization"},
         HeaderValue{
             "Bearer " + json_value(config_.api_key)}},
        {HeaderName{"Content-Type"},
         HeaderValue{"application/json"}}};

    auto result = http_client_.post(
        HttpPath{"/api/v1/chat/completions"},
        HttpBody{request.dump()},
        headers);
    if (not result) {
        return make_error("{}", result.error());
    }

    auto const & response = *result;

    if (response.status != HttpStatusCode{200}) {
        try {
            auto err = nlohmann::json::parse(
                json_value(response.body));
            if (err.contains("error")
                and err["error"].contains("message"))
            {
                return make_error(
                    "API error ({}): {}",
                    json_value(response.status),
                    err["error"]["message"]
                        .get<std::string>());
            }
        } catch (nlohmann::json::exception const &) {
        }
        return make_error(
            "API error ({}): {}",
            json_value(response.status),
            json_value(response.body));
    }

    try {
        return nlohmann::json::parse(
            json_value(response.body));
    } catch (nlohmann::json::parse_error const & e) {
        return make_error(
            "Failed to parse response JSON: {}", e.what());
    }
}
```

---

## Step 5: Replace `do_send_message()` with the agent loop

This is the main event.  **Delete the entire existing `do_send_message`
member function** (the one that starts around line 96 with
`Result<AssistantResponse> OpenRouterClient::do_send_message`) and
**replace it with:**

```cpp
Result<AssistantResponse>
OpenRouterClient::
do_send_message(
    conversation::Conversation const & conversation)
{
    // Build a local copy of the messages JSON.
    // Tool-call history lives here for this turn only.
    auto messages = convert_messages_to_openai(conversation);
    auto const tools = make_tools_json();

    // --- Agent loop ---
    // Keep calling the API until we get a text response
    // (not a tool call).  Cap at 20 iterations for safety.
    for (int i = 0; i < 20; ++i) {
        auto request = nlohmann::json{
            {"model", json_value(config_.model)},
            {"max_tokens", json_value(config_.max_tokens)},
            {"messages", messages},
            {"tools", tools}};

        auto result = send_api_request(request);
        if (not result) {
            return make_error("{}", result.error());
        }

        auto const & choice = (*result)["choices"][0];
        auto const & message = choice["message"];

        // Check: did the model request tool calls?
        if (message.contains("tool_calls")
            and not message["tool_calls"].empty())
        {
            // Append the assistant's tool-call message
            // to our local messages array (the API
            // requires it for context on the next call).
            messages.push_back(message);

            // Execute each requested tool call
            for (auto const & tc : message["tool_calls"]) {
                auto args = nlohmann::json::parse(
                    tc["function"]["arguments"]
                        .get<std::string>());
                auto cmd = args["command"].get<std::string>();

                // Show what's happening
                std::cerr << "\n[tool] bash: " << cmd
                          << std::endl;
                auto output = execute_bash(cmd);
                std::cerr << output << std::endl;

                // Append the tool result message
                messages.push_back(
                    {{"role", "tool"},
                     {"tool_call_id", tc["id"]},
                     {"content", output}});
            }

            continue; // loop back to the API
        }

        // No tool calls — we have a text response.
        return parse_response(*result);
    }

    return make_error("Agent loop exceeded 20 iterations");
}
```

**What's happening here:**

1. We build a local `messages` JSON array from the conversation
2. Each iteration: send request (with `tools` included) → get response
3. If the response contains `tool_calls`:
   - Append the assistant message (with its tool_calls) to `messages`
   - Execute each bash command
   - Append each tool result as a `role: "tool"` message
   - Loop back to step 2
4. If the response is plain text: return it as `AssistantResponse`
5. Safety cap at 20 iterations prevents infinite loops

> **Save the file.**

---

## Step 6: Build

```bash
cmake --build --preset debug
```

Fix any typos until it compiles clean.

---

## Step 7: Run it

```bash
./build/debug/src/wjh/apps/chat/chat_app -s \
  "You are a software engineer working on a C++ project. You have a bash tool. Use it to explore and modify the codebase when asked. The working directory is the project root."
```

### Try these prompts:

**Simple test:**
```
You> List the files under src/wjh/chat/client/
```

**Read its own source:**
```
You> Read the file src/wjh/chat/client/OpenRouterClient.cpp and explain the agent loop we just added.
```

**Compile the project:**
```
You> Run the build: cmake --build --preset debug 2>&1 | tail -5
```

**Run the tests:**
```
You> Run ctest --preset debug and tell me if anything fails.
```

---

## Step 8: The Punchline

Now ask the model to improve itself:

> Read your own source code in `src/wjh/chat/client/OpenRouterClient.cpp`.
> You'll see that tool-call history is lost between conversation turns
> because we use a local JSON array instead of modifying the Conversation
> object.  Fix this so tool interactions persist across turns.  Read
> whatever files you need, make the changes, and rebuild to verify.

The model will use its bash tool to read the source files, understand the
architecture, write the fix, compile, and test — all autonomously.

---

## Known Limitations (features for the model to add!)

These are intentional shortcuts.  Each one is a task you can hand to
the model now that it has bash access:

| Limitation | Prompt to give the model |
|------------|--------------------------|
| **No confirmation before running commands** | "Add a y/n confirmation prompt before executing each bash command. Print the command, ask the user, only execute on 'y'." |
| **Tool history lost between turns** | (the Step 8 prompt above) |
| **No streaming** | "Add streaming support so I can see the response as it generates." |
| **Only one tool** | "Add a `read_file` tool and a `write_file` tool so you don't have to shell out to cat/tee for everything." |
| **100KB output limit is crude** | "Add smarter truncation that keeps the first and last N lines." |
