# 12 — Bot mass and centre-of-mass constants

**What to build:** The kinematic model's bot mass and centre-of-mass offset become board-profile values. The DLC32 MAX profile gets values estimated from the CAD assembly (board, drivers and chosen steppers), with a clear note on how to update them from a weighed bot.

**Blocked by:** 09

**Status:** done

- [x] Mass and centre-of-mass offset come from the profile, and the upstream values are unchanged
- [x] The DLC32 MAX values are estimated and documented

## Comments

Done (profile values in e578cac/c588ec6, rationale in docs/dlc32-max.md). botMassKg 0.90 (est.), comBelowAxesMm -2.0 (est., from CAD centroids: bottom centroid z -86.1 -> -77.6, cover -28.2 -> -12.0, bigger board ~30 mm higher). **Risk:** centre of mass near/above the belt tangent line -> top-heavy bot, more tilt under pen drag. Mitigation documented: counterweight low in the bottom part, re-estimate, adjust.
