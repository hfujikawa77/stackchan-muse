#!/usr/bin/env python3
"""Developer helper: edits an SDK clone (with patches 0001/0002 applied) to add the
stackchan.led / stackchan.servo Muse commands and the stackchan_hw init, writes the
diff to patches/0003-stackchan-hw.patch, then restores the three files.
Run: python3 make-hw-patch.py <sdk>/esp32 <out.patch>"""
import sys, pathlib, subprocess, tempfile, shutil

root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
FILES = ["main/CMakeLists.txt", "main/noise_control.cpp", "main/app.c", "main/sdk_token_console.c",
         "components/muse/CMakeLists.txt", "components/muse/muse_audio.c"]


def sub1(s, old, new):
    assert s.count(old) == 1, old
    return s.replace(old, new)


def cmake(s):
    return sub1(s, "    stackchan_tts\n    nvs_flash\n", "    stackchan_tts\n    stackchan_hw\n    nvs_flash\n")


def noise(s):
    s = sub1(s, "static void add_command(cJSON *commands, const char *name,",
             'static cJSON *int_param(const char *description) {\n'
             '    cJSON *param = cJSON_CreateObject();\n'
             '    cJSON_AddStringToObject(param, "type", "integer");\n'
             '    cJSON_AddStringToObject(param, "description", description);\n'
             '    return param;\n'
             '}\n\n'
             'static void add_command(cJSON *commands, const char *name,')
    cmds = (
        '    {\n'
        '        cJSON *rgb = cJSON_CreateObject();\n'
        '        cJSON_AddItemToObject(rgb, "r", int_param("Red, 0 to 255."));\n'
        '        cJSON_AddItemToObject(rgb, "g", int_param("Green, 0 to 255."));\n'
        '        cJSON_AddItemToObject(rgb, "b", int_param("Blue, 0 to 255."));\n'
        '        add_command(commands, "stackchan.led",\n'
        '                    "Light all 12 LEDs of the Stack-chan body in one colour. 0,0,0 turns them off.",\n'
        '                    rgb, nullptr);\n'
        '        cJSON *pose = cJSON_CreateObject();\n'
        '        cJSON_AddItemToObject(pose, "x", int_param("Pan angle in degrees, 60 to 240; 150 is centre."));\n'
        '        cJSON_AddItemToObject(pose, "y", int_param("Tilt angle in degrees, 70 to 90; 90 is level."));\n'
        '        cJSON_AddItemToObject(pose, "ms", int_param("Time to get there in milliseconds, at least 150; default 600."));\n'
        '        add_command(commands, "stackchan.servo",\n'
        '                    "Move the Stack-chan head. With no x and y it returns to centre. "\n'
        '                    "Angles outside the safe range are clamped. Returns the angles it moved to.",\n'
        '                    nullptr, pose);\n'
        '    }\n')
    return sub1(s, '#if CONFIG_HOMEHUB_TUNNEL\n    add_command(commands, "device.discover",',
                cmds + '#if CONFIG_HOMEHUB_TUNNEL\n    add_command(commands, "device.discover",')


HANDLER = r'''/* stackchan.led / stackchan.servo: quick, so they run right here on the session task. */
static int json_int(cJSON *params, const char *key, int fallback, bool *present) {
    cJSON *item = params ? cJSON_GetObjectItem(params, key) : NULL;
    if (present) *present = cJSON_IsNumber(item);
    return cJSON_IsNumber(item) ? item->valueint : fallback;
}

static cJSON *stackchan_command(const char *command, cJSON *params) {
    if (strcmp(command, "stackchan.led") == 0) {
        bool r_ok, g_ok, b_ok;
        int r = json_int(params, "r", 0, &r_ok);
        int g = json_int(params, "g", 0, &g_ok);
        int b = json_int(params, "b", 0, &b_ok);
        if (!r_ok || !g_ok || !b_ok) return command_error("missing_param", "r, g and b are required");
        if (!stackchan_led_ready()) return command_error("unavailable", "LEDs are not available");
        r = r < 0 ? 0 : (r > 255 ? 255 : r);
        g = g < 0 ? 0 : (g > 255 ? 255 : g);
        b = b < 0 ? 0 : (b > 255 ? 255 : b);
        if (stackchan_led_fill((uint8_t)r, (uint8_t)g, (uint8_t)b) != ESP_OK) {
            return command_error("io_error", "LED update failed");
        }
        cJSON *result = cJSON_CreateObject();
        cJSON_AddBoolToObject(result, "ok", true);
        return result;
    }
    /* stackchan.servo */
    if (!stackchan_servo_ready()) return command_error("unavailable", "servos are not available");
    bool x_ok, y_ok, ms_ok;
    int x = json_int(params, "x", STACKCHAN_SERVO_X_CENTER, &x_ok);
    int y = json_int(params, "y", STACKCHAN_SERVO_Y_CENTER, &y_ok);
    int ms = json_int(params, "ms", 600, &ms_ok);
    if (ms < 0) ms = 0;
    int cur_x, cur_y;
    stackchan_servo_position(&cur_x, &cur_y);
    if (x_ok != y_ok) {   /* only one axis given: keep the other where it is */
        if (!x_ok) x = cur_x;
        if (!y_ok) y = cur_y;
    }
    if (stackchan_servo_move(x, y, (uint32_t)ms) != ESP_OK) {
        return command_error("io_error", "servo move failed");
    }
    stackchan_servo_position(&x, &y);
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "x", x);
    cJSON_AddNumberToObject(payload, "y", y);
    cJSON *result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "ok", true);
    cJSON_AddItemToObject(result, "payload", payload);
    return result;
}

'''


def app(s):
    s = sub1(s, '#include "led_status.h"\n', '#include "led_status.h"\n#include "stackchan_hw.h"\n')
    s = sub1(s, "static cJSON *on_ws_command(\n", HANDLER + "static cJSON *on_ws_command(\n")
    s = sub1(s, '    if (strcmp(command, "device.list_vms") == 0) {\n',
             '    if (strcmp(command, "stackchan.led") == 0 || strcmp(command, "stackchan.servo") == 0) {\n'
             '        return stackchan_command(command, params);\n'
             '    }\n'
             '    if (strcmp(command, "device.list_vms") == 0) {\n')
    return sub1(s, "#if CONFIG_HOMEHUB_VOICE\n    voice_init();\n#endif\n",
                "#if CONFIG_HOMEHUB_VOICE\n    voice_init();\n#endif\n    stackchan_hw_init();\n")


def console(s):
    s = sub1(s, '#include "stackchan_tts.h"\n', '#include "stackchan_tts.h"\n#include "stackchan_hw.h"\n')
    return sub1(s, "    if (stackchan_tts_console_command(line)) return true;\n",
                "    if (stackchan_tts_console_command(line)) return true;\n    if (stackchan_hw_console_command(line)) return true;\n")


def muse_cmake(s):
    return sub1(s, "list(APPEND muse_priv_requires camera stackchan_tts)",
                "list(APPEND muse_priv_requires camera stackchan_tts stackchan_hw)")


def muse_audio(s):
    s = sub1(s, '#include "muse_settings.h"\n', '#include "muse_settings.h"\n#include "stackchan_hw.h"\n')
    old = ("    while (frames) {\n"
           "        size_t n = frames > MUSE_AUDIO_CHUNK ? MUSE_AUDIO_CHUNK : frames;\n"
           "        for (size_t i = 0; i < n; i++) {\n"
           "            s_out_stereo[2 * i] = mute")
    new = ("    while (frames) {\n"
           "        size_t n = frames > MUSE_AUDIO_CHUNK ? MUSE_AUDIO_CHUNK : frames;\n"
           "        stackchan_hw_audio_level(muse_audio_level(mono, n));   /* speech animation */\n"
           "        for (size_t i = 0; i < n; i++) {\n"
           "            s_out_stereo[2 * i] = mute")
    return sub1(s, old, new)


EDITS = {"main/CMakeLists.txt": cmake, "main/noise_control.cpp": noise, "main/app.c": app, "main/sdk_token_console.c": console,
         "components/muse/CMakeLists.txt": muse_cmake, "components/muse/muse_audio.c": muse_audio}

tmp = pathlib.Path(tempfile.mkdtemp())
try:
    for rel in FILES:
        dst = tmp / "a" / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(root / rel, dst)
    for rel, fn in EDITS.items():
        p = root / rel
        p.write_text(fn(p.read_text()))
    # Diff relative to the SDK repo root (esp32/ prefix), the way git apply expects.
    chunks = []
    for rel in FILES:
        r = subprocess.run(["diff", "-u", str(tmp / "a" / rel), str(root / rel)], capture_output=True, text=True)
        body = r.stdout.split("\n", 2)[2] if r.stdout else ""
        chunks.append(f"diff --git a/esp32/{rel} b/esp32/{rel}\n--- a/esp32/{rel}\n+++ b/esp32/{rel}\n{body}")
    out.write_text("".join(chunks))
finally:
    for rel in FILES:   # restore: apply.sh applies the patch for real
        shutil.copy(tmp / "a" / rel, root / rel)
    shutil.rmtree(tmp)
print("wrote", out)
