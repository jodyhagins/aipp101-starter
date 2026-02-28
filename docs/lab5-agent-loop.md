# Lab 5: The Agent Loop

In Lab 4 you wired up real tool definitions and manually typed results back
into the chat. The model sent structured `tool_calls` JSON, you ran the
command in another terminal, and pasted the output. It worked -- but for any
task that requires more than two or three tool calls, the copy-paste workflow
is unbearable.

In this lab you close the loop. Your code will execute tool calls
automatically, feed results back to the model, and keep going until the model
responds with text. By the end, the model explores your project autonomously
-- reading files, listing directories, chaining commands -- while you sit
back and approve each command.

---

## Background: The Agent Loop

The agent loop is a simple control flow:

```
     ┌──────────────────────────────────────────┐
     │                                          │
     ▼                                          │
  Build request                                 │
  (messages + tools)                            │
     │                                          │
     ▼                                          │
  Send to API ──────────▶ tool_calls?           │
                            │        │          │
                           yes       no         │
                            │        │          │
                            ▼        ▼          │
                      Execute     Text only     │
                      bash cmd    ──▶ return    │
                      (with y/n     to user     │
                       prompt)                  │
                            │                   │
                            ▼                   │
                      Append tool               │
                      result to                 │
                      messages ─────────────────┘
```

Three things keep this safe and debuggable:

1. **Permission prompt** -- before every bash command, your code prints the
   command to stderr and waits for `y` or `n`. You stay in control.

2. **Debug logging** -- the `DEBUG_COMMS` flag from Lab 4 dumps full
   request/response JSON to stderr so you can see exactly what the model
   sends and receives.

3. **Iteration cap** -- the loop runs at most 20 times. If the model has not
   produced a text response by then, you return an error. This prevents
   runaway agents.

---

## What You Need to Build

Four components in **two files**:

| File | Component |
|------|-----------|
| `OpenRouterClient.cpp` | `execute_bash()` -- run a command via `popen`, capture output, truncate at 100KB, prompt user for permission before each command |
| `OpenRouterClient.hpp` | `send_api_request()` declaration -- extract HTTP plumbing into a reusable helper that returns `Result<nlohmann::json>` |
| `OpenRouterClient.cpp` | `send_api_request()` implementation -- headers, POST, status check, JSON parse (extracted from the existing `do_send_message()`) |
| `OpenRouterClient.cpp` | Replace `do_send_message()` with the full agent loop: detect `tool_calls` -> execute -> append result -> loop; detect text content -> return; handle empty/null content by nudging the model with "Please use your tools or respond with text." |

The agent loop maintains a **local** `messages` JSON array (it does not
modify the `Conversation` object). It starts from
`convert_messages_to_openai()` and appends assistant messages, tool results,
and nudge messages as the loop progresses.

Cap the loop at **20 iterations**. If exhausted, return an error.

---

## Choose your path: Hard -- just the requirements

### What to build

- Add `execute_bash(std::string const & command)` in the anonymous namespace
  of `OpenRouterClient.cpp`. It must:
  - Print the command to stderr and prompt `[y/n]>`
  - If the user declines, return `"Command skipped by user"`
  - Run the command with `popen`, redirecting stderr via `" 2>&1"`
  - Read output in a 4KB buffer, truncate at 100KB
  - Append `[exit code: N]` using `WEXITSTATUS`

- Add `send_api_request(nlohmann::json const & request)` as a public method
  on `OpenRouterClient`, returning `Result<nlohmann::json>`. Extract the HTTP
  headers, POST call, status check, and JSON parse from `do_send_message()`.

- Replace `do_send_message()` with a `for` loop (max 20 iterations) that:
  1. Builds the request JSON (model, max\_tokens, messages, tools, optional
     temperature)
  2. Calls `send_api_request()`
  3. If the response contains `tool_calls`: append the assistant message to
     `messages`, parse each tool call's arguments JSON, extract the `command`
     string, call `execute_bash()`, print the output to stderr, append a
     `tool` role message with the result, and `continue`
  4. If the response contains non-null, non-empty `content`: return
     `parse_response()` on the full response JSON
  5. Otherwise (empty/null content): append the assistant message if it has
     a `content` key, then append a user message with
     `"Please use your tools or respond with text."` and `continue`
  6. After the loop: return an error `"Agent loop exceeded 20 iterations"`

### Acceptance criteria

- `cmake --build --preset debug && ctest --preset debug` passes
- Ask "What kind of project is this?" -- the model calls bash, you approve,
  output appears, model chains more calls, eventually responds with text
- Deny a command -- the model sees "Command skipped by user" and adapts
- Enable `DEBUG_COMMS` -- full JSON protocol visible on stderr


## Choose your path: Medium -- component guidance

### `execute_bash()`

This function lives in the anonymous namespace alongside `make_tools_json()`
and `debug_json()` from Lab 4. You need three new includes at the top of the
file:

```cpp
#include <array>
#include <cstdio>
#include <iostream>
```

Key patterns:

- **Permission prompt**: print to `stderr` (not stdout -- that is the chat
  stream). Read the answer from `std::cin` with `std::getline`.
- **Command execution**: append `" 2>&1"` to capture stderr alongside stdout.
  Use `popen(full_cmd.c_str(), "r")` to get a `FILE*` pipe.
- **Read buffer**: `std::array<char, 4096>` with `fgets`. Accumulate into a
  `std::string`. If the string exceeds 100,000 bytes, append a truncation
  notice and break.
- **Exit code**: `pclose()` returns a raw status. Use `WEXITSTATUS(status)`
  to get the actual exit code. Append it as `[exit code: N]`.

### `send_api_request()`

This is a mechanical refactor. The HTTP plumbing currently lives in
`do_send_message()` -- pull it out into its own method so the agent loop can
call it repeatedly without duplicating code.

The method takes a `nlohmann::json const & request` and returns
`Result<nlohmann::json>`. It:

1. Builds the `Authorization` and `Content-Type` headers
2. Calls `http_client_.post()` with the path, dumped JSON body, and headers
3. Checks the HTTP status code, parsing error JSON if available
4. Parses and returns the response body as JSON

Declare it in the `private:` section of `OpenRouterClient` in the header
(after `parse_response`).

### The agent loop (`do_send_message()`)

The new `do_send_message()` replaces the old one entirely. Key structural
decisions:

- **Local messages array**: call `convert_messages_to_openai(conversation)` once
  at the top. All subsequent modifications (appending assistant messages, tool
  results, nudges) happen on this local array. The `Conversation` object is
  not modified.
- **Tools**: call `make_tools_json()` once and reuse across iterations.
- **Request construction**: build a fresh `nlohmann::json` each iteration with
  `model`, `max_tokens`, `messages`, `tools`, and optionally `temperature`.
- **Tool call handling**: when the response contains `tool_calls`, push the
  full assistant message onto `messages`, then iterate each tool call. Parse
  the `arguments` string as JSON, extract the `command` field, call
  `execute_bash()`, print the output to stderr, and push a `tool` role
  message with the `tool_call_id` and output as content.
- **Text content**: if the message has non-null, non-empty `content` and no
  tool calls, return `parse_response()` on the full response JSON.
- **Empty/null content**: some models return a message with `content: null` or
  empty string and no tool calls. When this happens, append the message (if
  it has a `content` key) and push a user nudge message asking the model to
  use tools or respond. This prevents the loop from stalling.

### Debugging

Use `debug_json("request", request)` and `debug_json("response", *result)`
in the loop body. When `DEBUG_COMMS` is true, you see the full protocol on
each iteration.


## Choose your path: Easy -- step-by-step walkthrough

### Step 1: Add `execute_bash()`

In `src/wjh/chat/client/OpenRouterClient.cpp`, add these includes at the top
of the file (with the other includes):

```cpp
#include <array>
#include <cstdio>
#include <iostream>
```

Then add `execute_bash()` inside the anonymous namespace, after
`make_tools_json()`:

```cpp
std::string execute_bash(std::string const & command)
{
    std::cerr << "\n[tool] bash: " << command
              << "\n[y/n]> " << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty()
        or (answer[0] != 'y' and answer[0] != 'Y'))
    {
        return "Command skipped by user";
    }

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
        "\n[exit code: "
        + std::to_string(WEXITSTATUS(status)) + "]";
    return result;
}
```

Build to make sure it compiles:

```bash
cmake --build --preset debug
```

### Step 2: Declare `send_api_request()` in the header

In `src/wjh/chat/client/OpenRouterClient.hpp`, add this declaration in the
`private:` section, after `parse_response`:

```cpp
    /**
     * Send a JSON request to the API and return parsed
     * response JSON.
     */
    Result<nlohmann::json> send_api_request(
        nlohmann::json const & request);
```

### Step 3: Implement `send_api_request()`

In `src/wjh/chat/client/OpenRouterClient.cpp`, add this method in the
`wjh::chat::client` namespace (before or after `do_send_message()`):

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
            "Failed to parse response JSON: {}",
            e.what());
    }
}
```

Build again to verify:

```bash
cmake --build --preset debug
```

### Step 4: Replace `do_send_message()` with the agent loop

Delete the entire existing `do_send_message()` implementation and replace it
with the agent loop. This is the core of the lab:

```cpp
Result<ChatResponse>
OpenRouterClient::
do_send_message(
    conversation::Conversation const & conversation)
{
    auto messages =
        convert_messages_to_openai(conversation);
    auto const tools = make_tools_json();

    for (int i = 0; i < 20; ++i) {
        auto request = nlohmann::json{
            {"model", json_value(config_.model)},
            {"max_tokens",
             json_value(config_.max_tokens)},
            {"messages", messages},
            {"tools", tools}};

        if (config_.temperature) {
            request["temperature"] =
                json_value(*config_.temperature);
        }

        debug_json("request", request);

        auto result = send_api_request(request);
        if (not result) {
            return make_error("{}", result.error());
        }

        debug_json("response", *result);

        auto const & choice = (*result)["choices"][0];
        auto const & message = choice["message"];

        // Tool calls: execute and loop
        if (message.contains("tool_calls")
            and not message["tool_calls"].empty())
        {
            messages.push_back(message);

            for (auto const & tc :
                 message["tool_calls"])
            {
                auto args = nlohmann::json::parse(
                    tc["function"]["arguments"]
                        .get<std::string>());
                auto cmd =
                    args["command"].get<std::string>();

                auto output = execute_bash(cmd);
                std::cerr << output << std::endl;

                messages.push_back(
                    {{"role", "tool"},
                     {"tool_call_id", tc["id"]},
                     {"content", output}});
            }
            continue;
        }

        // Text content: return to user
        if (message.contains("content")
            and not message["content"].is_null()
            and not message["content"]
                        .get<std::string>()
                        .empty())
        {
            return parse_response(*result);
        }

        // Empty/null content: nudge the model
        if (message.contains("content")) {
            messages.push_back(message);
        }
        messages.push_back(
            {{"role", "user"},
             {"content",
              "Please use your tools or respond "
              "with text."}});
    }

    return make_error(
        "Agent loop exceeded 20 iterations");
}
```

### Build and test

```bash
cmake --build --preset debug && ctest --preset debug
```

The existing tests should still pass -- they use `MockClient`, which has its
own `do_send_message()`. Your changes only affect the real
`OpenRouterClient`.

---

## Experiments

### Let the agent explore

Start the app and ask it to describe the project:

```
You> Describe the project in this directory.
```

The model should call `ls`, then `cat` a few files, chaining commands until
it has enough context. Approve each command with `y`. Watch it build up
understanding across multiple tool calls -- automatically.

### Count source files

```
You> How many C++ source files are in this project?
```

The model should use `find` to locate `.cpp` and `.hpp` files. It might
refine its approach across multiple commands.

### Discover design patterns

```
You> What design patterns does this codebase use?
```

This requires reading multiple files. The model should explore headers,
notice the NVI pattern, find the strong type system, and identify the
dependency injection in `ChatLoop`. Watch how many tool calls it takes.

### Find TODOs

```
You> Find all TODO comments in the source code.
```

The model should use `grep -r` or a similar approach.

### Read a specific file

```
You> What's in the CMakeLists.txt dependency list?
```

The model should `cat CMakeLists.txt` and find the `FetchContent` blocks.

### Compare with Lab 4

Try the same questions you used in Lab 4 (manually typing results). Notice
the difference in speed and depth. The agent can chain 5-10 tool calls in the
time it took you to copy-paste one result.

### Deny a command

When the model requests a command, type `n` at the `[y/n]>` prompt. The
model receives `"Command skipped by user"` as the tool result. Watch how it
adapts -- it might try a different approach or explain what it wanted to do.

### Watch the protocol

Set `DEBUG_COMMS = true` in `OpenRouterClient.cpp`, rebuild, and repeat a
conversation. On stderr you will see the full request and response JSON for
every iteration of the agent loop. Notice how the `messages` array grows with
each tool call and result.

---

## Extra Challenge: Yolo Mode (Dogfooding)

The ultimate test of your agent: use it to modify its own source code.

Start the app and paste this prompt:

```
Read src/wjh/chat/client/OpenRouterClient.cpp.

You'll see a constexpr bool DEBUG_COMMS flag. I want you to add a
second flag right next to it:

    constexpr bool YOLO_MODE = false;

Then modify execute_bash() so that when YOLO_MODE is true, it skips
the confirmation prompt and runs commands immediately.

Read the file, make the change, rebuild with:
    cmake --build --preset debug
and verify the tests still pass with:
    ctest --preset debug
```

Approve the commands as the agent reads the file, writes the changes, builds,
and runs the tests. If it makes a mistake, it will see the compiler error in
the tool result and try to fix it -- just like a real coding agent.

### What this demonstrates

The agent is reading its own source code, understanding the structure,
making a targeted edit, and verifying the change compiles and passes tests --
all using the very tool-calling loop it just modified. This is dogfooding in
the most literal sense.

Think about what happened mechanically:

1. The model called `cat` to read `OpenRouterClient.cpp`
2. It identified the location of `DEBUG_COMMS` and `execute_bash()`
3. It used `sed` or a heredoc to insert the new code
4. It ran the build and read the compiler output
5. It ran the tests and confirmed they passed
6. It reported success with text -- ending the agent loop

Every step was a tool call. Every result was fed back. The model made all the
decisions about *what* to do and *how* to do it. Your code just executed bash
commands and shuttled results back and forth. That is the entire architecture
of a coding agent.
