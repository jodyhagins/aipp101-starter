# Lab 6: Specialized Tools

In Lab 5 you built an agent loop with a single `bash` tool. The agent can
read files, write files, compile, and test -- all through bash commands. It
works, but there is a problem: **bash is a chainsaw**. Every operation goes
through the same blunt instrument. The model runs `cat` to read a file,
`echo` with redirects to write one, and `sed` to edit. Each of those
commands can fail in surprising ways, has no semantic meaning to your
permission system, and gives the model no guidance about which approach to
use.

In this lab you will add three specialized tools: `read_file`, `write_file`,
and `edit_file`. But here is the twist: **you will not write the code
yourself.** You will prompt your agent to modify its own source code, build
it, and verify the tests pass. Your role shifts from programmer to director.

---

## Background: Why Specialized Tools?

### Safety

With only `bash`, every operation requires the same permission prompt. You
see `bash: cat README.md` and `bash: rm -rf /` and have to evaluate each
one. With specialized tools, `read_file` needs no permission (it is
read-only), `write_file` shows the path and size, and `edit_file` shows a
diff preview. You can make informed decisions without parsing bash commands
in your head.

### Reliability

`echo "content" > file.txt` breaks if the content contains quotes,
backticks, or dollar signs. `sed -i 's/old/new/'` breaks on special regex
characters. Specialized tools use direct file I/O -- no shell escaping, no
regex surprises.

### Semantics

When the model calls `read_file`, you know it wants to read. When it calls
`edit_file`, you know it wants to make a targeted change. This is
information you can log, audit, and use for policy decisions. With `bash`,
you have to parse the command string to guess intent.

### The meta-lesson

The agent is about to modify **its own source code** to add these tools.
It will read `OpenRouterClient.cpp`, understand the existing tool system,
write new functions, update the tool registry, build, and test. This is
exactly how production coding agents work -- they extend themselves.

---

## Tool Specifications

### `read_file`

| Field | Value |
|-------|-------|
| **Parameters** | `file_path` (string, required), `offset` (integer, optional, 1-indexed line number), `limit` (integer, optional, max lines to read) |
| **Returns** | File contents with line numbers in `cat -n` format: `{:>6}\t{}` per line |
| **Permission** | None -- read-only operations are always safe |
| **Truncation** | 100KB max output |
| **Errors** | "Cannot open file: \<path\>", "File is empty or offset is past end" |

### `write_file`

| Field | Value |
|-------|-------|
| **Parameters** | `file_path` (string, required), `content` (string, required) |
| **Permission** | Required -- `[tool] write_file: <path> (N bytes)\n[y/n]>` |
| **Behavior** | Creates parent directories via `std::filesystem::create_directories` |
| **Errors** | "Cannot create directory: \<path\>", "Cannot open file for writing: \<path\>", "Write failed" |

### `edit_file`

| Field | Value |
|-------|-------|
| **Parameters** | `file_path` (string, required), `old_string` (string, required), `new_string` (string, required) |
| **Permission** | Required -- shows `--- old ---` / `--- new ---` diff preview |
| **Constraint** | `old_string` must appear exactly once in the file. Fails (before prompting) if 0 matches or 2+ matches. |
| **Errors** | "Cannot open file: \<path\>", "old_string not found in \<path\>", "old_string is not unique in \<path\> (found N occurrences)", "Cannot write file: \<path\>" |

---

## Choose your path: Hard -- just the goal

Add three tools (`read_file`, `write_file`, `edit_file`) to the agent using
the specs above. The catch: **prompt your agent to do it.** You write zero
C++ code.

Your agent needs to:

1. Add the three tool definitions to `make_tools_json()`
2. Add `execute_read_file()`, `execute_write_file()`, `execute_edit_file()`
   functions in the anonymous namespace
3. Add a `dispatch_tool(name, args)` function that routes by tool name
4. Update the agent loop in `do_send_message()` to use `dispatch_tool()`
   instead of hardcoded bash dispatch
5. Update the tests in `OpenRouterClient_ut.cpp`

### Acceptance criteria

- `cmake --build --preset debug && ctest --preset debug` passes
- Ask "Read Config.hpp" -- model uses `read_file` (no permission prompt)
- Ask "Create a hello.txt file" -- model uses `write_file` (permission
  prompt shows path and size)
- Ask "Change DEBUG_COMMS to true" -- model uses `edit_file` (diff preview)
- Deny a write -- model sees "Write skipped by user" and adapts

### Hints

- Your agent needs context. Show it the current `OpenRouterClient.cpp`
  first.
- Let the agent build and test after each change.
- If the agent makes a mistake, let it see the compiler error -- it will
  fix it.

---

## Choose your path: Medium -- phased approach

Break the work into four phases. After each phase, have the agent build and
test before moving to the next.

### Phase 1: `read_file`

Prompt your agent to add the `read_file` tool. Give it context about the
current `make_tools_json()` structure and the `execute_bash()` pattern.

**What the agent needs to do:**
- Add `#include <fstream>` and `#include <filesystem>` at the top
- Add the `read_file` tool definition to `make_tools_json()`
- Add `execute_read_file(nlohmann::json const & args)` in the anonymous
  namespace
- The function should: open the file, read lines with optional offset/limit,
  format with line numbers (`{:>6}\t{}`), truncate at 100KB

**Hint:** Show the agent the current `make_tools_json()` first so it knows
the JSON structure pattern.

**Verify:** Build and run tests. The new tool definition should appear in
the schema but is not yet dispatched.

### Phase 2: `write_file`

Prompt for `write_file`. The agent already knows the pattern from Phase 1.

**What the agent needs to do:**
- Add the `write_file` tool definition to `make_tools_json()`
- Add `execute_write_file(nlohmann::json const & args)` -- permission
  prompt, create parent dirs, write content

**Hint:** Point out that `execute_bash()` already has a permission prompt
pattern the agent can follow.

**Verify:** Build and run tests.

### Phase 3: `edit_file`

This is the most complex tool. The agent needs to:
- Add the `edit_file` tool definition to `make_tools_json()`
- Add `execute_edit_file(nlohmann::json const & args)` -- read file, find
  old_string, check uniqueness, show diff preview, get permission, replace,
  write back

**Hint:** The uniqueness check must happen **before** the permission
prompt. No point asking the user to approve an edit that cannot be applied.

**Verify:** Build and run tests.

### Phase 4: `dispatch_tool()` and agent loop update

Now wire everything together:
- Add `dispatch_tool(std::string const & name, nlohmann::json const & args)`
  that routes to the correct execute function
- Update the agent loop in `do_send_message()` to extract the tool name
  and call `dispatch_tool()` instead of hardcoding `execute_bash()`

**What to change in the agent loop:**

Replace:
```cpp
auto cmd = args["command"].get<std::string>();
auto output = execute_bash(cmd);
```

With:
```cpp
auto name = tc["function"]["name"].get<std::string>();
auto args = nlohmann::json::parse(...);
auto output = dispatch_tool(name, args);
```

**Hint:** Also update the bash tool description to remove "read/write files"
since those operations now have dedicated tools.

**Verify:** Full build and test. Then run the app and test each tool.

### Common pitfalls

- **Missing includes**: `<filesystem>` and `<fstream>` are needed for the
  new tools.
- **Permission prompt on read**: `read_file` should NOT prompt. Only
  `write_file` and `edit_file` need permission.
- **Edit uniqueness**: the edit must fail if `old_string` appears 0 or 2+
  times. Check **before** prompting.
- **Dispatch ordering**: `dispatch_tool()` must appear after all
  `execute_*` functions in the file (forward declarations are in the
  anonymous namespace, so ordering matters).

---

## Choose your path: Easy -- guided prompts

### Phase 1: Add `read_file`

Start your agent and paste this prompt:

```
Read src/wjh/chat/client/OpenRouterClient.cpp.

I want you to add a `read_file` tool. Here is the spec:

Parameters: file_path (string, required), offset (integer, optional,
1-indexed), limit (integer, optional)

Returns: File contents with line numbers in cat -n format ({:>6}\t{} per
line). Truncate at 100KB. No permission prompt needed (read-only).

Errors: "Cannot open file: <path>", "File is empty or offset is past end"

What to do:
1. Add #include <fstream> and #include <filesystem> at the top
2. Add a read_file tool definition to make_tools_json() following the
   same pattern as the bash tool
3. Add execute_read_file(nlohmann::json const & args) in the anonymous
   namespace after execute_bash()
4. Build with: cmake --build --preset debug
5. Run tests with: ctest --preset debug
```

**What to expect:** The agent will read the file, understand the structure,
add the includes, expand `make_tools_json()`, write `execute_read_file()`,
and build/test. Approve the bash commands for building and testing.

**If it makes a mistake:** Let it see the compiler error. It will read the
error, identify the issue, and fix it. This is the agent loop working on
itself.

### Phase 2: Add `write_file`

```
Now add a `write_file` tool to the same file.

Parameters: file_path (string, required), content (string, required)

Permission: Required. Print to stderr:
  [tool] write_file: <path> (N bytes)
  [y/n]>
If denied, return "Write skipped by user".

Behavior: Create parent directories with
std::filesystem::create_directories. Write content with std::ofstream.

Errors: "Cannot create directory: <path>",
"Cannot open file for writing: <path>", "Write failed"

Add the tool definition to make_tools_json() and add
execute_write_file(nlohmann::json const & args) after execute_read_file().

Build and test.
```

**What to expect:** The agent reads the updated file, adds the tool
definition and implementation, builds, and tests.

### Phase 3: Add `edit_file`

```
Now add an `edit_file` tool.

Parameters: file_path (string, required), old_string (string, required),
new_string (string, required)

Constraint: old_string must appear exactly once in the file.
- If 0 occurrences: return error "old_string not found in <path>"
  (BEFORE prompting)
- If 2+ occurrences: return error "old_string is not unique in <path>
  (found N occurrences)" (BEFORE prompting)

Permission: Required. Show a diff preview on stderr:
  [tool] edit_file: <path>
  --- old ---
  <old_string>
  --- new ---
  <new_string>
  [y/n]>
If denied, return "Edit skipped by user".

Behavior: Read the whole file into a string
(std::istreambuf_iterator<char>), find old_string, replace with
new_string, write back.

Add the tool definition to make_tools_json() and add
execute_edit_file(nlohmann::json const & args) after
execute_write_file().

Build and test.
```

**What to expect:** This is the most complex tool. The agent may need
two attempts if it gets the uniqueness check wrong. Let it see the
compiler error and fix it.

### Phase 4: Wire up `dispatch_tool()`

```
Now add a dispatch_tool() function and update the agent loop.

1. Add this function in the anonymous namespace AFTER all the execute_*
   functions:

   std::string dispatch_tool(
       std::string const & name,
       nlohmann::json const & args)

   It should route by name:
   - "bash" -> execute_bash(args["command"])
   - "read_file" -> execute_read_file(args)
   - "write_file" -> execute_write_file(args)
   - "edit_file" -> execute_edit_file(args)
   - anything else -> "Error: unknown tool: <name>"

2. In do_send_message(), replace the hardcoded bash dispatch:
   Change:
     auto args = nlohmann::json::parse(...);
     auto cmd = args["command"].get<std::string>();
     auto output = execute_bash(cmd);

   To:
     auto name = tc["function"]["name"].get<std::string>();
     auto args = nlohmann::json::parse(
         tc["function"]["arguments"].get<std::string>());
     auto output = dispatch_tool(name, args);

3. Update the bash tool description in make_tools_json() to remove
   "read/write files" since those now have dedicated tools.

Build and test.
```

**What to expect:** The agent makes the changes, builds, and tests. The
existing tests should still pass. The agent loop now routes to all four
tools.

### Phase 5: Update the tests

```
Read src/wjh/chat/tests/OpenRouterClient_ut.cpp.

The "Bash tool schema is well-formed" test hardcodes a 1-tool array.
We now have 4 tools. Update the tests:

1. Update the existing bash tool schema test -- the tools array now has
   4 entries, and the bash tool description no longer says "read/write
   files"

2. Add a SUBCASE for each new tool schema:
   - "read_file tool schema" -- check parameters: file_path (required),
     offset and limit (optional)
   - "write_file tool schema" -- check parameters: file_path and content
     (both required)
   - "edit_file tool schema" -- check parameters: file_path, old_string,
     new_string (all required)

3. Add a "Tool dispatch with mixed tool names" SUBCASE in the
   "Agent loop message structures" TEST_CASE. Build a tool_calls array
   with different tool names (bash, read_file, write_file) and verify
   the name and argument parsing.

Build and test.
```

---

## Experiments

### Does the model prefer specialized tools?

Start the app with all four tools active and ask:

```
You> Read src/wjh/chat/Config.hpp
```

Does the model use `read_file` or `bash cat`? It should prefer `read_file`
-- no permission prompt, cleaner output with line numbers. If it still uses
bash, try being more explicit: "Use your read_file tool to read Config.hpp."

### File creation

```
You> Create a file called hello.txt with the content "Hello from the agent!"
```

The model should use `write_file`. You will see the permission prompt with
the path and byte count. Approve it and verify the file exists.

### Targeted editing

```
You> Change DEBUG_COMMS from false to true in OpenRouterClient.cpp
```

The model should use `edit_file`. You will see a diff preview showing the
old and new values. Approve it and verify the change.

### Compare with Lab 5

Try the same tasks from Lab 5's experiments. Notice how the model's tool
choices change with specialized tools available. The same "describe this
project" task should use `read_file` instead of `bash cat`.

### Self-extension

The ultimate test -- can the agent add **another** tool?

```
You> Add a list_files tool that takes a directory path and optional
recursive flag. It should return the file listing. Add it to
make_tools_json(), implement execute_list_files(), and add it to
dispatch_tool(). Build and test.
```

If the agent successfully extends the tool system it just built, you have
a fully self-modifying coding agent.

---

## Reflect

Think about what just happened in this lab:

1. You **prompted** the agent to read its own source code
2. The agent **understood** the tool registration system
3. It **wrote** three new tool implementations
4. It **updated** the dispatch logic to route to them
5. It **compiled** the modified code
6. It **ran** the tests to verify correctness
7. It **used** the new tools it just created

The agent modified its own source code, compiled it, tested it, and then
used the result. This is the architecture of every production coding agent:
Claude Code, Cursor, Aider, Copilot Workspace -- they all have specialized
tools for reading, writing, and editing files, dispatched through exactly
this kind of routing function.

The human's role shifted in this lab. You did not write code. You directed
and approved. You decided **what** to build, the agent decided **how** to
build it, and you verified the result. That is the workflow of AI-assisted
development -- not replacing the programmer, but changing what "programming"
means.
