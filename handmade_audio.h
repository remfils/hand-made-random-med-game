#if !defined(HANDMADE_AUDIO_H)
#define HANDMADE_AUDIO_H


struct playing_sound
{
    sound_id Id;
    v2 CurrentVolume;
    v2 dCurrentVolume;
    v2 TargetVolume;
    real32 SamplesPlayed;
    real32 dSample;
    playing_sound *Next;
};

struct audio_state
{
    memory_arena *Arena;
    playing_sound *FirstPlayingSound;
    playing_sound *FirstFreePlayingSound;  
};
#endif
