# 01 — Windows build baseline

**What to build:** A maker on Windows can run the full PlatformIO build of the existing upstream target from a clean checkout: worker, React UI, firmware and filesystem image. PlatformIO is installed with `uv`. The pre-build script uses portable file operations instead of shell `rm`/`cp`, and behaves the same on Linux and in the devcontainer.

**Blocked by:** None — can start immediately

**Status:** done

- [x] PlatformIO is available on the human's Windows machine, installed with `uv tool`
- [x] The pre-build script uses only portable Python file operations, with no shell-specific commands
- [x] The upstream target's firmware builds green on Windows
- [x] The upstream target's filesystem image builds green on Windows

## Comments

Done in 718229c. Two Windows-only failures fixed: (1) worker dep `canvas` needs node-gyp+cairo -> worker installs with `--ignore-scripts` (canvas is only used by the dev-only tester.ts); (2) PlatformIO's output-echo thread crashed on vite's Unicode check mark under cp1252 and hung the build -> build.py re-prints npm output as ASCII. Upstream target: RAM 14.9%, Flash 94.1%.
