#!/usr/bin/env python3
"""Developer helper: on an SDK clone with patches 0001-0004 applied, adds the optional
`voice` argument to stackchan.say (#13), writes the diff to patches/0005-say-voice.patch,
then restores the files (apply.sh applies the patch for real).
Run: python3 make-say-voice-patch.py <sdk>/esp32 <out.patch>"""
import sys, pathlib, subprocess, tempfile, shutil

root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
FILES = ["main/noise_control.cpp", "main/app.c"]


def sub1(s, old, new):
    assert s.count(old) == 1, old
    return s.replace(old, new)


def noise(s):
    s = sub1(s, '    add_command(commands, "stackchan.say",\n',
             '    cJSON *say_optional = cJSON_CreateObject();\n'
             '    cJSON_AddItemToObject(say_optional, "voice",\n'
             '                          string_param("Google Cloud TTS voice name to speak in, e.g. "\n'
             '                                       "en-US-Neural2-C; its language comes from the name. "\n'
             '                                       "Without it, each sentence is read in the voice for its "\n'
             '                                       "language: Japanese, English or Mongolian."));\n'
             '    add_command(commands, "stackchan.say",\n')
    return sub1(s, "                say_required, nullptr);\n", "                say_required, say_optional);\n")


def app(s):
    s = sub1(s, "        if (!stackchan_say_post(text->valuestring, &bytes, &truncated)) {\n",
             '        cJSON *voice = cJSON_GetObjectItem(params, "voice");\n'
             '        const char *v = cJSON_IsString(voice) && voice->valuestring && voice->valuestring[0]\n'
             '                            ? voice->valuestring : NULL;\n'
             '        if (v && strlen(v) >= 40) return command_error("bad_param", "voice name too long");\n'
             "        if (!stackchan_say_post_voice(text->valuestring, v, &bytes, &truncated)) {\n")
    return sub1(s, '        cJSON_AddBoolToObject(payload, "truncated", truncated);\n',
                '        cJSON_AddBoolToObject(payload, "truncated", truncated);\n'
                '        if (v) cJSON_AddStringToObject(payload, "voice", v);\n')


EDITS = {"main/noise_control.cpp": noise, "main/app.c": app}

tmp = pathlib.Path(tempfile.mkdtemp())
try:
    for rel in FILES:
        dst = tmp / "a" / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(root / rel, dst)
    for rel, fn in EDITS.items():
        p = root / rel
        p.write_text(fn(p.read_text()))
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
