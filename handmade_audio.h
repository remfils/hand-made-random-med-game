struct playing_sound
{
    sound_id Id;
    v2 CurrentVolume;
    v2 dCurrentVolume;
    v2 TargetVolume;
    int32 SamplesPlayed;
    playing_sound *Next;
};

struct audio_state
{
    memory_arena *Arena;
    playing_sound *FirstPlayingSound;
    playing_sound *FirstFreePlayingSound;  
};
