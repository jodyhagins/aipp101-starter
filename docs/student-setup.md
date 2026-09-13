# AI++ 101: student setup

Complete these steps before the workshop. You will install Docker and Git,
download the workshop environment, and build the starter project inside it.
The downloaded image includes the C++ compiler, CMake, Atlas, and the project's
third-party dependencies. You do not need to build the Docker image yourself.

Have the **workshop image reference** supplied by your instructor ready. It will
start with `ghcr.io/jodyhagins/aipp101-starter/workshop` and identify the version
for your class. Image downloads can be several gigabytes; allow time before class.

## 1. Install and start Docker

Follow the official instructions for your system:

| System | Install | Terminal to use for this guide |
| --- | --- | --- |
| macOS, Apple silicon | [Docker Desktop for Mac](https://docs.docker.com/desktop/setup/install/mac-install/) — choose Apple silicon | Terminal |
| macOS, Intel | [Docker Desktop for Mac](https://docs.docker.com/desktop/setup/install/mac-install/) — choose Intel | Terminal |
| Windows | [Docker Desktop for Windows](https://docs.docker.com/desktop/setup/install/windows-install/) with the WSL 2 backend | Your Ubuntu/WSL terminal |
| Ubuntu Linux | [Docker Engine for Ubuntu](https://docs.docker.com/engine/install/ubuntu/) | Your usual terminal |
| Other Linux distributions | [Docker Engine installation guides](https://docs.docker.com/engine/install/) | Your usual terminal |

On macOS and Windows, **open Docker Desktop and wait for it to finish starting**.
On Windows, enable integration with your Ubuntu/WSL distribution following
[Docker's WSL instructions](https://docs.docker.com/desktop/features/wsl/). Use
Linux containers and run the remaining commands in WSL, not PowerShell or cmd.exe.
Keep the checkout in your WSL home directory rather than under `/mnt/c`.

On Linux, follow the
[post-installation instructions](https://docs.docker.com/engine/install/linux-postinstall/)
so your account can run Docker commands without `sudo`.

Verify that Docker is running:

```bash
docker info
```

This should display both client and server information without a connection or
permission error.

## 2. Install Git and download the project

Check whether Git is installed:

```bash
git --version
```

If needed, follow [GitHub's Git setup instructions](https://docs.github.com/en/get-started/git-basics/set-up-git).
On Windows, install Git inside your WSL distribution.

In your terminal:

```bash
mkdir -p ~/projects
cd ~/projects
git clone https://github.com/jodyhagins/aipp101-starter.git
cd aipp101-starter
```

If you already have the checkout, enter its directory and run `git pull` instead.
Keep your own edits committed or saved before pulling workshop updates.

## 3. Download the workshop image

Replace the entire example value below with the image reference your instructor
provides. A reference ending in `@sha256:...` identifies the exact image for your
class; a reference ending in `:workshop-v1` is a version tag.

```bash
export WORKSHOP_IMAGE='PASTE_THE_IMAGE_REFERENCE_FROM_YOUR_INSTRUCTOR_HERE'
docker pull "$WORKSHOP_IMAGE"
```

For this project the image is hosted on **GitHub Container Registry (GHCR)**,
associated with the repository's Packages section. Docker chooses the appropriate
AMD64 or ARM64 image for your machine. A public workshop image does not require a
GitHub account or registry login to download.

Save the instructor's reference: set `WORKSHOP_IMAGE` again when opening a new
terminal. An image must have been published before `docker pull` can download it;
ask your instructor if you have not received the class image reference yet.

## 4. Enter the workshop environment

From the `aipp101-starter` checkout on your computer:

```bash
./scripts/workshop.sh "$WORKSHOP_IMAGE"
```

You are now in a Linux shell **inside the container**, in `/workspace`. This is the
same project directory you see on your computer. You can edit files with your
usual editor on your computer and build them in this shell.

## 5. Build and test the project

Run these commands **inside the container**:

```bash
cmake --preset workshop
cmake --build --preset workshop
ctest --preset workshop
```

The tests should report `100% tests passed`. The first project build compiles the
application and its tests; the dependencies are already installed. Later builds
only compile what changed.

Building and running the tests does not require an OpenRouter API key.

## 6. Configure and run the chat app

Before running the chat app, create `.env` if it does not already exist:

```bash
test -f .env || cp .env.example .env
```

Open `.env` in your editor on your computer and set `OPENROUTER_API_KEY` to the
key supplied for the workshop or your own key. This file is ignored by Git.

Back in the container shell:

```bash
.build/workshop/src/wjh/apps/chat/chat_app
```

Use `/exit` to leave the chat app. Use `exit` to leave the container shell.
Your project files, `.env`, build outputs, and compiler cache remain in the checkout.

## During the workshop: get changes and rebuild

From a terminal **on your computer**, in the checkout:

```bash
git pull
./scripts/workshop.sh "$WORKSHOP_IMAGE"
```

Then, **inside the container**:

```bash
cmake --preset workshop
cmake --build --preset workshop
ctest --preset workshop
```

Use the same image and `workshop` preset throughout class. Keep `.build` so your
build outputs and compiler cache survive container restarts. You only need to
pull a new image if the instructor announces an environment update.

## If something goes wrong

| Problem | What to check |
| --- | --- |
| `docker: command not found` | Complete installation, reopen your terminal, and on Windows check WSL integration. |
| Cannot connect to Docker | Start Docker Desktop, or check that Docker Engine is running on Linux. |
| Permission denied connecting to Docker on Linux | Complete Docker's Linux post-installation setup and reopen your login session. |
| Image download says `denied` or `manifest unknown` | Check the exact instructor-provided reference. The instructor may still need to publish the image or make its package public. |
| The launcher says the image is unavailable | Run `docker pull "$WORKSHOP_IMAGE"` first; the launcher does not download automatically. |
| CMake reports a missing installed dependency or outdated image | Confirm you launched the instructor's image and ask whether a new image was released. |
| CMake is missing or uses paths from your host system | Run the build inside the container with the `workshop` preset. |

For instructors, see [image publishing and maintenance](workshop.md).
