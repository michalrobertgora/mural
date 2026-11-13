# 01 — Windows build baseline

**What to build:** A maker on Windows can run the full PlatformIO build of the existing upstream target from a clean checkout: worker, React UI, firmware and filesystem image. PlatformIO is installed with `uv`. The pre-build script uses portable file operations instead of shell `rm`/`cp`, and behaves the same on Linux and in the devcontainer.

**Blocked by:** None — can start immediately

**Status:** ready-for-agent

- [ ] PlatformIO is available on the human's Windows machine, installed with `uv tool`
- [ ] The pre-build script uses only portable Python file operations, with no shell-specific commands
- [ ] The upstream target's firmware builds green on Windows
- [ ] The upstream target's filesystem image builds green on Windows
