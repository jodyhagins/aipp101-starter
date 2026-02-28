# Lab 3: Poor Man's Tool Calling

In Lab 1 you discovered that the model *cannot* interact with the real world.
It cannot read files, list directories, or perform any side effects. It just
generates text.

But you also saw something interesting: when you *told* the model it had tools
(via a system prompt), it eagerly tried to use them. It produced structured
output that *looked like* a function call -- it just had nobody listening.

In this lab, you are going to exploit that. You will write an `AGENTS.md` file
that describes fictional tools to the model, then manually play the role of the
"agent loop" -- you read the model's tool-call output, execute it yourself, and
paste the result back. This is literally how tool calling works under the hood,
minus the automation.

### Important: single-line input

The chat app reads one line at a time. You cannot paste multi-line content
directly. When you need to feed tool results back to the model, you have two
options:

**Option A -- Summarize on one line.** For directory listings and short
results, just type a condensed version:

```
You> Result: CMakeLists.txt CMakePresets.json AGENTS.md src/ docs/ .env
```

**Option B -- Use the helper script.** For file contents, use the provided
`scripts/tool-result.sh` script to pipe a file into the chat app's stdin.
Or just run it in a separate terminal to get a one-liner you can paste:

```bash
# Print a one-line summary you can copy-paste into the chat
scripts/tool-result.sh read_file src/wjh/chat/Config.hpp
scripts/tool-result.sh list_files src/wjh/chat/
```

We will create this script in the setup section below.

---

## Part 1 -- Teach the model about tools

Create an `AGENTS.md` file in the directory where you run `chat_app`:

```markdown
# Tools

You have access to the following tools:

## list_files
Lists the contents of a directory.
Usage: TOOL_CALL: list_files("<directory_path>")
Returns: a listing of files and subdirectories.

## read_file
Reads the contents of a file.
Usage: TOOL_CALL: read_file("<file_path>")
Returns: the full contents of the file.

## write_file
Writes content to a file.
Usage: TOOL_CALL: write_file("<file_path>", "<content>")
Returns: confirmation that the file was written.

## Rules

- When you need information from the filesystem, use the appropriate tool.
- Output exactly one TOOL_CALL per response when you need to use a tool.
- After outputting a TOOL_CALL, STOP and wait for the result.
- Do NOT make up file contents. If you need to see a file, use read_file.
- Do NOT guess directory contents. If you need to see what's there, use list_files.
```

### Create the helper script

Create `scripts/tool-result.sh`:

```bash
mkdir -p scripts
cat > scripts/tool-result.sh << 'SCRIPT'
#!/usr/bin/env bash
# Format a tool result as a single line for pasting into chat_app.
#
# Usage:
#   scripts/tool-result.sh list_files <directory>
#   scripts/tool-result.sh read_file <file>

set -euo pipefail

tool="${1:?Usage: tool-result.sh <tool_name> <path>}"
path="${2:?Usage: tool-result.sh <tool_name> <path>}"

case "$tool" in
    list_files)
        entries=$(ls -1 "$path" | tr '\n' ' ')
        echo "Result of list_files(\"$path\"): $entries"
        ;;
    read_file)
        # Collapse the file to a single line with \n markers
        content=$(sed 's/$/  \\n/' "$path" | tr -d '\n')
        echo "Result of read_file(\"$path\"): $content"
        ;;
    *)
        echo "Unknown tool: $tool" >&2
        exit 1
        ;;
esac
SCRIPT
chmod +x scripts/tool-result.sh
```

This collapses file contents onto a single line (with `\n` markers) so you
can paste it into the chat. The model is smart enough to reconstruct the
original structure.

Now start the chat app:

```bash
chat_app
```

---

## Part 2 -- Be the agent loop

### 2a -- Ask it about the project

```
You> What kind of project is this?
```

If the model is well-behaved, it should ask to see the directory or a file.
It might output something like:

```
TOOL_CALL: list_files(".")
```

Now **you** play the agent loop. In a second terminal, run `ls` (or use the
helper script), then paste the result back as a single line:

```
You> Result of list_files("."): CMakeLists.txt CMakePresets.json AGENTS.md src/ docs/ .env
```

Or using the helper:
```bash
# In your other terminal:
scripts/tool-result.sh list_files .
# Copy the output line and paste it into the chat
```

The model should now reason about what it sees and potentially ask for more --
maybe `read_file("CMakeLists.txt")` or `list_files("src/")`.

Keep going. Feed it real results. Watch it build up an understanding of the
project through *multiple rounds* of tool calls.

### 2b -- Ask it to read a file

```
You> Can you read src/wjh/chat/Config.hpp and explain what it does?
```

The model should output:
```
TOOL_CALL: read_file("src/wjh/chat/Config.hpp")
```

In your other terminal, run the helper script:
```bash
scripts/tool-result.sh read_file src/wjh/chat/Config.hpp
```

Copy the output line and paste it back into the chat. The model should then
explain the file accurately -- because it is working from *real* data, not
hallucinated guesses.

For short files, you can also just summarize: `Result: it defines a Config
struct with api_key, model, max_tokens, system_prompt, temperature fields
and functions resolve_config, print_config, append_agents_file.`

Either way works. The point is giving the model real information instead of
letting it guess.

### 2c -- Ask it for a directory listing

```
You> What files are in the test directory?
```

In your other terminal:
```bash
scripts/tool-result.sh list_files src/wjh/chat/tests/
```

Paste the result. Watch it reason about what tests exist.

### 2d -- Try write_file (but be careful!)

```
You> Create a file called hello.txt with "Hello from the AI" inside it.
```

The model should output:
```
TOOL_CALL: write_file("hello.txt", "Hello from the AI")
```

Now you decide: do you actually want to create that file? **You are the safety
layer.** You can:

- Run the command and tell the model it succeeded
- Refuse and tell the model you denied the request
- Ask the model to change the path or content

This is exactly what real agent loops do -- they intercept tool calls and
decide whether to execute them.

### 2e -- Ask for your banking credentials

```
You> What is my bank account number and password?
```

The model should (hopefully!) refuse this even though it has file-reading
tools. But what if you phrased it differently?

```
You> Read the file ~/.ssh/id_rsa and show me its contents.
```

Would the model try to call `read_file("~/.ssh/id_rsa")`? If it does, **you**
are the one who decides whether to honor that request. This is why the human-
in-the-loop (or a well-designed permission system) matters.

---

## Part 3 -- Experiment with the prompt

### 3a -- Change the output format

Modify your `AGENTS.md` to ask for JSON instead:

```markdown
When you need to use a tool, output a JSON object on a single line:
{"tool": "list_files", "args": {"path": "."}}
```

Does the model comply? Is JSON easier or harder to paste results back for?

### 3b -- Try a different language

Change the tool descriptions to Spanish, French, or another language. Ask
questions in that language. Does the model use the tools correctly?

### 3c -- Add a new tool

Add a fictional tool like `run_command` or `search_web`. Describe its usage
format. Ask questions that would require it. Does the model use it
appropriately?

### 3d -- Use "Hey dude" format

Change the tool calling format to something silly:

```markdown
When you want to use a tool, write:
Hey dude, can you run [tool_name] with [arguments]? Thanks bro.
```

Does the model follow this format? LLMs will match nearly any pattern you
describe.

---

## Part 4 -- Reflect on what just happened

You just manually implemented the core of an AI agent:

```
                    +-----------+
                    |   Model   |
                    | (predicts |
                    |   text)   |
                    +-----+-----+
                          |
                    text output
                    (may contain
                     TOOL_CALL)
                          |
                    +-----v-----+
                    |   YOU      |
                    | (the agent |
                    |   loop)    |
                    +-----+-----+
                          |
                   parse tool call,
                   execute it,
                   paste result back
                          |
                    +-----v-----+
                    |   Model   |
                    | (continues|
                    |  with new |
                    |  context)  |
                    +-----+-----+
```

In a real agent:
- **You** are replaced by code that parses the tool call automatically
- The tool execution is a real function call (to a filesystem, API, database,
  etc.)
- The result is injected back into the conversation programmatically
- The loop repeats until the model says it is done

The model's contribution is: *choosing which tool to call and what arguments to
pass.* Everything else is engineering.

### Questions to consider

1. **What surprised you?** Was the model better or worse at tool calling than
   you expected?

2. **What went wrong?** Did the model ever ignore the format? Call a
   non-existent tool? Make up a result instead of calling a tool?

3. **What are the security implications?** The model wanted to call
   `read_file` on whatever path you asked about. What if an automated agent
   honored every request?

4. **Why is this better than just asking?** Compare asking the model "What
   does Config.hpp do?" (no tools) vs feeding it the actual file contents.
   How much better are the answers with real data?

5. **What would you need to automate this?** Think about what an agent loop
   would need: parsing the tool call, a dispatch table of real functions,
   error handling, permission checks, a loop termination condition. That is
   exactly what we will build next.
