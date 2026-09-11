# Workshop environment

Students use one instructor-provided Docker image and the `workshop` CMake preset.
The image contains the compilers, Atlas, OpenSSL development files, and installed
CMake packages for every application and test dependency. Routine project builds
need no dependency downloads or third-party compilation.

Student-facing instructions: [AI++ 101: student setup](student-setup.md).
Once committed and pushed to `main`, share this link:

https://github.com/jodyhagins/aipp101-starter/blob/main/docs/student-setup.md

GitHub renders the guide directly; no separate website hosting is required.

## Before the workshop: instructor

1. Commit and push the project changes, including the workflow, to `main`. Open
   **Actions → Workshop image → Run workflow**, choosing a new tag such as
   `workshop-v1`.
2. The workflow builds on native AMD64 and ARM64 runners, runs
   `scripts/verify-workshop.sh` with networking disabled, then publishes the tested
   image to `ghcr.io/jodyhagins/aipp101-starter/workshop:<tag>`. Both architectures use the
   same Atlas commit, resolved from the tip of `main` at the start of the workflow.
3. Read the image digest from the workflow summary. Share the full immutable
   reference, for example `ghcr.io/jodyhagins/aipp101-starter/workshop@sha256:<digest>`.
   After the first publication, open the linked package under the repository's
   **Packages** section, open **Package settings**, and change its visibility to
   **Public** for unauthenticated student pulls. Public repository visibility alone
   does not guarantee public package visibility. Alternatively, arrange registry
   access in advance. See [GitHub's package visibility instructions](https://docs.github.com/en/packages/learn-github-packages/configuring-a-packages-access-control-and-visibility).
   The workflow uses the repository's `GITHUB_TOKEN`
   with package write permission; native ARM64 runners must be available.
4. Have students pull the image and complete a first build before class. Keep this
   image fixed throughout the workshop; publish a new tag for intentional updates.

The workflow is manual: pushing application changes does not publish a new image.
Creating this workflow does not itself upload an image.

To build and verify locally instead:

```bash
docker build -f docker/Dockerfile -t aipp101-workshop:workshop-v1 .
docker run --rm --network=none \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  aipp101-workshop:workshop-v1 bash /source/scripts/verify-workshop.sh
```

A local build targets the host's architecture. The publishing workflow handles both
architectures. `--build-arg ATLAS_REF=<commit>` can reproduce a known Atlas version;
the default is `main` and remote commit metadata refreshes the Docker layer when
that branch changes.

## Before the workshop: student

Use Docker Desktop on macOS/Windows, or Docker Engine on Linux. On Windows, run
these commands from a WSL2 Linux shell and keep the checkout in the WSL filesystem.

Set the exact image reference supplied by the instructor:

```bash
export WORKSHOP_IMAGE='ghcr.io/jodyhagins/aipp101-starter/workshop@sha256:<digest>'
docker pull "$WORKSHOP_IMAGE"
./scripts/workshop.sh "$WORKSHOP_IMAGE"
```

The launcher opens a shell at `/workspace`, with the checkout mounted there. It
uses your host UID/GID to avoid creating root-owned files and requires the image
already to be present. Inside that shell:

```bash
cp .env.example .env
# Edit .env and set OPENROUTER_API_KEY.
cmake --preset workshop
cmake --build --preset workshop
ctest --preset workshop
.build/workshop/src/wjh/apps/chat/chat_app
```

The API key stays in the checkout's ignored `.env` file, not in the image. The
workshop preset uses GCC, Debug, tests enabled, and AddressSanitizer disabled.
Builds use two parallel jobs to limit memory usage on student laptops.

## During the workshop

Pull changes from the host shell, then build in the container:

```bash
# Host shell, in this checkout:
git pull
./scripts/workshop.sh "$WORKSHOP_IMAGE"

# Container shell:
cmake --preset workshop
cmake --build --preset workshop
ctest --preset workshop
```

Keep `.build` between sessions. The bind mount preserves both `.build/workshop`
and `.build/.ccache` even though the launcher removes the container when you exit.
Use the same image, preset, and `/workspace` mount path. Do not build the `workshop`
preset on the host or reuse its build directory across CPU architectures.
Other presets have separate build directories.

CMake refuses to fetch missing dependencies in workshop mode. If the checkout's
dependency pins change, it detects an outdated workshop image and asks for an
updated image. Application and generated-header changes still cause the affected
application/test files to compile; stable dependencies stay installed in the image.

## Dependency maintenance

`cmake/DependencySources.cmake` is the shared source of pinned revisions for Docker
and ordinary development builds. It preserves the instructor's doctest and
RapidCheck forks. The image installs these packages under `/opt/wjh-deps` and adds
that prefix to `CMAKE_PREFIX_PATH`; Atlas is installed under `/usr/local` and is on
`PATH`. The source pins are also recorded under
`/opt/wjh-deps/share/wjh-workshop/` in the image.

RapidCheck (including its doctest integration) and cpp-httplib are precompiled
static libraries. The remaining libraries provide installed headers and CMake
packages. HTTPS remains required. The workshop packages are built with GCC and
libstdc++; the supported workshop configuration is the tested `workshop` preset.
Other development presets remain available, but their compiler/standard-library
compatibility should be checked before using precompiled packages. Sanitized
application builds do not add sanitizer instrumentation to prebuilt libraries.

To update a dependency, edit its commit in `DependencySources.cmake` and adjust
any corresponding package version requirement in `ThirdParty.cmake`. Build and
test a new image, publish a new tag, and distribute its digest. Atlas continues to track
`main` at image-build time. For a deliberate dependency development build, use a
separate build directory and disable that package's discovery with
`-DCMAKE_DISABLE_FIND_PACKAGE_<package>=ON`; leave
`WJH_CHAT_REQUIRE_INSTALLED_DEPS=OFF`. Ordinary presets prefer compatible installed
packages and fetch the declared revisions when absent.
