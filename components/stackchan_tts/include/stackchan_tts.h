// Google Cloud Text-to-Speech for Muse replies (MP3 out, fetched on its own task).
#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// True when an API key is stored (set it over serial: ">tts_key=<key>").
bool stackchan_tts_enabled(void);

// Opens the connection to Google ahead of time (call when a turn starts). Returns at once.
void stackchan_tts_prewarm(void);

// Starts fetching speech for `text` in the background. Replaces any request in
// flight. Returns false when TTS isn't configured or can't start.
bool stackchan_tts_begin(const char *text);

// Copies up to `max` bytes of MP3 into dst without blocking. *ended is set once
// the request is complete and fully read; *failed when it ended with no audio.
size_t stackchan_tts_read(unsigned char *dst, size_t max, bool *ended, bool *failed);

// Serial commands (line without the leading '>'): "tts_key=...", "tts_voice=...",
// "tts_key=" (clear). Returns true if the line was one of them.
bool stackchan_tts_console_command(const char *line);

// ---- stackchan.say (stackchan_say.c) ----

// Longest text spoken, in bytes; longer text is cut at a whole UTF-8 character.
// Below the Muse turn's TEXT_MAX (1024 on the CoreS3), which a message holds.
#define STACKCHAN_SAY_MAX 1023

// Queues text to be spoken outside any chat turn, replacing one not yet played,
// and returns at once. *bytes gets the length kept, *truncated whether it was cut.
bool stackchan_say_post(const char *text, size_t *bytes, bool *truncated);

// For the Muse voice task while idle: whether a say is queued, and taking it
// (malloc'd, the caller frees; NULL if none, or dropped as stale).
bool stackchan_say_waiting(void);
char *stackchan_say_take(void);

// Called after each post, so a resting voice task wakes for it.
void stackchan_say_set_wake(void (*wake)(void));

#ifdef __cplusplus
}
#endif
