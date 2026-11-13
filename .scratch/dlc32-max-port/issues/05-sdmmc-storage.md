# 05 — SDMMC storage implementation

**What to build:** On the DLC32 MAX, the onboard TF slot is used through 4-bit SDMMC behind the storage interface. The pins are CLK 13, CMD 14, D0 12, D1 11, D2 47, D3 21 and card-detect 10. Card-missing and unformatted states are reported on the display and serial like upstream. On-device formatting may degrade to "format FAT32 on a PC".

**Blocked by:** 03, 04

**Status:** done

- [x] An SDMMC implementation of the storage interface exists
- [x] The DLC32 MAX target selects the SDMMC implementation through its profile
- [x] Missing and unformatted cards produce a clear status for the user
- [x] Both targets build green

## Comments

Done in e578cac/c588ec6: SDMMC backend (4-bit, setPins) with card-detect pin; present-but-unmountable card goes through the same format countdown as upstream. Card-detect polarity is verified at bring-up (08).
