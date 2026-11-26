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
#include "stackchan_hw.h"   /* stackchan_hw_overjoyed() */
/* stackchan-muse (#16): hearts over the 3D avatar while it is happy (a pat), as the built-in
 * character draws them. The poses have none of their own. Three small A8 images rise from
 * above the head and fade; the 3D renderer is not involved. */
static const uint8_t s_fx_heart_px[9 * 8] = {
    0,255,255,0,0,0,255,255,0,
    255,255,255,255,0,255,255,255,255,
    255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,
    0,255,255,255,255,255,255,255,0,
    0,0,255,255,255,255,255,0,0,
    0,0,0,255,255,255,0,0,0,
    0,0,0,0,255,0,0,0,0,
};
static const lv_image_dsc_t s_fx_heart_dsc = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8, .w = 9, .h = 8, .stride = 9},
    .data_size = sizeof(s_fx_heart_px),
    .data = s_fx_heart_px,
};
/* Three hearts when happy; when overjoyed (six pats, the head's full turn) lots of them,
 * across the top of the screen, quicker. */
#define FX_HEARTS 3
#define FX_HEARTS_MAX 40
static lv_obj_t *s_fx_hearts[FX_HEARTS_MAX];

static void fx_hearts_tick(float happy, bool visible)
{
    if (!s_canvas) return;
    if (!s_fx_hearts[0]) {
        for (int i = 0; i < FX_HEARTS_MAX; i++) {
            s_fx_hearts[i] = lv_image_create(lv_obj_get_parent(s_canvas));
            lv_image_set_src(s_fx_hearts[i], &s_fx_heart_dsc);
            /* The first three (a pat) 16-20 px; the rest (overjoyed) 14-27 px, mixed. */
            lv_image_set_scale(s_fx_hearts[i], i < FX_HEARTS ? (i % 3 ? 448 : 576) : 384 + (i % 4) * 128);
            lv_obj_set_style_image_recolor(s_fx_hearts[i], lv_color_hex(i % 4 == 3 ? 0xff86b5 : 0xff4f8b), 0);
            lv_obj_set_style_image_recolor_opa(s_fx_hearts[i], LV_OPA_COVER, 0);
            lv_obj_add_flag(s_fx_hearts[i], LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_IGNORE_LAYOUT);
            lv_obj_remove_flag(s_fx_hearts[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }
    bool overjoyed = stackchan_hw_overjoyed();
    int count = visible && happy > 0 ? (overjoyed ? FX_HEARTS_MAX : FX_HEARTS) : 0;
    int32_t size = lv_obj_get_width(s_canvas);
    int32_t x0 = lv_obj_get_x(s_canvas), y0 = lv_obj_get_y(s_canvas);
    float t = (float)esp_timer_get_time() / 1e6f;
    for (int i = 0; i < FX_HEARTS_MAX; i++) {
        if (i >= count) {
            lv_obj_add_flag(s_fx_hearts[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        int32_t x, y;
        float phase;
        if (overjoyed) {
            /* Fill the screen: each heart has its own column and speed and rises from the
             * bottom edge to the top, offset by the golden ratio so they never line up. */
            lv_obj_t *parent = lv_obj_get_parent(s_canvas);
            int32_t pw = lv_obj_get_width(parent), ph = lv_obj_get_height(parent);
            float speed = 0.45f + 0.25f * ((i * 7) % 5) / 4.0f;   /* 1.4-2.2 s to cross */
            phase = t * speed + i * 0.618f;
            phase -= (int)phase;
            x = pw * ((i * 37) % 100) / 100 - 10 + (int32_t)(sinf(phase * 6.28f * 1.5f + i) * 8);
            y = ph + 10 - (int32_t)(phase * (ph + 40));
        } else {
            phase = t * 0.7f + i / (float)count;   /* rising over about 1.4 s */
            phase -= (int)phase;
            x = x0 + size * 60 / 100 + i * size / 9 + (int32_t)(sinf(phase * 6.28f + i) * size / 40);
            y = y0 + size * 22 / 100 - (int32_t)(phase * size * 0.25f);
        }
        lv_obj_set_pos(s_fx_hearts[i], x, y);
        /* A pat's hearts fade as they rise; overjoyed ones stay bright until the top fifth. */
        float fade = overjoyed ? (phase < 0.8f ? 1.0f : (1.0f - phase) * 5.0f) : 1.0f - phase;
        lv_obj_set_style_image_opa(s_fx_hearts[i], (lv_opa_t)(255 * fade * (happy < 1 ? happy : 1)), 0);
        lv_obj_remove_flag(s_fx_hearts[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_fx_hearts[i]);
    }
}

/* stackchan-muse (#16): an effect per state around the 3D avatar, drawn with small LVGL
 * images and arcs (each object is small, so only small areas redraw):
 *   idle       white sparkles twinkling, upper left and right
 *   listening  blue waves spreading out to both sides, quick
 *   thinking   pink sparkles twinkling above the head
 *   speaking   green waves spreading out to both sides, slow
 *   happy      the hearts above, and a few faint white sparkles */
static const uint8_t s_fx_spark_px[7 * 7] = {
    0,0,0,255,0,0,0,
    0,0,0,255,0,0,0,
    0,0,128,255,128,0,0,
    255,255,255,255,255,255,255,
    0,0,128,255,128,0,0,
    0,0,0,255,0,0,0,
    0,0,0,255,0,0,0,
};
static const lv_image_dsc_t s_fx_spark_dsc = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8, .w = 7, .h = 7, .stride = 7},
    .data_size = sizeof(s_fx_spark_px),
    .data = s_fx_spark_px,
};

#define FX_SPARKS 6
#define FX_DOTS 5
#define FX_WAVES 3                  /* per side */
static lv_obj_t *s_fx_sparks[FX_SPARKS], *s_fx_dots[FX_DOTS], *s_fx_waves[2][FX_WAVES];
/* Where they sit, in percent of the avatar's size from its top left. */
static const int8_t s_fx_spark_at[FX_SPARKS][2] = {{8, 10}, {20, 4}, {14, 24}, {92, 10}, {80, 4}, {86, 24}};
static const int8_t s_fx_dot_at[FX_DOTS][2] = {{30, 6}, {40, 0}, {50, -3}, {60, 0}, {70, 6}};

static lv_obj_t *fx_effect_image(const lv_image_dsc_t *src, int scale)
{
    lv_obj_t *o = lv_image_create(lv_obj_get_parent(s_canvas));
    lv_image_set_src(o, src);
    lv_image_set_scale(o, scale);
    lv_obj_set_style_image_recolor_opa(o, LV_OPA_COVER, 0);
    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static lv_obj_t *fx_effect_arc(void)
{
    lv_obj_t *a = lv_arc_create(lv_obj_get_parent(s_canvas));
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(a, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_style_arc_width(a, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_INDICATOR);
    lv_arc_set_bg_angles(a, 0, 360);
    return a;
}

/* Twinkle: 0..1, bright for a short part of each cycle. */
static float fx_twinkle(float t, float speed, int i)
{
    float s = sinf(t * speed + i * 2.399f);   /* golden angle: the stars don't blink together */
    return s > 0 ? s * s : 0;
}

static void fx_effects_tick(muse_mode_t mode, float happy, bool visible)
{
    if (!s_canvas) return;
    if (!s_fx_sparks[0]) {
        for (int i = 0; i < FX_SPARKS; i++) s_fx_sparks[i] = fx_effect_image(&s_fx_spark_dsc, 256 + (i % 3) * 96);
        for (int i = 0; i < FX_DOTS; i++) s_fx_dots[i] = fx_effect_image(&s_fx_spark_dsc, 320 + (i % 2) * 128);
        for (int side = 0; side < 2; side++)
            for (int i = 0; i < FX_WAVES; i++) s_fx_waves[side][i] = fx_effect_arc();
    }
    fx_hearts_tick(happy, visible);
    bool is_happy = happy > 0;
    bool sparks = visible && (is_happy || mode == MUSE_MODE_IDLE);
    bool dots = visible && !is_happy && mode == MUSE_MODE_THINKING;
    bool waves = visible && !is_happy && (mode == MUSE_MODE_LISTENING || mode == MUSE_MODE_SPEAKING);
    int32_t size = lv_obj_get_width(s_canvas);
    int32_t x0 = lv_obj_get_x(s_canvas), y0 = lv_obj_get_y(s_canvas);
    float t = (float)esp_timer_get_time() / 1e6f;

    for (int i = 0; i < FX_SPARKS; i++) {
        lv_obj_t *o = s_fx_sparks[i];
        if (!sparks || (is_happy && i % 2)) {   /* happy: half of them, and fainter */
            lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        float b = fx_twinkle(t, is_happy ? 2.2f : 1.6f, i) * (is_happy ? 0.45f : 0.9f);
        lv_obj_set_style_image_recolor(o, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_image_opa(o, (lv_opa_t)(255 * b), 0);
        lv_obj_set_pos(o, x0 + size * s_fx_spark_at[i][0] / 100, y0 + size * s_fx_spark_at[i][1] / 100);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(o);
    }

    for (int i = 0; i < FX_DOTS; i++) {
        lv_obj_t *o = s_fx_dots[i];
        if (!dots) {
            lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        float b = 0.15f + 0.85f * fx_twinkle(t, 3.0f, i);
        lv_obj_set_style_image_recolor(o, lv_color_hex(i % 2 ? 0xffa6e6 : 0xff5fc8), 0);
        lv_obj_set_style_image_opa(o, (lv_opa_t)(255 * b), 0);
        lv_obj_set_pos(o, x0 + size * s_fx_dot_at[i][0] / 100, y0 + size * s_fx_dot_at[i][1] / 100);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(o);
    }

    /* Waves: arcs facing out from a point beside each cheek, growing as they fade. */
    bool speaking = mode == MUSE_MODE_SPEAKING;
    float period = speaking ? 2.4f : 1.1f;
    uint32_t color = speaking ? 0x5fe3a1 : 0x4fa8ff;
    int32_t r0 = size / 20, r1 = size / 6;
    int32_t cy = y0 + size * 45 / 100;
    int32_t cx[2] = {x0 + size * 14 / 100, x0 + size * 86 / 100};
    for (int side = 0; side < 2; side++) {
        for (int i = 0; i < FX_WAVES; i++) {
            lv_obj_t *a = s_fx_waves[side][i];
            if (!waves) {
                lv_obj_add_flag(a, LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            float phase = t / period + i / (float)FX_WAVES;
            phase -= (int)phase;
            int32_t r = r0 + (int32_t)((r1 - r0) * phase);
            lv_obj_set_size(a, 2 * r, 2 * r);
            lv_obj_set_pos(a, cx[side] - r, cy - r);
            if (side == 0) lv_arc_set_angles(a, 145, 215);   /* facing left */
            else lv_arc_set_angles(a, 325, 35);               /* facing right */
            lv_obj_set_style_arc_color(a, lv_color_hex(color), LV_PART_INDICATOR);
            lv_obj_set_style_arc_opa(a, (lv_opa_t)(230 * (1.0f - phase)), LV_PART_INDICATOR);
            lv_obj_remove_flag(a, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(a);
        }
    }
}
#endif
'''


def ui(s):
    # Hearts while happy, after the frame's avatar update.
    s = sub1(s, "static void frame_tick(lv_timer_t *timer)\n{\n", HEARTS_SRC + "\nstatic void frame_tick(lv_timer_t *timer)\n{\n")
    s = sub1(s, "        if (s_was_visible) update_cached_avatar(mode, &pose, !was_visible);\n",
             "        if (s_was_visible) update_cached_avatar(mode, &pose, !was_visible);\n"
             "        fx_effects_tick(mode, pose.happy, s_was_visible);   /* stackchan-muse: hearts, sparkles, waves */\n")
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
