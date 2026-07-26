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
#define FLIP_FIRST_SAMPLES 105
#define FLIP_PAUSE_SAMPLES 30
#define FLIP_SECOND_SAMPLES 165
#define FLIP_SAMPLES                                                           \
    (FLIP_FIRST_SAMPLES + FLIP_PAUSE_SAMPLES + FLIP_SECOND_SAMPLES)

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

static UWORD noiseStep(UWORD noise)
{
    if (noise & 1) return (UWORD)((noise >> 1) ^ 0xb400);
    return noise >> 1;
}

static UWORD generateNoiseBurst(UBYTE *dst, UWORD length, WORD volume,
                                UWORD noise)
{
    UWORD i;

    for (i = 0; i < length; i++)
    {
        WORD envelope = (WORD)(((ULONG)(length - i) * volume) / length);
        WORD value;

        noise = noiseStep(noise);
        value = ((WORD)(BYTE)(noise & 0xff) * envelope) / 127;
        dst[i] = (UBYTE)(BYTE)value;
    }

    return noise;
}

static void generateFlipSound(UBYTE *sample)
{
    UWORD noise;
    UWORD i;

    noise = generateNoiseBurst(sample, FLIP_FIRST_SAMPLES, 34, 0xace1);

    for (i = 0; i < FLIP_PAUSE_SAMPLES; i++)
        sample[FLIP_FIRST_SAMPLES + i] = 0;

    generateNoiseBurst(sample + FLIP_FIRST_SAMPLES + FLIP_PAUSE_SAMPLES,
                       FLIP_SECOND_SAMPLES, 52, noise);
}

BOOL AudioInit(struct AudioContext *audio, BOOL chime, BOOL flipSound)
{
    static UBYTE channelPreference[4] = {1, 2, 4, 8};
    UBYTE *nextSample;

    audio->au_Port = NULL;
    audio->au_Request = NULL;
    audio->au_Samples = NULL;
    audio->au_ChimeSample = NULL;
    audio->au_FlipSample = NULL;
    audio->au_SampleBytes =
        (chime ? CHIME_SAMPLES : 0) + (flipSound ? FLIP_SAMPLES : 0);
    audio->au_Open = FALSE;
    audio->au_Running = FALSE;

    if (audio->au_SampleBytes == 0) return FALSE;

    audio->au_Port = CreatePort(NULL, 0);
    if (!audio->au_Port) return FALSE;

    audio->au_Request =
        (struct IOAudio *)CreateExtIO(audio->au_Port, sizeof(struct IOAudio));
    if (!audio->au_Request)
    {
        AudioExit(audio);
        return FALSE;
    }

    audio->au_Samples = (UBYTE *)AllocMem(audio->au_SampleBytes, MEMF_CHIP);
    if (!audio->au_Samples)
    {
        AudioExit(audio);
        return FALSE;
    }

    nextSample = audio->au_Samples;
    if (chime)
    {
        audio->au_ChimeSample = nextSample;
        generateChime(nextSample);
        nextSample += CHIME_SAMPLES;
    }
    if (flipSound)
    {
        audio->au_FlipSample = nextSample;
        generateFlipSound(nextSample);
    }

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

    if (audio->au_Samples)
    {
        FreeMem(audio->au_Samples, audio->au_SampleBytes);
        audio->au_Samples = NULL;
        audio->au_ChimeSample = NULL;
        audio->au_FlipSample = NULL;
        audio->au_SampleBytes = 0;
    }

    if (audio->au_Port)
    {
        DeletePort(audio->au_Port);
        audio->au_Port = NULL;
    }
}

static void playSample(struct AudioContext *audio, UBYTE *sample, ULONG length)
{
    if (!audio->au_Request || !sample) return;

    if (audio->au_Running)
    {
        if (!CheckIO((struct IORequest *)audio->au_Request)) return;

        WaitIO((struct IORequest *)audio->au_Request);
        audio->au_Running = FALSE;
    }

    audio->au_Request->ioa_Request.io_Command = CMD_WRITE;
    audio->au_Request->ioa_Request.io_Flags = ADIOF_PERVOL;
    audio->au_Request->ioa_Data = sample;
    audio->au_Request->ioa_Length = length;
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

void AudioPlayChime(struct AudioContext *audio)
{
    playSample(audio, audio->au_ChimeSample, CHIME_SAMPLES);
}

void AudioPlayFlip(struct AudioContext *audio)
{
    playSample(audio, audio->au_FlipSample, FLIP_SAMPLES);
}
