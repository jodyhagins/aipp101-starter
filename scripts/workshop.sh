#!/usr/bin/env bash
# Run the checkout in a previously pulled workshop image; keep .build on the host.
set -euo pipefail
if [[ $# -eq 0 ]]; then
    echo "Usage: $0 IMAGE[:TAG|@DIGEST] [COMMAND ...]" >&2
    exit 2
fi
workshop_image=$1
shift
if [[ $# -eq 0 ]]; then
    set -- bash
fi
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
terminal_flag=-i
if [[ -t 0 && -t 1 ]]; then
    terminal_flag=-it
fi
exec docker run --rm --init --pull=never "$terminal_flag" \
    --user "$(id -u):$(id -g)" --env HOME=/tmp \
    --mount "type=bind,source=$project_dir,target=/workspace" \
    --workdir /workspace "$workshop_image" "$@"
