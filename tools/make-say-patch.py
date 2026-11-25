#!/usr/bin/env python3
"""Developer helper: edits a muse-gadget-sdk clone that already has patches 0001-0003
applied (and committed, or the diff includes them) to add the
stackchan.say command, then prints `git diff` of the touched files (save it as
patches/0004-stackchan-say.patch).
Run: python3 make-say-patch.py <sdk>/esp32"""
import sys, subprocess, pathlib

root = pathlib.Path(sys.argv[1])

def edit(rel, fn):
    p = root / rel
    s = p.read_text()
    n = fn(s)
    assert n != s, rel
    p.write_text(n)

def sub1(s, old, new):
    assert s.count(old) == 1, old
    return s.replace(old, new)

# ---- Muse voice session: a turn of its own for a say, played like a reply ----

edit("components/muse/muse_chat.h", lambda s: sub1(s,
    "void muse_hatch_text_turn(char *text);\nvoid muse_hatch_text_cancel(void);\n",
    """void muse_hatch_text_turn(char *text);
void muse_hatch_text_cancel(void);

/*
 * Stack-chan: for the voice task, while idle. True if a stackchan.say was
 * waiting (stackchan_say_take); it then starts a turn of its own that speaks
 * that text, played like any reply (muse_hatch_turn_event and the rest),
 * without posting anything to the chat.
 */
bool muse_hatch_say_begin(void);
"""))

def sub_any(s, pairs):
    """The first (old, new) whose old is in s, exactly once: anchors that differ between SDK versions."""
    for old, new in pairs:
        if s.count(old) == 1:
            return s.replace(old, new)
    raise AssertionError("none of: " + " | ".join(o for o, _ in pairs))

def chat(s):
    s = sub_any(s, [
        ("CMD_TEXT, CMD_TEXT_CANCEL, CMD_WAKE };", "CMD_TEXT, CMD_TEXT_CANCEL, CMD_WAKE, CMD_SAY };"),
        # newer SDKs: more commands after CMD_WAKE
        ("CMD_WAKE, CMD_INCOMING, CMD_OPTION,\n", "CMD_WAKE, CMD_INCOMING, CMD_OPTION, CMD_SAY,\n"),
    ])
    s = sub1(s, "    bool agent_busy;\n",
             "    bool agent_busy;\n    bool local;              /* stackchan.say: nothing from the chat belongs to it */\n")
    # A say's turn takes no events: the agent that sent it is likely still busy,
    # and its status would hold the turn open after the speech.
    s = sub_any(s, [
        ("    if (s_turn.phase != P_WAIT_REPLY) {\n        return;\n    }\n"
         "    const char *event = cJSON_GetStringValue(cJSON_GetObjectItem(line, \"event\")) ?: \"\";\n",
         "    if (s_turn.phase != P_WAIT_REPLY || s_turn.local) {\n        return;\n    }\n"
         "    const char *event = cJSON_GetStringValue(cJSON_GetObjectItem(line, \"event\")) ?: \"\";\n"),
        # newer SDKs read the event first (for options) and check the turn after
        ("    if (s_turn.phase != P_WAIT_REPLY) {\n        return;\n    }\n    bool was = s_turn.agent_busy;\n",
         "    if (s_turn.phase != P_WAIT_REPLY || s_turn.local) {\n        return;\n    }\n    bool was = s_turn.agent_busy;\n"),
    ])
    s = sub1(s, "    char *text;              /* CMD_TEXT: malloc'd",
             "    char *text;              /* CMD_TEXT, CMD_SAY: malloc'd")
    # say_begin: after on_chat_ack, before the speech section
    s = sub1(s, "/* ---- Turn: speech ---- */\n",
"""static_assert(STACKCHAN_SAY_MAX < TEXT_MAX, "a say must fit one message's text");

/*
 * stackchan.say: a turn that posts nothing. Its one message is the text,
 * local only (not in the chat), which start_tts() speaks through Google TTS
 * and check_turn() ends once it's said.
 */
static void say_begin(uint32_t gen, const char *text)
{
    if (!turn_start(gen, false)) {   /* ends a typed turn in progress, as a press does */
        return;   /* turn_fail told the voice task */
    }
    int64_t t = now_us();
    s_turn.phase = P_WAIT_REPLY;
    s_turn.chat_posted = s_turn.acked = true;
    s_turn.chat_us = t;
    s_turn.last_event_us = s_turn.last_content_us = t - SETTLE_US;   /* nothing more is coming */
    s_turn.local = true;   /* on_event leaves it alone */
    msg_t &m = s_turn.msgs[s_turn.nmsgs++];
    m = msg_t{};
    strlcpy(m.id, "local-say", sizeof(m.id));
    append_text(m, text);
    message_done(0, nullptr);
    ESP_LOGI(TAG, "stackchan.say: %u bytes", (unsigned)m.len);
}

/* ---- Turn: speech ---- */
""")
    s = sub1(s, "    case CMD_WAKE:   /* only ends hatch_task's resting wait */\n        break;\n",
"""    case CMD_WAKE:   /* only ends hatch_task's resting wait */
        break;
    case CMD_SAY:
        if (cmd.gen == s_gen.load()) {
            say_begin(cmd.gen, cmd.text);
        }
        free(cmd.text);
        break;
""")
    s = sub1(s, "extern \"C\" void muse_hatch_text_cancel(void)\n{\n    post(CMD_TEXT_CANCEL, 0);\n}\n",
"""extern "C" void muse_hatch_text_cancel(void)
{
    post(CMD_TEXT_CANCEL, 0);
}

extern "C" bool muse_hatch_say_begin(void)
{
    if (!s_cmds) {
        return false;
    }
    char *text = stackchan_say_take();
    if (!text) {
        return false;
    }
    /* Numbered like a press's turn, so a press that follows replaces it. */
    uint32_t gen = ++s_gen;
    drain_out();
    cmd_t cmd{ CMD_SAY, gen, text };
    if (xQueueSend(s_cmds, &cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGW(TAG, "stackchan.say dropped: session busy");
        free(text);
        return false;
    }
    return true;
}
""")
    return s
edit("components/muse/muse_chat_session.cpp", chat)

def voice(s):
    s = sub1(s, '#include "muse_wifi.h"\n', '#include "muse_wifi.h"\n#include "stackchan_tts.h"\n')
    # Stay awake for a queued say. (Newer SDKs also stay awake for the wake word.)
    s = sub_any(s, [
        ("            bool rest = asleep && battery && !s_chirp && !s_mp3test && !s_loopback;\n",
         "            bool rest = asleep && battery && !s_chirp && !s_mp3test && !s_loopback\n"
         "                        && !stackchan_say_waiting();\n"),
        ("            bool rest = asleep && battery && !s_chirp && !s_mp3test && !s_loopback && !muse_wakeword_enabled();\n",
         "            bool rest = asleep && battery && !s_chirp && !s_mp3test && !s_loopback && !muse_wakeword_enabled()\n"
         "                        && !stackchan_say_waiting();\n"),
    ])
    # hatch_reply() takes the caption to show until the reply in newer SDKs.
    reply_call = "hatch_reply(&delivered, \"\")" if "static bool hatch_reply(bool *delivered, const char *opening)" in s \
        else "hatch_reply(&delivered)"
    # Play it when idle, a press first.
    s = sub1(s, "            /* The 20 ms read paces this loop. */\n            idle_capture();\n",
"""#if CONFIG_MUSE_HATCH
            muse_input_event_t say_next;
            if (stackchan_say_waiting() && xQueuePeek(s_queue, &say_next, 0) != pdTRUE && muse_hatch_say_begin()) {
                /* stackchan.say: light the screen and speak it like a reply. */
                bool delivered;
                muse_state_set_asleep(false);
                muse_wifi_power(MUSE_WIFI_FULL);
                pending_down = REPLY_CALL;
                pre_reset();
                if (!pending_down && muse_state_mode(NULL) != MUSE_MODE_IDLE) {
                    muse_state_make_happy();
                    go_idle("");
                }
                continue;
            }
#endif
            /* The 20 ms read paces this loop. */
            idle_capture();
""".replace("REPLY_CALL", reply_call))
    # A post wakes a resting loop.
    s = sub1(s, "esp_err_t muse_voice_start(QueueHandle_t queue)\n{\n    s_queue = queue;\n",
             "esp_err_t muse_voice_start(QueueHandle_t queue)\n{\n    s_queue = queue;\n"
             "    stackchan_say_set_wake(muse_state_nudge);\n")
    return s
edit("components/muse/muse_voice.c", voice)

# ---- Home Link: advertise and handle the command ----

edit("main/noise_control.cpp", lambda s: sub1(s,
    "#if CONFIG_HOMEHUB_VOICE\n    cJSON *volume_optional = cJSON_CreateObject();\n",
"""#if CONFIG_MUSE_HATCH
    cJSON *say_required = cJSON_CreateObject();
    cJSON_AddItemToObject(say_required, "text",
                          string_param("What to say, up to 1023 bytes (longer is cut)."));
    add_command(commands, "stackchan.say",
                "Speak text aloud on Stack-chan now (Google TTS), with the "
                "speaking face. Not part of the chat. Replies once queued, "
                "before speaking; a newer say replaces one not yet started.",
                say_required, nullptr);
#endif

#if CONFIG_HOMEHUB_VOICE
    cJSON *volume_optional = cJSON_CreateObject();
"""))

def app(s):
    s = sub1(s, "#if CONFIG_MUSE_ENABLED\n#include \"muse_glue.h\"\n",
             "#include \"stackchan_tts.h\"\n#if CONFIG_MUSE_ENABLED\n#include \"muse_glue.h\"\n")
    s = sub1(s, "#if CONFIG_HOMEHUB_VOICE\n    if (strcmp(command, \"voice.configure\") == 0) {\n",
"""#if CONFIG_MUSE_HATCH
    if (strcmp(command, "stackchan.say") == 0) {
        cJSON *text = cJSON_GetObjectItem(params, "text");
        if (!cJSON_IsString(text) || !text->valuestring || !text->valuestring[0]) {
            return command_error("missing_param", "text is required");
        }
        if (!stackchan_tts_enabled()) {
            return command_error("tts_not_configured", "no Google TTS key on the device (>tts_key=)");
        }
        size_t bytes = 0;
        bool truncated = false;
        if (!stackchan_say_post(text->valuestring, &bytes, &truncated)) {
            return command_error("out_of_memory", "failed to queue the text");
        }
        /* Replies now: the speech follows on the voice task. */
        cJSON *result = cJSON_CreateObject();
        cJSON *payload = cJSON_CreateObject();
        cJSON_AddBoolToObject(result, "ok", true);
        cJSON_AddBoolToObject(payload, "queued", true);
        cJSON_AddNumberToObject(payload, "bytes", (double)bytes);
        cJSON_AddBoolToObject(payload, "truncated", truncated);
        cJSON_AddItemToObject(result, "payload", payload);
        return result;
    }
#endif
#if CONFIG_HOMEHUB_VOICE
    if (strcmp(command, "voice.configure") == 0) {
""")
    return s
edit("main/app.c", app)

print(subprocess.run(["git", "diff", "--", "components/muse/muse_chat.h",
                      "components/muse/muse_chat_session.cpp", "components/muse/muse_voice.c",
                      "main/noise_control.cpp", "main/app.c"],
                     cwd=root, capture_output=True, text=True).stdout, end="")
