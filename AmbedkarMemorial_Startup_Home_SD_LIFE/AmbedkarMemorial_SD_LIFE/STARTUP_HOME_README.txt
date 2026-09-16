STARTUP + HOME BUILD

This build adds:
- Five-stage startup animation with only a handful of full-screen redraws.
- Redesigned 480x320 Home screen.
- Existing SdFat/LIFE image architecture preserved.
- LIFE button enters the existing LIFE renderer.
- Other Home buttons are intentionally held as placeholders until their
  individual screens are implemented.

The startup animation is intentionally short and static between stages:
there is no continuous animation loop, to avoid the repeated-redraw issue
seen earlier.

IMPORTANT:
This is a UI milestone. Test:
1. Boot animation.
2. Home layout.
3. Tap LIFE.
4. Verify LIFE photos still work.
