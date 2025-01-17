global_variable real32 tSine = 0;


internal void
OutputTestSineWave(game_sound_output_buffer *soundBuffer, game_state *gameState)
{
    int16 *sampleOut = soundBuffer->Samples;

    real32 toneHz = 256;
    real32 toneVolume = 1000;
    real32 wavePeriod = (real32)soundBuffer->SamplesPerSecond / toneHz;
    
    for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; ++sampleIndex)
    {
        // TODO: bugs in sound, if want to hear broken sound loop unkomment
        // 
        // real32 sineValue = sinf(tSine);
        // int16 sampleValue = (int16)(sineValue * toneVolume);

        int16 sampleValue = 0;
        
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        tSine += (2.0f * Pi32 * 1.0f) / wavePeriod;
    }
}

internal playing_sound*
PlaySound(audio_state *audioState, sound_id id)
{
    if (!audioState->FirstFreePlayingSound) {
        audioState->FirstFreePlayingSound = PushStruct(audioState->Arena, playing_sound);
        audioState->FirstFreePlayingSound->Next = 0;
    }

    playing_sound *sound = audioState->FirstFreePlayingSound;
    audioState->FirstFreePlayingSound = sound->Next;

    sound->SamplesPlayed = 0;
    sound->Volume[0] = 0.2f;
    sound->Volume[1] = 0.2f;
    sound->Id = id;
    sound->Next = audioState->FirstPlayingSound;

    audioState->FirstPlayingSound = sound;

    return sound;
}

internal void
OutputPlayingSounds(
                    audio_state *audioState,
                    game_sound_output_buffer *soundBuffer,
                    game_assets *assets,
                    memory_arena *tempArena
                    )
{
    temporary_memory mixerMemory = BeginTemporaryMemory(tempArena);

    real32 *realChannel_0 = PushArray(tempArena, soundBuffer->SampleCount, real32);
    real32 *realChannel_1 = PushArray(tempArena, soundBuffer->SampleCount, real32);

    // clearout mixer channels
    {
        real32 *dest_0 = realChannel_0;
        real32 *dest_1 = realChannel_1;
        for (int sampleIndex=0;
             sampleIndex < soundBuffer->SampleCount;
             sampleIndex++)
        {
            *dest_0++ = 0.0f;
            *dest_1++ = 0.0f;
        }
    }

    // summ all sounds
    for (playing_sound **playingSoundPtr = &audioState->FirstPlayingSound;
         *playingSoundPtr;
         )
    {
        bool32 isSoundFinished = false;
        playing_sound *playingSound = *playingSoundPtr;

        uint32 totalSamplesToMix = soundBuffer->SampleCount;
        real32 *dest_0 = realChannel_0;
        real32 *dest_1 = realChannel_1;
        
        while (totalSamplesToMix && !isSoundFinished)
        {
            loaded_sound *loadedSound = GetSound(assets, playingSound->Id);
            if (loadedSound)
            {
                asset_sound_info *soundInfo = GetSoundInfo(assets, playingSound->Id);
                PrefetchSound(assets, soundInfo->NextIdToPlay);
            
                real32 vol_0 = playingSound->Volume[0];
                real32 vol_1 = playingSound->Volume[1];

                uint32 samplesToMix = totalSamplesToMix;
                uint32 samplesRemainingInSound = loadedSound->SampleCount - playingSound->SamplesPlayed;
                if (samplesToMix > samplesRemainingInSound) {
                    samplesToMix = samplesRemainingInSound;
                }

                for (uint32 sampleIndex = playingSound->SamplesPlayed;
                     sampleIndex < (playingSound->SamplesPlayed + samplesToMix);
                     ++sampleIndex)
                {
                    real32 sampleValue = loadedSound->Samples[0][sampleIndex];
                    *dest_0++ += sampleValue * vol_0;
                    *dest_1++ += sampleValue * vol_1;
                }

                playingSound->SamplesPlayed += samplesToMix;
                totalSamplesToMix -= samplesToMix;

                if ((uint32)playingSound->SamplesPlayed == loadedSound->SampleCount)
                {
                    if (IsValid(soundInfo->NextIdToPlay))
                    {
                        playingSound->Id = soundInfo->NextIdToPlay;
                        playingSound->SamplesPlayed = 0;
                    }
                    else
                    {
                        isSoundFinished = true;
                    }
                }
                else
                {
                    Assert(totalSamplesToMix == 0);
                    break;
                }
            }
            else
            {
                // nothing to play
                LoadSound(assets, playingSound->Id);
                break;
            }
        }
        
        if (isSoundFinished)
        {
            // once played remove sound from list

            *playingSoundPtr = playingSound->Next;
                
            playingSound->Next = audioState->FirstFreePlayingSound;
            audioState->FirstFreePlayingSound = playingSound;
        }
        else
        {
            playingSoundPtr = &playingSound->Next;
        }
    }

    // convert to 16bits
    {
        real32 *source_0 = realChannel_0;
        real32 *source_1 = realChannel_1;

        int16 *sampleOut = soundBuffer->Samples;
        for (int sampleIndex = 0;
             sampleIndex < soundBuffer->SampleCount;
             ++sampleIndex)
        {
            *sampleOut++ = (int16)(*source_0 + 0.5f);
            *sampleOut++ = (int16)(*source_1 + 0.5f);

            source_0++;
            source_1++;
        }
    }

    EndTemporaryMemory(mixerMemory);
}


internal void
InitializeAudioState(audio_state *audioState, memory_arena *arena)
{
    audioState->Arena = arena;
    audioState->FirstPlayingSound = 0;
    audioState->FirstFreePlayingSound = 0;
}
