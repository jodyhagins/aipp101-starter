#!/usr/bin/env python3
"""Verify exercise starting trees, feature boundaries, and workshop build setup."""
from pathlib import Path
import subprocess

REPO = Path(__file__).resolve().parents[1]
ENVIRONMENT_COMMIT = '94749e2d9a39d509774f753d83ed7f87fcd54a71'
GENERATED = {
    'src/wjh/chat/types_gen.hpp',
    'src/wjh/chat/client/types_gen.hpp',
    'src/wjh/chat/conversation/types_gen.hpp',
}
# Values: original source SHA, completed lab 2, API definitions, agent loop, file tools.
CHECKPOINTS = {
    'workshop-lab0': ('d8c32d1999a356c07d791f948e0f930531bbcd46', False, False, False, False),
    'workshop-lab1': ('d8c32d1999a356c07d791f948e0f930531bbcd46', False, False, False, False),
    'workshop-lab2': ('d8c32d1999a356c07d791f948e0f930531bbcd46', False, False, False, False),
    'workshop-lab3': ('534b3cb0cc3edb854b1c4456e9793d9d045b5d25', True, False, False, False),
    'workshop-lab4': ('872904489bfcb4e1a8491a01ca2541d4d72c6dbb', True, False, False, False),
    'workshop-lab5': ('e02585c7c19e9e77874314fe44fc4923f444fc3c', True, True, False, False),
    'workshop-lab6': ('c6ee23bc870d627457276717af0ab87f281c910f', True, True, True, False),
    'workshop-complete': ('153ed9d813260fd2f7051ecf2d05e4025fcadcbb', True, True, True, True),
}


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args], text=True).strip()


def source(ref, path):
    return git('show', f'{ref}:{path}')


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def verify_checkpoint(tag, actual_ref=None):
    ref = actual_ref or tag
    original, lab2, definitions, loop, file_tools = CHECKPOINTS[tag]
    changed = git('diff', '--name-only', original, ref, '--', 'src').splitlines()
    unexpected = [p for p in changed if p not in GENERATED]
    require(not unexpected, f'{tag} ({ref}) has the wrong exercise source: {unexpected}')
    cpp = source(ref, 'src/wjh/chat/client/OpenRouterClient.cpp')
    markers = [
        ('temperature type', '[class Temperature]', source(ref, 'src/wjh/chat/types.atlas'), lab2),
        ('temperature CLI', '--temperature', source(ref, 'src/wjh/chat/CommandLine.cpp'), lab2),
        ('AGENTS.md loading', 'AGENTS.md', source(ref, 'src/wjh/chat/Config.cpp'), lab2),
        ('usage command', 'cmd == "/usage"', source(ref, 'src/wjh/chat/ChatLoop.cpp'), lab2),
        ('API tool definitions', 'make_tools_json', cpp, definitions),
        ('bash execution', 'execute_bash', cpp, loop),
        ('agent loop', 'Agent loop exceeded 20 iterations', cpp, loop),
        ('read_file', 'execute_read_file', cpp, file_tools),
        ('write_file', 'execute_write_file', cpp, file_tools),
        ('edit_file', 'execute_edit_file', cpp, file_tools),
    ]
    for name, marker, content, expected in markers:
        require((marker in content) == expected,
                f'{tag}: {name} must be {"implemented" if expected else "unfinished"}')
    for path in GENERATED:
        require('Atlas Strong Type Generator v2.0.0' in source(ref, path),
                f'{tag}: {path} was not regenerated with the workshop Atlas')
    for path in ['CMakePresets.json', 'cmake/ThirdParty.cmake',
                 'cmake/DependencySources.cmake', 'scripts/workshop.sh',
                 'scripts/verify-workshop.sh', 'docker/Dockerfile',
                 'docker/dependencies/CMakeLists.txt', '.dockerignore',
                 '.github/workflows/workshop-image.yml',
                 'docs/student-setup.md', 'docs/workshop.md']:
        require(source(ref, path) == source(ENVIRONMENT_COMMIT, path),
                f'{tag}: workshop environment differs in {path}')
    return git('rev-parse', ref + '^{commit}')


def main():
    previous = None
    for tag in CHECKPOINTS:
        commit = verify_checkpoint(tag)
        if previous:
            subprocess.run(['git', '-C', str(REPO), 'merge-base', '--is-ancestor',
                            previous, commit], check=True)
        print(f'{tag}: {commit[:12]} — source and exercise boundary verified')
        previous = commit


if __name__ == '__main__':
    main()
