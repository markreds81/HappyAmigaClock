#include "audio.h"

#define SAMPLE_RATE 6000UL
#define SAMPLE_PERIOD 591
#define SAMPLE_VOLUME 42
#define FIRST_NOTE_HZ 784UL
#define SECOND_NOTE_HZ 988UL
#define FIRST_NOTE_SAMPLES 720
#define PAUSE_SAMPLES 120
#define SECOND_NOTE_SAMPLES 960
#define CHIME_SAMPLES (FIRST_NOTE_SAMPLES + PAUSE_SAMPLES + SECOND_NOTE_SAMPLES)
#define FADE_SAMPLES 60

static BYTE triangleSample(ULONG phase)
{
    UWORD position = (UWORD)((phase >> 8) & 0xff);

    if (position < 128) return (BYTE)((WORD)position * 2 - 127);
    return (BYTE)(383 - (WORD)position * 2);
}

static void generateNote(UBYTE *dst, UWORD length, ULONG frequency)
{
    ULONG phase = 0;
    ULONG step = (frequency * 65536UL) / SAMPLE_RATE;
    UWORD i;

    for (i = 0; i < length; i++)
    {
        WORD envelope = SAMPLE_VOLUME;
        WORD sample;

        if (i < FADE_SAMPLES)
            envelope = (WORD)(((ULONG)i * SAMPLE_VOLUME) / FADE_SAMPLES);
        else if (i >= length - FADE_SAMPLES)
            envelope = (WORD)(((ULONG)(length - 1 - i) * SAMPLE_VOLUME) /
                              FADE_SAMPLES);

        sample = ((WORD)triangleSample(phase) * envelope) / 127;
        dst[i] = (UBYTE)(BYTE)sample;
        phase += step;
    }
}

static void generateChime(UBYTE *sample)
{
    UWORD i;

    generateNote(sample, FIRST_NOTE_SAMPLES, FIRST_NOTE_HZ);

    for (i = 0; i < PAUSE_SAMPLES; i++)
        sample[FIRST_NOTE_SAMPLES + i] = 0;

    generateNote(sample + FIRST_NOTE_SAMPLES + PAUSE_SAMPLES,
                 SECOND_NOTE_SAMPLES, SECOND_NOTE_HZ);
}

BOOL AudioInit(struct AudioContext *audio)
{
    static UBYTE channelPreference[4] = {1, 2, 4, 8};

    audio->au_Port = NULL;
    audio->au_Request = NULL;
    audio->au_Sample = NULL;
    audio->au_Open = FALSE;
    audio->au_Running = FALSE;

    audio->au_Port = CreatePort(NULL, 0);
    if (!audio->au_Port) return FALSE;

    audio->au_Request =
        (struct IOAudio *)CreateExtIO(audio->au_Port, sizeof(struct IOAudio));
    if (!audio->au_Request)
    {
        AudioExit(audio);
        return FALSE;
    }

    audio->au_Sample = (UBYTE *)AllocMem(CHIME_SAMPLES, MEMF_CHIP);
    if (!audio->au_Sample)
    {
        AudioExit(audio);
        return FALSE;
    }

    generateChime(audio->au_Sample);

    audio->au_Request->ioa_Data = channelPreference;
    audio->au_Request->ioa_Length = sizeof(channelPreference);
    audio->au_Request->ioa_Request.io_Message.mn_Node.ln_Pri = 0;
    audio->au_Request->ioa_Request.io_Flags = ADIOF_NOWAIT;

    if (OpenDevice((STRPTR)AUDIONAME, 0, (struct IORequest *)audio->au_Request,
                   0) != 0)
    {
        AudioExit(audio);
        return FALSE;
    }

    audio->au_Open = TRUE;
    return TRUE;
}

void AudioExit(struct AudioContext *audio)
{
    if (audio->au_Request)
    {
        if (audio->au_Running)
        {
            AbortIO((struct IORequest *)audio->au_Request);
            WaitIO((struct IORequest *)audio->au_Request);
            audio->au_Running = FALSE;
        }

        if (audio->au_Open)
        {
            CloseDevice((struct IORequest *)audio->au_Request);
            audio->au_Open = FALSE;
        }

        DeleteExtIO((struct IORequest *)audio->au_Request);
        audio->au_Request = NULL;
    }

    if (audio->au_Sample)
    {
        FreeMem(audio->au_Sample, CHIME_SAMPLES);
        audio->au_Sample = NULL;
    }

    if (audio->au_Port)
    {
        DeletePort(audio->au_Port);
        audio->au_Port = NULL;
    }
}

void AudioPlayChime(struct AudioContext *audio)
{
    if (!audio->au_Request) return;

    if (audio->au_Running)
    {
        if (!CheckIO((struct IORequest *)audio->au_Request)) return;

        WaitIO((struct IORequest *)audio->au_Request);
        audio->au_Running = FALSE;
    }

    audio->au_Request->ioa_Request.io_Command = CMD_WRITE;
    audio->au_Request->ioa_Request.io_Flags = ADIOF_PERVOL;
    audio->au_Request->ioa_Data = audio->au_Sample;
    audio->au_Request->ioa_Length = CHIME_SAMPLES;
    audio->au_Request->ioa_Period = SAMPLE_PERIOD;
    audio->au_Request->ioa_Volume = 64;
    audio->au_Request->ioa_Cycles = 1;

    /*
     * audio.device must be entered through BeginIO(): SendIO() clears
     * device-specific flag bits, including ADIOF_PERVOL, which would
     * discard the period and volume configured above.
     */
    BeginIO((struct IORequest *)audio->au_Request);
    audio->au_Running = TRUE;
}
