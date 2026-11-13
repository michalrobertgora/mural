# 02 — Board profile prefactor

**What to build:** All board-specific facts for the upstream board live in a single compile-time board profile: stepper STEP/DIR pins per side, enable pin and polarity, driver-config mode, servo pin, I2C pins, SD pins and the bot-mass constant. The movement, pen, display and storage modules read from it, and nothing else names a GPIO. Upstream behaviour is unchanged.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] One board profile per target, selected by a build flag; the upstream target is the default
- [ ] No GPIO literals remain outside the board profiles
- [ ] The upstream target still builds, with pin values identical to before
- [ ] A host-side (PlatformIO native) test asserts that each profile has no duplicate pins and uses no flash or other forbidden pins
