# stackchan_fonts

`stackchan_font_cyr_16.c` is the caption font for Cyrillic (Mongolian, #13): GNU
Unifont 16.0.04's 8x16 bitmaps for U+0400 to U+052F, 304 glyphs, about 5 KB of
flash. Patch 0003 makes it the fallback after the CJK font in the caption font
(`caption_font()` in `muse_ui.c`), so a caption with Cyrillic in it shows it
instead of boxes.

Generated as the SDK generates its Unifont subsets (`tools/muse/gen_cjk_font.sh`),
from the same font file:

```sh
curl -fsSL -o unifont.otf https://unifoundry.com/pub/unifont/unifont-16.0.04/font-builds/unifont-16.0.04.otf
echo "0e3981ab552231b5a2a870f2b61741903a4bf25c23ef5aeb05fdced1b3c7af4d  unifont.otf" | shasum -a 256 -c -
npx -y lv_font_conv@1.5.3 --font unifont.otf --size 16 --bpp 1 --format lvgl \
    --lv-font-name stackchan_font_cyr_16 --no-compress -r 0x400-0x52F -o font.c
sed -e 's|#include "lvgl/lvgl.h"|#include "lvgl.h"|' font.c > stackchan_font_cyr_16.c
```

GNU Unifont is by Roman Czyborra, Paul Hardy and contributors
(https://unifoundry.com/unifont/). Its compiled fonts are licensed under the SIL
Open Font License, version 1.1 (https://openfontlicense.org), and under the GNU
GPL version 2 or later with the GNU font embedding exception. This file is a
conversion of an unaltered subset of the font.
