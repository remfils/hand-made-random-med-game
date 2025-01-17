struct playing_sound
{
    sound_id Id;
    real32 Volume[2];
    int32 SamplesPlayed;
    playing_sound *Next;
};

struct audio_state
{
    memory_arena *Arena;
    playing_sound *FirstPlayingSound;
    playing_sound *FirstFreePlayingSound;  
};
