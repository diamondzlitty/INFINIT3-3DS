# INFINIT3 TERMINAL CIA packaging

The SMDH is built from `apps/terminal/icon.png`, restored from the original
DSi INFINIT3 infinity-mark asset. The selected-title banner remains the
separate grayscale terminal banner.

The title ID is `000400000494E300` (derived from project-local unique ID
`0x494E3`). The CIA
is a CFW/homebrew package and is built with test signing (`-target t`).

The generated banner deliberately stays grayscale to match the terminal UI.
It is a banner only; the SMDH provides the small HOME Menu icon. The local
`bannertool` can package a supplied CGFX model, but this tree has no CGFX
authoring/conversion tool or animated model asset, so T5 keeps the working
static banner and its existing sound instead of risking the release build.

Run `build_cia.sh` after the normal `.3dsx` build. It expects temporary
makerom and bannertool folders at `/private/tmp/infinit3-cia-tools`, or a
different path in `INFINIT3_CIA_TOOLS`. The checked-in `banner.png` is the
release input; `banner.svg` remains its editable source.
