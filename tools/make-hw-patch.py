#!/usr/bin/env python3
"""Developer helper: edits an SDK clone (with patches 0001/0002 applied) to add the
stackchan.led / stackchan.servo Muse commands and the stackchan_hw init, writes the
diff to patches/0003-stackchan-hw.patch, then restores the three files.
Run: python3 make-hw-patch.py <sdk>/esp32 <out.patch>"""
import sys, pathlib, subprocess, tempfile, shutil

root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
FILES = ["main/CMakeLists.txt", "main/noise_control.cpp", "main/app.c", "main/sdk_token_console.c",
         "components/muse/CMakeLists.txt", "components/muse/muse_audio.c", "components/muse/muse_state.c",
         "components/muse/muse_ui.c", "components/muse/muse_text.c"]


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
        '        cJSON *led = cJSON_CreateObject();\n'
        '        cJSON_AddItemToObject(led, "r", int_param("Red, 0 to 255."));\n'
        '        cJSON_AddItemToObject(led, "g", int_param("Green, 0 to 255."));\n'
        '        cJSON_AddItemToObject(led, "b", int_param("Blue, 0 to 255."));\n'
        '        cJSON_AddItemToObject(led, "effect", string_param("rainbow, breath, blink, strobe, chase, wave, alternate, solid, or off to stop."));\n'
        '        cJSON_AddItemToObject(led, "preset", string_param("party, ocean, christmas, rainbow, calm_breath, alert_red, warm_white or cool_blue."));\n'
        '        cJSON_AddItemToObject(led, "speed", int_param("Effect speed, 1 (slow) to 10 (fast); default 5."));\n'
        '        cJSON_AddItemToObject(led, "r2", int_param("Second colour for alternate: red."));\n'
        '        cJSON_AddItemToObject(led, "g2", int_param("Second colour for alternate: green."));\n'
        '        cJSON_AddItemToObject(led, "b2", int_param("Second colour for alternate: blue."));\n'
        '        add_command(commands, "stackchan.led",\n'
        '                    "Light the 12 LEDs of the Stack-chan body. Give r, g, b for one steady colour "\n'
        '                    "(0,0,0 turns them off); or an effect, coloured by r, g, b (and r2, g2, b2) at a speed; "\n'
        '                    "or a preset. While it listens, thinks or speaks, those lights come first.",\n'
        '                    nullptr, led);\n'
        '        cJSON *pose = cJSON_CreateObject();\n'
        '        cJSON_AddItemToObject(pose, "x", int_param("Pan angle in degrees, 60 to 240; 150 is centre."));\n'
        '        cJSON_AddItemToObject(pose, "y", int_param("Tilt angle in degrees, 70 to 90; 90 is the resting position, smaller looks up."));\n'
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

static int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static cJSON *stackchan_command(const char *command, cJSON *params) {
    if (strcmp(command, "stackchan.led") == 0) {
        if (!stackchan_led_ready()) return command_error("unavailable", "LEDs are not available");
        bool r_ok, g_ok, b_ok, unused;
        int r = clamp255(json_int(params, "r", 0, &r_ok));
        int g = clamp255(json_int(params, "g", 0, &g_ok));
        int b = clamp255(json_int(params, "b", 0, &b_ok));
        int r2 = clamp255(json_int(params, "r2", 0, &unused));
        int g2 = clamp255(json_int(params, "g2", 0, &unused));
        int b2 = clamp255(json_int(params, "b2", 0, &unused));
        int speed = json_int(params, "speed", 0, &unused);
        cJSON *preset = params ? cJSON_GetObjectItem(params, "preset") : NULL;
        cJSON *effect = params ? cJSON_GetObjectItem(params, "effect") : NULL;
        esp_err_t err;
        if (cJSON_IsString(preset) && preset->valuestring) {
            err = stackchan_led_preset(preset->valuestring);
            if (err == ESP_ERR_NOT_FOUND) return command_error("bad_param", "unknown preset");
        } else if (cJSON_IsString(effect) && effect->valuestring) {
            err = stackchan_led_effect(effect->valuestring, (uint8_t)r, (uint8_t)g, (uint8_t)b,
                                       (uint8_t)r2, (uint8_t)g2, (uint8_t)b2, speed);
            if (err == ESP_ERR_NOT_FOUND) return command_error("bad_param", "unknown effect");
        } else {
            if (!r_ok || !g_ok || !b_ok) return command_error("missing_param", "give r, g and b, an effect or a preset");
            err = stackchan_led_fill((uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
        if (err != ESP_OK) return command_error("io_error", "LED update failed");
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
    s = sub1(s, "#define WIFI_QUICK_MIN_RSSI     (-75)   // near enough to join without a full scan\n",
             "#define WIFI_QUICK_MIN_RSSI     (-65)   // near enough to join without a full scan\n"
             "                                        // stackchan-muse: was -75, which took a weak last-joined\n"
             "                                        // network over a strong one; a full scan picks the strongest\n")
    s = sub1(s, '#include "led_status.h"\n', '#include "led_status.h"\n#include "stackchan_hw.h"\n')
    s = sub1(s, "static cJSON *on_ws_command(\n", HANDLER + "static cJSON *on_ws_command(\n")
    s = sub1(s, '    if (strcmp(command, "device.list_vms") == 0) {\n',
             '    if (strcmp(command, "stackchan.led") == 0 || strcmp(command, "stackchan.servo") == 0) {\n'
             '        return stackchan_command(command, params);\n'
             '    }\n'
             '    if (strcmp(command, "device.list_vms") == 0) {\n')
    return sub1(s, "#if CONFIG_HOMEHUB_VOICE\n    voice_init();\n#endif\n",
                "#if CONFIG_HOMEHUB_VOICE\n    voice_init();\n#endif\n    stackchan_hw_init();\n")


WIFI_CONSOLE = r"""/* stackchan-muse: Wi-Fi from a PC over the serial console, for when the phone can't.
 *   >wifi=SSID,PASSWORD   save it first among the saved networks and restart to join it
 *   >wifi_forget=SSID     forget a saved network
 *   >wifi_list            the saved networks, in the order they are tried (no passwords)
 * The SSID ends at the first comma; the password is the rest of the line. */
static bool wifi_console_command(const char *line) {
    if (!strcmp(line, "wifi_list")) {
        wifi_known_list_t *list = malloc(sizeof(*list));
        if (!list) {
            printf("@wifi_list error no memory\n");
        } else {
            int n = wifi_known_load(list);
            printf("@wifi_list %d:", n);
            for (int i = 0; i < n; i++) printf(" [%d] %s%s", i + 1, list->nets[i].ssid, list->nets[i].hidden ? " (hidden)" : "");
            printf("\n");
            wifi_known_wipe(list);
            free(list);
        }
        fflush(stdout);
        return true;
    }
    if (!strncmp(line, "wifi_forget=", 12)) {
        bool ok = wifi_known_forget(line + 12);
        printf(ok ? "@wifi_forget ok %s\n" : "@wifi_forget error %s\n", line + 12);
        fflush(stdout);
        return true;
    }
    if (strncmp(line, "wifi=", 5) != 0) return false;
    const char *ssid = line + 5;
    const char *comma = strchr(ssid, ',');
    size_t ssid_len = comma ? (size_t)(comma - ssid) : 0;
    const char *pass = comma ? comma + 1 : "";
    if (!comma || ssid_len == 0 || ssid_len > WIFI_KNOWN_SSID_MAX || strlen(pass) > WIFI_KNOWN_PASS_MAX) {
        printf("@wifi error usage: >wifi=SSID,PASSWORD\n");
        fflush(stdout);
        return true;
    }
    char name[WIFI_KNOWN_SSID_MAX + 1];
    memcpy(name, ssid, ssid_len);
    name[ssid_len] = '\0';
    if (!wifi_known_remember(name, pass, -1)) {
        printf("@wifi error could not save\n");
        fflush(stdout);
        return true;
    }
    ESP_LOGI(TAG, "saved wifi network %s first; restarting to join it", name);
    printf("@wifi ok %s (restarting)\n", name);
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(300));
    esp_restart();
    return true;
}

"""


def console(s):
    s = sub1(s, '#include "stackchan_tts.h"\n', '#include "stackchan_tts.h"\n#include "stackchan_hw.h"\n#include "wifi_known.h"\n#include <stdlib.h>\n')
    # an SSID (32) and a password (64) do not fit in 96
    s = sub1(s, "#define COMMAND_LINE_MAX 96\n", "#define COMMAND_LINE_MAX 128\n")
    s = sub1(s, "bool sdk_token_console_command(const char *line) {\n", WIFI_CONSOLE + "bool sdk_token_console_command(const char *line) {\n")
    return sub1(s, "    if (stackchan_tts_console_command(line)) return true;\n",
                "    if (stackchan_tts_console_command(line)) return true;\n    if (stackchan_hw_console_command(line)) return true;\n"
                "    if (wifi_console_command(line)) return true;\n")


def muse_cmake(s):
    return sub1(s, "list(APPEND muse_priv_requires camera stackchan_tts)",
                "list(APPEND muse_priv_requires camera stackchan_tts stackchan_hw stackchan_fonts)")


def muse_text(s):
    # A text with Cyrillic is drawn and paged like one with CJK, in the caption font, whose
    # Unifont fallback has it (Montserrat and unscii-8 don't). Line breaking is per character
    # elsewhere (muse_text_cjk()) and still treats Cyrillic as words.
    return sub1(s, "        size_t len;\n        decode((const unsigned char *)s, &len);\n        s += len;\n    }\n    return false;\n}\n",
                "        size_t len;\n        int32_t cp = decode((const unsigned char *)s, &len);\n"
                "        if (cp >= 0x400 && cp <= 0x52F) {   /* stackchan-muse: Cyrillic (Mongolian, #13) */\n"
                "            return true;\n"
                "        }\n"
                "        s += len;\n    }\n    return false;\n}\n")


def muse_ui(s):
    # Captions: after the CJK font, Unifont's 8x16 Cyrillic (Mongolian, #13), not boxes.
    s = sub1(s, "/* unscii-16 for captions and replies; with CONFIG_MUSE_CJK_FONT, a copy that\n",
             "LV_FONT_DECLARE(stackchan_font_cyr_16)   /* stackchan-muse: Cyrillic captions */\n\n"
             "/* unscii-16 for captions and replies; with CONFIG_MUSE_CJK_FONT, a copy that\n")
    return sub1(s, "        font = lv_font_unscii_16;\n        font.fallback = &CJK_FONT;\n",
                "        font = lv_font_unscii_16;\n"
                "        /* stackchan-muse: a copy of the CJK font, falling back to Cyrillic. */\n"
                "        static lv_font_t cjk_cyr;\n"
                "        cjk_cyr = CJK_FONT;\n"
                "        cjk_cyr.fallback = &stackchan_font_cyr_16;\n"
                "        font.fallback = &cjk_cyr;\n")


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


def muse_state(s):
    s = sub1(s, '#include "muse_text.h"\n', '#include "muse_text.h"\n#include "stackchan_hw.h"\n')
    s = sub1(s, "void muse_state_init(void)\n{\n",
             "void muse_state_init(void)\n{\n"
             "    stackchan_hw_set_pet_callback(muse_state_make_happy);   /* head pats make the face happy */\n")
    return sub1(s, "    s_mode_since_us = esp_timer_get_time();\n    s_mode = mode;\n}",
                "    s_mode_since_us = esp_timer_get_time();\n    s_mode = mode;\n"
                "    stackchan_hw_ui_mode((int)mode);   /* LEDs for listening / thinking */\n}")


EDITS = {"main/CMakeLists.txt": cmake, "main/noise_control.cpp": noise, "main/app.c": app, "main/sdk_token_console.c": console,
         "components/muse/CMakeLists.txt": muse_cmake, "components/muse/muse_audio.c": muse_audio,
         "components/muse/muse_state.c": muse_state, "components/muse/muse_ui.c": muse_ui,
         "components/muse/muse_text.c": muse_text}

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
