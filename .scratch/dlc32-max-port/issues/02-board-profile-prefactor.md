# 02 — Board profile prefactor

**What to build:** All board-specific facts for the upstream board live in a single compile-time board profile: stepper STEP/DIR pins per side, enable pin and polarity, driver-config mode, servo pin, I2C pins, SD pins and the bot-mass constant. The movement, pen, display and storage modules read from it, and nothing else names a GPIO. Upstream behaviour is unchanged.

**Blocked by:** 01

**Status:** done

- [x] One board profile per target, selected by a build flag; the upstream target is the default
- [x] No GPIO literals remain outside the board profiles
- [x] The upstream target still builds, with pin values identical to before
- [x] A host-side (PlatformIO native) test asserts that each profile has no duplicate pins and uses no flash or other forbidden pins

## Comments

Done in e578cac. Deviation: no PlatformIO native test env (no host C++ compiler on the dev machine); the profile consistency checks are `static_assert`s in boardprofile.h instead, so they run in every firmware build (local + CI) - a higher seam than a native test. Profile also carries comBelowAxesMm (centre of mass) besides botMassKg.
