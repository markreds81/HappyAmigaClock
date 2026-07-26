#ifndef AUDIO_H
#define AUDIO_H

#include "amiga.h"

/*
 * Owns one audio.device channel and the enabled generated samples.
 */
struct AudioContext
{
    struct MsgPort *au_Port;
    struct IOAudio *au_Request;
    UBYTE *au_Samples;
    UBYTE *au_ChimeSample;
    UBYTE *au_FlipSample;
    ULONG au_SampleBytes;
    BOOL au_Open;
    BOOL au_Running;
};

/* Opens audio.device and prepares the requested generated samples. */
BOOL AudioInit(struct AudioContext *audio, BOOL chime, BOOL flipSound);

/* Stops any pending sound and releases the channel and CHIP RAM. */
void AudioExit(struct AudioContext *audio);

/* Starts the chime asynchronously; ignored if it is already playing. */
void AudioPlayChime(struct AudioContext *audio);

/* Starts the short mechanical flip sound unless another sound is active. */
void AudioPlayFlip(struct AudioContext *audio);

#endif /* AUDIO_H */
