#!/usr/bin/env python3
"""Developer helper: on an SDK clone with patches 0000-0005 applied, turns the 3D-style
avatar of upstream PR #513 (patch 0000) on for the M5Stack CoreS3 (#16), writes the diff
to patches/0006-3d-avatar-cores3.patch, then restores the files (apply.sh applies the
patch for real).
Run: python3 make-3d-cores3-patch.py <sdk>/esp32 <out.patch>"""
import sys, pathlib, subprocess, tempfile, shutil

root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
CMAKE = "components/muse/CMakeLists.txt"
UI = "components/muse/muse_ui.c"
BOARD = "components/muse/boards/board_m5stack_cores3.c"
AVATAR_PX = 240   # the layout picks 160 (2/3 of 240) for this short landscape screen
STUB = "components/muse/boards/muse_lcd_bands_none.c"

STUB_SRC = r'''/* stackchan-muse: the transfer counters of muse_lcd_bands.c for a board that draws
 * through esp_lvgl_port instead (the M5Stack CoreS3). The cached 3D avatar (upstream PR
 * #513) waits for its patch to reach the panel before showing the next expression: it
 * notes submitted() when it invalidates, again at LVGL's RENDER_READY, then waits for
 * completed() to reach that. Here each call to submitted() is a new ticket and completed()
 * is the latest one, so a refresh counts as presented when it has been rendered; the
 * port's flush follows straight after. Nothing fails. */
#include <stdint.h>

static uint32_t s_ticket;

uint32_t muse_lcd_bands_submitted(void) { return __atomic_add_fetch(&s_ticket, 1, __ATOMIC_RELAXED); }
uint32_t muse_lcd_bands_completed(void) { return __atomic_load_n(&s_ticket, __ATOMIC_RELAXED); }
uint32_t muse_lcd_bands_failures(void) { return 0; }
'''


def sub1(s, old, new):
    assert s.count(old) == 1, old
    return s.replace(old, new)


def cmake(s):
    s = sub1(s, "    elseif(CONFIG_MUSE_BOARD_WAVESHARE_S3_175C)\n        set(muse_cached_avatar ON)\n",
             "    elseif(CONFIG_MUSE_BOARD_WAVESHARE_S3_175C OR CONFIG_MUSE_BOARD_M5STACK_CORES3)\n"
             "        # stackchan-muse: the 3D-style avatar on the CoreS3 too (#16)\n"
             "        set(muse_cached_avatar ON)\n")
    return sub1(s, '        list(APPEND embed "../../avatar/3d/muse3d.bin")\n',
                '        list(APPEND embed "../../avatar/3d/muse3d.bin")\n'
                '        if(CONFIG_MUSE_BOARD_M5STACK_CORES3)\n'
                '            list(APPEND srcs "boards/muse_lcd_bands_none.c")   # no LCD bands on this board\n'
                '        endif()\n')


HEARTS_SRC = r'''
#if MUSE_CACHED_AVATAR
/* stackchan-muse (#16): hearts over the 3D avatar while it is happy (a pat), as the built-in
 * character draws them. The poses have none of their own. Three small A8 images rise from
 * above the head and fade; the 3D renderer is not involved. */
static const uint8_t s_heart_px[9 * 8] = {
    0,255,255,0,0,0,255,255,0,
    255,255,255,255,0,255,255,255,255,
    255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,
    0,255,255,255,255,255,255,255,0,
    0,0,255,255,255,255,255,0,0,
    0,0,0,255,255,255,0,0,0,
    0,0,0,0,255,0,0,0,0,
};
static const lv_image_dsc_t s_heart_dsc = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8, .w = 9, .h = 8, .stride = 9},
    .data_size = sizeof(s_heart_px),
    .data = s_heart_px,
};
#define HEARTS 3
static lv_obj_t *s_hearts[HEARTS];

static void hearts_tick(float happy, bool visible)
{
    if (!s_canvas) return;
    if (!s_hearts[0]) {
        for (int i = 0; i < HEARTS; i++) {
            s_hearts[i] = lv_image_create(lv_obj_get_parent(s_canvas));
            lv_image_set_src(s_hearts[i], &s_heart_dsc);
            lv_image_set_scale(s_hearts[i], 512);   /* 18 x 16 */
            lv_obj_set_style_image_recolor(s_hearts[i], lv_color_hex(0xff4f8b), 0);
            lv_obj_set_style_image_recolor_opa(s_hearts[i], LV_OPA_COVER, 0);
            lv_obj_add_flag(s_hearts[i], LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_IGNORE_LAYOUT);
            lv_obj_remove_flag(s_hearts[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }
    bool show = visible && happy > 0;
    int32_t size = lv_obj_get_width(s_canvas);
    int32_t x0 = lv_obj_get_x(s_canvas), y0 = lv_obj_get_y(s_canvas);
    float t = (float)esp_timer_get_time() / 1e6f;
    for (int i = 0; i < HEARTS; i++) {
        if (!show) {
            lv_obj_add_flag(s_hearts[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        float phase = t * 0.7f + i / (float)HEARTS;   /* each rises over about 1.4 s */
        phase -= (int)phase;
        int32_t x = x0 + size * 60 / 100 + i * size / 9 + (int32_t)(sinf(phase * 6.28f + i) * size / 40);
        int32_t y = y0 + size * 22 / 100 - (int32_t)(phase * size * 0.25f);
        lv_obj_set_pos(s_hearts[i], x, y);
        lv_obj_set_style_image_opa(s_hearts[i], (lv_opa_t)(255 * (1.0f - phase) * (happy < 1 ? happy : 1)), 0);
        lv_obj_remove_flag(s_hearts[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_hearts[i]);
    }
}
#endif
'''


def ui(s):
    # Hearts while happy, after the frame's avatar update.
    s = sub1(s, "static void frame_tick(lv_timer_t *timer)\n{\n", HEARTS_SRC + "\nstatic void frame_tick(lv_timer_t *timer)\n{\n")
    s = sub1(s, "        if (s_was_visible) update_cached_avatar(mode, &pose, !was_visible);\n",
             "        if (s_was_visible) update_cached_avatar(mode, &pose, !was_visible);\n"
             "        hearts_tick(pose.happy, s_was_visible);   /* stackchan-muse */\n")
    # muse_lcd_bands.h needs esp_lv_adapter.h, which a board drawing through esp_lvgl_port
    # does not have: declare just the three counters there.
    return sub1(s, '#include "boards/muse_lcd_bands.h"\n',
                "#if CONFIG_MUSE_BOARD_M5STACK_CORES3   /* stackchan-muse: no LCD bands (muse_lcd_bands_none.c) */\n"
                "uint32_t muse_lcd_bands_submitted(void);\n"
                "uint32_t muse_lcd_bands_completed(void);\n"
                "uint32_t muse_lcd_bands_failures(void);\n"
                "#else\n"
                '#include "boards/muse_lcd_bands.h"\n'
                "#endif\n")


def board(s):
    # A larger 3D avatar than the layout's default for this screen.
    return sub1(s, "    .round = false,\n    .touch = true,\n",
                "    .round = false,\n"
                "#if MUSE_CACHED_AVATAR\n"
                f"    .avatar_px = {AVATAR_PX},   /* stackchan-muse: the 3D avatar larger than the default 160 (#16) */\n"
                "#endif\n"
                "    .touch = true,\n")


EDITS = {CMAKE: cmake, UI: ui, BOARD: board}

tmp = pathlib.Path(tempfile.mkdtemp())
try:
    for rel in EDITS:
        dst = tmp / "a" / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(root / rel, dst)
    for rel, fn in EDITS.items():
        (root / rel).write_text(fn((root / rel).read_text()))
    (root / STUB).write_text(STUB_SRC)
    chunks = []
    for rel in EDITS:
        r = subprocess.run(["diff", "-u", str(tmp / "a" / rel), str(root / rel)], capture_output=True, text=True)
        body = r.stdout.split("\n", 2)[2]
        chunks.append(f"diff --git a/esp32/{rel} b/esp32/{rel}\n--- a/esp32/{rel}\n+++ b/esp32/{rel}\n{body}")
    lines = STUB_SRC.splitlines()
    chunks.append(f"diff --git a/esp32/{STUB} b/esp32/{STUB}\nnew file mode 100644\n--- /dev/null\n+++ b/esp32/{STUB}\n"
                  f"@@ -0,0 +1,{len(lines)} @@\n" + "".join("+" + l + "\n" for l in lines))
    out.write_text("".join(chunks))
finally:
    for rel in EDITS:
        shutil.copy(tmp / "a" / rel, root / rel)
    (root / STUB).unlink(missing_ok=True)
    shutil.rmtree(tmp)
print("wrote", out)
