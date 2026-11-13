# 03 — Storage interface prefactor

**What to build:** The firmware reaches "the card" through one storage interface, which provides the filesystem handle, total and used bytes, and a mount result that covers missing and unformatted cards. The phase manager, SVG upload and runner use it. The SPI-SD implementation keeps upstream's detect, format and halt behaviour exactly.

**Blocked by:** 01

**Status:** done

- [x] The storage interface exists, and nothing outside the SPI implementation uses the SPI SD global directly
- [x] The SPI implementation keeps upstream's card-missing, format-countdown and mount behaviour
- [x] The upstream target builds green

## Comments

Done in e578cac (storage.h/.cpp; sdcard.* removed). SPI path is upstream's logic verbatim.
