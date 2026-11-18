#!/usr/bin/env python3
"""Developer helper: edits a clean muse-gadget-sdk clone to add Google TTS, then
prints `git diff` of the touched files (save it as patches/0002-google-tts.patch).
Run in the SDK root: python3 make-tts-patch.py <sdk>/esp32"""
import sys, re, subprocess, pathlib

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

# muse component -> stackchan_tts
edit("components/muse/CMakeLists.txt",
     lambda s: sub1(s, "list(APPEND muse_priv_requires camera)",
                    "list(APPEND muse_priv_requires camera stackchan_tts)"))
# main -> stackchan_tts (serial key command)
edit("main/CMakeLists.txt",
     lambda s: sub1(s, "set(GADGET_REQUIRES\n    nvs_flash\n",
                    "set(GADGET_REQUIRES\n    stackchan_tts\n    nvs_flash\n"))
edit("main/sdk_token_console.c", lambda s: sub1(
    sub1(s, '#include "identity.h"\n', '#include "identity.h"\n#include "stackchan_tts.h"\n'),
    "bool sdk_token_console_command(const char *line) {\n",
    "bool sdk_token_console_command(const char *line) {\n    if (stackchan_tts_console_command(line)) return true;\n"))

def chat(s):
    s = sub1(s, '#include "muse_settings.h"\n', '#include "muse_settings.h"\n#include "stackchan_tts.h"\n')
    s = sub1(s, "    bool mp3_ended;\n    mp3dec_t dec;",
             "    bool mp3_ended;\n    bool sc_tts;             /* MP3 is arriving from stackchan_tts */\n    mp3dec_t dec;")
    # start_tts: try Google TTS first
    s = sub1(s, "        m.pcm_start = s_turn.pcm_out;\n        /* Paced to the text kept:",
"""        s_turn.sc_tts = false;
        if (s_turn.texts && s_turn.texts[i * TEXT_MAX] && s_turn.mp3 && stackchan_tts_begin(s_turn.texts + i * TEXT_MAX)) {
            m.pcm_start = s_turn.pcm_out;
            m.pcm_frames = 0;
            m.tts = TTS_ACTIVE;
            s_turn.tts_msg = i;
            s_turn.silent = false;
            s_turn.mp3_len = 0;
            s_turn.mp3_ended = false;
            s_turn.kbps = 0;
            s_turn.down_rate = 0;
            mp3dec_init(&s_turn.dec);
            s_turn.sc_tts = true;
            s_turn.sc_t0 = now_us();
            ESP_LOGI(TAG, "speaking message %s via Google TTS (%u chars)", m.id, (unsigned)m.len);
            show_reply_start(m);
            return;
        }
        m.pcm_start = s_turn.pcm_out;
        /* Paced to the text kept:""")
    # decode: pull MP3 from stackchan_tts
    s = sub1(s, "    if (s_turn.silent) {\n        pace_silently();\n        return;\n    }\n    /*\n     * minimp3",
"""    if (s_turn.sc_tts && !s_turn.silent) {
        bool ended = false, failed = false;
        while (s_turn.mp3_len < MP3_BUF) {
            size_t room = MP3_BUF - s_turn.mp3_len;
            size_t n = stackchan_tts_read(s_turn.mp3 + s_turn.mp3_len, room < 4096 ? room : 4096, &ended, &failed);
            if (!n) {
                break;
            }
            mark(M_MP3);
            s_turn.mp3_len += n;
        }
        if (!ended && !s_turn.mp3_len && !s_turn.down_rate && now_us() - s_turn.sc_t0 > 12000000) {
            ended = true;   /* no audio in 12 s: give up rather than leave the turn thinking */
            failed = true;
        }
        if (ended) {
            s_turn.sc_tts = false;
            if (failed && !s_turn.mp3_len && !s_turn.down_rate) {
                /* TTS failed before any audio: show the text at reading pace instead. */
                msg_t &fm = s_turn.msgs[s_turn.tts_msg];
                size_t shown = s_turn.texts ? CAPTION_LEN(s_turn.texts + s_turn.tts_msg * TEXT_MAX) : fm.len;
                fm.pcm_start = s_turn.pcm_out;
                fm.pcm_frames = (uint32_t)(shown * MIC_RATE / TEXT_CHARS_PER_S);
                s_turn.silent = true;
                ESP_LOGW(TAG, "Google TTS failed, showing text only");
                pace_silently();
                return;
            }
            s_turn.mp3_ended = true;
        }
    }
    if (s_turn.silent) {
        pace_silently();
        return;
    }
    /*
     * minimp3""")
    return s
edit("components/muse/muse_chat_session.cpp", chat)

print(subprocess.run(["git", "diff", "--", "components/muse/CMakeLists.txt", "main/CMakeLists.txt",
                      "main/sdk_token_console.c", "components/muse/muse_chat_session.cpp"],
                     cwd=root, capture_output=True, text=True).stdout)
