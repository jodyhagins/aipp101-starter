#!/usr/bin/env bash
# Run inside the workshop image with --network=none. Never modifies the checkout.
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
workshop_test_dir=$(mktemp -d)
trap 'rm -rf "$workshop_test_dir"' EXIT
cp -R "$project_dir"/{CMakeLists.txt,CMakePresets.json,cmake,src,scripts} "$workshop_test_dir/"
cd "$workshop_test_dir"

# An older image must not silently use a different set of dependency pins.
cmp cmake/DependencySources.cmake /opt/wjh-deps/share/wjh-workshop/DependencySources.cmake
atlas --version
cmake --preset workshop
if [[ -d .build/workshop/_deps ]]; then
    echo 'Workshop configuration unexpectedly populated third-party sources.' >&2
    exit 1
fi
cmake --build --preset workshop
ctest --preset workshop
.build/workshop/src/wjh/apps/chat/chat_app --help
cmake --build --preset workshop | tee rebuild.log
grep -q 'ninja: no work to do.' rebuild.log

# Every dependency must fail clearly when unavailable, without fetching a fallback.
for package in Atlas tl-expected nlohmann_json httplib laserpants_dotenv doctest rapidcheck; do
    if cmake --preset workshop -B ".build/missing-$package" \
        "-DCMAKE_DISABLE_FIND_PACKAGE_$package=ON" >missing.log 2>&1; then
        echo "Expected configuration to fail without installed $package." >&2
        exit 1
    fi
    grep -q "Required installed dependency '$package'" missing.log
done

# A checkout with changed pins must reject the old workshop image.
printf '\n# Simulated dependency manifest update\n' >> cmake/DependencySources.cmake
if cmake --preset workshop -B .build/stale-image >stale.log 2>&1; then
    echo 'Expected configuration to reject the outdated workshop image.' >&2
    exit 1
fi
grep -q "dependency pins differ" stale.log
