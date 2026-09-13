# Refreshed workshop checkpoints

The `new-lab2` through `new-lab6` tags provide the historical lab checkpoints
with the preinstalled workshop dependencies and committed Atlas v2 generated
headers. Use these tags with the `workshop` preset and the workshop-v1 image.
The original `lab*` tags and commits remain available.

## Student checkout commands

After the instructor publishes the refreshed history and tags, run these
commands on your computer from the project directory. Commit or stash any
work you want to keep before switching checkpoints.

```bash
git switch main
git pull --ff-only
git fetch origin --tags
git switch --detach new-lab2
./scripts/workshop.sh "$WORKSHOP_IMAGE"
```

Inside the container:

```bash
cmake --preset workshop && cmake --build --preset workshop && ctest --preset workshop
```

For another checkpoint, exit the container shell, run
`git switch --detach new-lab3` (or the tag the instructor specifies) on your
computer, and reopen the workshop shell. Keep `.build` for incremental builds.
All shells mounting the same checkout see the switch immediately.

## Commit mapping

`new-lab2` deliberately uses `e341939`, as requested for this refreshed series.
The original `lab2` tag points to `d8c32d1`, an earlier source state without the
existing temperature and AGENTS.md features. The original `lab0` and `lab1`
checkpoints are outside this refresh.

| Original commit | Rebuilt commit | New tag |
| --- | --- | --- |
| `64eb60ae50e0` | `026ed1efcc5c` | — |
| `e341939975d7` | `bc5cd3dc8ba0` | `new-lab2` |
| `534b3cb0cc3e` | `45fa71af7518` | `new-lab3` |
| `872904489bfc` | `ae8fd8d0035e` | `new-lab4` |
| `e02585c7c19e` | `446db15f0fa9` | — |
| `28afb08810a4` | `76915cca18d2` | — |
| `c6ee23bc870d` | `a6ba2821a92d` | `new-lab5` |
| `88514783793c` | `2f520eca43a2` | `new-lab6` |
| `9c641774a8e9` | `37374bb96509` | — |
| `156d52e46b30` | `4a9d2a4273cb` | — |
| `153ed9d81326` | `765c46782cba` | — |

## History and validation

The series starts on top of the previous main tip, `153ed9d81326`. Its first
commit restores the source state immediately before `e341939` and carries the
prebuilt environment setup back to that checkpoint. Each subsequent original
commit is replayed in order, with generated headers recreated by the image.
The original environment-setup commit is retained as an empty replay because
its changes are already present in the baseline. Commit messages record the
original source SHA and Atlas SHA.

All 11 reconstructed checkpoints passed the following using the existing
workshop image with Docker networking disabled:

- Configure with the installed dependencies and no populated `_deps` directory.
- Build and CTest using the `workshop` preset (GCC, Debug).
- Run the chat application with `--help`.
- Force a second Atlas generation and verify identical hashes for all three headers.
- Run an incremental build and verify Ninja reports no work to do.

The source-tree audit confirmed that each checkpoint matches its original
except for the carried environment files and regenerated headers. At the end
of the replay, only the three generated headers differ from the previous main
tip; this guide and its README link are added afterward.

Atlas commit: `96ebb2bcc86fb25b48c59b819c1dda522d645c32`.
The local image used for verification was:

```text
ghcr.io/jodyhagins/aipp101-starter/workshop@sha256:d6a8c8324c2e3c6a39994ea42daf59a41af9bae16e480052ea112b626efdd028
```

This validates the workshop preset in the local image. It does not claim a
new run of the entire compiler matrix or verification on both architectures.

## Publishing the series

The local `workshop-atlas-refresh` branch descends directly from the previous
`main`, so it can be integrated with a fast-forward merge. Publish the new tags
explicitly; no force push or replacement of old tags is needed.

```bash
git switch main
git merge --ff-only workshop-atlas-refresh
git push --atomic origin main new-lab2 new-lab3 new-lab4 new-lab5 new-lab6
```

If main has advanced independently, integrate those changes before publishing.
Do not squash the series: its intermediate commits are the student checkpoints.
