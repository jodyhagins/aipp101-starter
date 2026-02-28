# Workshop exercise checkpoints

Use `workshop-lab0` through `workshop-lab6` as the **starting points** for the
exercises. Each coding exercise is unfinished at its own checkpoint and complete
at the next. `workshop-complete` contains the completed lab 6 solution.

The earlier `new-lab*` series had incorrect exercise boundaries: `new-lab2`
already included temperature and AGENTS.md loading, `new-lab5` already included
the agent loop, and `new-lab6` already included specialized tools. Those published
tags remain unchanged; use the `workshop-*` tags below for teaching.

## What is implemented at each checkpoint?

| Start here | Original source state | Already implemented | Exercise to do |
| --- | --- | --- | --- |
| `workshop-lab0` | `d8c32d1999a3` (original `lab0`) | Base chat application | Explore tokens and prompts; no application code change |
| `workshop-lab1` | `d8c32d1999a3` (original `lab1`) | Base chat application | Talk to the model; no application code change |
| `workshop-lab2` | `d8c32d1999a3` (original `lab2`) | Base chat application | Add temperature, AGENTS.md loading, and `/usage` |
| `workshop-lab3` | `534b3cb0cc3e` (original `lab3`) | All three lab 2 features | Perform tool calls manually through prompting |
| `workshop-lab4` | `872904489bfc` (original `lab4`) | Lab 2 features; no API tool definitions | Add API tool definitions and display tool calls |
| `workshop-lab5` | `e02585c7c19e` | API tool definitions and display; no automatic execution | Implement the agent loop with bash execution |
| `workshop-lab6` | `c6ee23bc870d` (original `lab5`) | Agent loop with bash; no specialized tools | Add read_file, write_file, and edit_file |
| `workshop-complete` | `153ed9d81326` | All lab 6 tools and the subsequent GCC warning fix | Review the completed solution |

Labs 0 and 1 are exploratory, so they share the lab 2 starting source tree.
Lab 3 is a manual prompting exercise: its AGENTS.md and helper script are
student-created artifacts, not an additional C++ feature. The lab 4 checkpoint
has the same application source as lab 3 and adds the lab 4 instructions.

`e341939` is a partial lab 2 solution, not a lab 2 starting point. It appears
between the lab 2 and lab 3 checkpoints as an untagged intermediate commit.
The old `lab5` and `lab6` tag names also identify later source states than the
corresponding exercises require; this table uses the actual exercise boundaries.

## Enter the environment

With Docker running, from a terminal on your computer (Ubuntu/WSL on Windows),
enter your existing checkout. Students without a checkout first run:

```bash
mkdir -p ~/projects
cd ~/projects
git clone https://github.com/jodyhagins/aipp101-starter.git
cd aipp101-starter
```

Then, from the checkout:

```bash
git remote set-url origin https://github.com/jodyhagins/aipp101-starter.git
git fetch origin --tags
git switch --detach workshop-lab2
export WORKSHOP_IMAGE='ghcr.io/jodyhagins/aipp101-starter/workshop@sha256:d6a8c8324c2e3c6a39994ea42daf59a41af9bae16e480052ea112b626efdd028'
docker pull "$WORKSHOP_IMAGE"
./scripts/workshop.sh "$WORKSHOP_IMAGE" \
  env 'PS1=\[\e[36m\]student@\h:\w\$ \[\e[0m\]' bash --norc -i
```

Inside the container:

```bash
cmake --preset workshop && cmake --build --preset workshop && ctest --preset workshop
```

The installed Atlas and all third-party dependencies come from the existing
workshop-v1 image. No image rebuild is required. All generated headers are
committed at each checkpoint.

## Advance to the next exercise

Save any student work you want to keep before changing checkpoints. Git changes
affect the host checkout and every container that mounts it immediately.

These commands can run **inside the container**, because origin now uses HTTPS:

```bash
git fetch origin --tags
git switch --detach workshop-lab3
cmake --preset workshop && cmake --build --preset workshop && ctest --preset workshop
```

Choose `workshop-lab4`, `workshop-lab5`, `workshop-lab6`, or `workshop-complete`
as directed by the instructor. At a tag, fetch and switch to a checkpoint;
`git pull` is for updating a branch. To update main inside the container:

```bash
git switch main
git pull --ff-only
```

HTTPS fetching from this public repository works with the launcher's numeric
UID. The earlier SSH error (`No user exists for uid 501`) is specific to using
SSH without a matching container user entry. The instructor can retain SSH for
pushes from the host by running this once in the checkout:

```bash
git remote set-url --push origin git@github.com:jodyhagins/aipp101-starter.git
```

## Verification and publication

The corrected series appends to `94749e2d9a39`, without rewriting published
commits or moving existing tags. For each historical source state, the build
setup is carried forward and Atlas regenerates the committed headers. Original
application source and tests are preserved apart from generated headers.
Lab instructions use the `workshop` build preset inside the image.

Verification covers offline configure/build/CTest, CLI and REPL checks of the
lab 2 feature boundaries, repeatable header generation, and an incremental
build with no work to do. The checkpoint audit additionally compares source
against the original commits and checks that API tools, the agent loop, and
specialized tools appear at the correct boundaries:

```bash
python3 scripts/verify-workshop-checkpoints.py
```

Validation uses Atlas `96ebb2bcc86fb25b48c59b819c1dda522d645c32` and the local
workshop image. It does not constitute testing the entire compiler matrix or
both image architectures.

The instructor can publish the prepared local branch from the host terminal:

```bash
git switch main
git merge --ff-only workshop-exercise-checkpoints
git push --atomic origin main workshop-lab0 workshop-lab1 workshop-lab2 workshop-lab3 workshop-lab4 workshop-lab5 workshop-lab6 workshop-complete
```

If main advances independently, integrate those changes before publication.
Keep the intermediate commits: they are the student exercise checkpoints.
