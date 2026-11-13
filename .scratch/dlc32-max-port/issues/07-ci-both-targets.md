# 07 — CI for both targets

**What to build:** A GitHub Actions workflow on the fork that runs on every push. It builds the worker and UI, compiles both firmware targets (and the bring-up environment once it exists), and runs the host-side tests.

**Blocked by:** 04

**Status:** done

- [x] The workflow builds both targets and runs the native tests
- [x] The workflow is green on the fork's branch

## Comments

Done in 39972e3. First run green on dlc32-max: https://github.com/michalrobertgora/mural/actions/runs/43492972768 (esp32dev, mks_dlc32_max, mks_dlc32_max_bringup; firmware + LittleFS images uploaded as artifacts). No native tests exist; profile checks are compile-time (see 02).
