# 07 — CI for both targets

**What to build:** A GitHub Actions workflow on the fork that runs on every push. It builds the worker and UI, compiles both firmware targets (and the bring-up environment once it exists), and runs the host-side tests.

**Blocked by:** 04

**Status:** ready-for-agent

- [ ] The workflow builds both targets and runs the native tests
- [ ] The workflow is green on the fork's branch
