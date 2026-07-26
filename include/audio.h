#ifndef AUDIO_H
#define AUDIO_H

#include "amiga.h"

/*
 * Owns one audio.device channel and a short, generated two-note sample.
 * The sample exists in CHIP RAM only when the hourly chime is enabled.
 */
struct AudioContext
{
    struct MsgPort *au_Port;
    struct IOAudio *au_Request;
    UBYTE *au_Sample;
    BOOL au_Open;
    BOOL au_Running;
};

/* Opens audio.device and prepares the generated chime. */
BOOL AudioInit(struct AudioContext *audio);

/* Stops any pending sound and releases the channel and CHIP RAM. */
void AudioExit(struct AudioContext *audio);

/* Starts the chime asynchronously; ignored if it is already playing. */
void AudioPlayChime(struct AudioContext *audio);

#endif /* AUDIO_H */
