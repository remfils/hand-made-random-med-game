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
        // TODO: bugs in sound, if want to hear broken sound loop uncomment
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

    sound->CurrentVolume = sound->TargetVolume = v2{1.0f, 1.0f};
    sound->dCurrentVolume = v2{0.0f, 0.0f};
    sound->dSample = 1.0f;
    
    sound->Id = id;
    sound->Next = audioState->FirstPlayingSound;

    audioState->FirstPlayingSound = sound;

    return sound;
}

internal void
ChangeVolume(audio_state *audioState, playing_sound *sound, real32 fadeDurationInSeconds, v2 volume)
{
    sound->TargetVolume = volume;
    
    if (fadeDurationInSeconds <= 0.0f)
    {
        sound->CurrentVolume = sound->TargetVolume;
    }
    else
    {
        real32 oneOver = 1.f / fadeDurationInSeconds;
        sound->dCurrentVolume = (sound->TargetVolume - sound->CurrentVolume) * oneOver;
    }
}

internal void
ChangePitch(playing_sound *sound, real32 dSample)
{
    sound->dSample = dSample;
}

internal void
OutputPlayingSounds(
                    audio_state *audioState,
                    game_sound_output_buffer *soundBuffer,
                    game_assets *assets,
                    memory_arena *tempArena
                    )
{
    #define SUPPORTED_CHANNEL_COUNT 2

    Assert((soundBuffer->SampleCount & 7) == 0);

    u32 chunkCount = soundBuffer->SampleCount / 4;

    temporary_memory mixerMemory = BeginTemporaryMemory(tempArena);

    real32 secondsPerSample = 1.0f / (real32) soundBuffer->SamplesPerSecond;

    __m128 *realChannel0 = PushArray(tempArena, chunkCount, __m128, 16);
    __m128 *realChannel1 = PushArray(tempArena, chunkCount, __m128, 16);


    // clearout mixer channels
    __m128 zero = _mm_set1_ps(0.0f);
    {
        __m128 *dest_0 = realChannel0;
        __m128 *dest_1 = realChannel1;

        
        for (u32 sampleIndex=0;
             sampleIndex < chunkCount;
             sampleIndex++)
        {
            _mm_store_ps((float *)dest_0++, zero);
            _mm_store_ps((float *)dest_1++, zero);
        }
    }

    // summ all sounds
    for (playing_sound **playingSoundPtr = &audioState->FirstPlayingSound;
         *playingSoundPtr;
         )
    {
        bool32 isSoundFinished = false;
        playing_sound *playingSound = *playingSoundPtr;

        uint32 totalChunksToMix = chunkCount;
        __m128 *dest0 = realChannel0;
        __m128 *dest1 = realChannel1;
        
        while (totalChunksToMix && !isSoundFinished)
        {
            loaded_sound *loadedSound = GetSound(assets, playingSound->Id);
            if (loadedSound)
            {
                asset_sound_info *soundInfo = GetSoundInfo(assets, playingSound->Id);
                PrefetchSound(assets, soundInfo->NextIdToPlay);

                v2 volume = playingSound->CurrentVolume;
                v2 dVolume = secondsPerSample * playingSound->dCurrentVolume;
                v2 dVolumeChunk = 4.0f * dVolume;
                real32 dSample = playingSound->dSample;
                real32 dSampleChunk = 4.0f * playingSound->dSample;

                
                __m128 volume0 = _mm_set_ps(volume.E[0] + 0.0f * dVolume.E[0],
                                              volume.E[0] + 1.0f * dVolume.E[0],
                                              volume.E[0] + 2.0f * dVolume.E[0],
                                              volume.E[0] + 3.0f * dVolume.E[0]
                                              );
                __m128 dVolume0 = _mm_set1_ps(dVolume.E[0]);
                __m128 dVolumeChunk0 = _mm_set1_ps(dVolumeChunk.E[0]);

                
                __m128 volume1 = _mm_set_ps(volume.E[1] + 0.0f * dVolume.E[1],
                                              volume.E[1] + 1.0f * dVolume.E[1],
                                              volume.E[1] + 2.0f * dVolume.E[1],
                                              volume.E[1] + 3.0f * dVolume.E[1]
                                              );
                __m128 dVolume1 = _mm_set1_ps(dVolume.E[1]);
                __m128 dVolumeChunk1 = _mm_set1_ps(dVolumeChunk.E[1]);


                uint32 chunksToMix = totalChunksToMix;
                real32 realChunksRemainingInSound = (loadedSound->SampleCount - RoundReal32ToInt32(playingSound->SamplesPlayed)) / dSampleChunk;
                u32 chunksRemainingInSound = RoundReal32ToInt32(realChunksRemainingInSound);
                b32 inputEnded = false;
                if (chunksToMix > chunksRemainingInSound) {
                    chunksToMix = chunksRemainingInSound;
                    inputEnded = true;
                }

                bool32 volumeEnded[SUPPORTED_CHANNEL_COUNT] = {};
                for (uint32 chidx=0; chidx < SUPPORTED_CHANNEL_COUNT; chidx++)
                {
                    // NOTE: epsilon rounding added
                    if (dVolume.E[chidx] >= 0.00001f)
                    {
                        real32 volumeDelta = playingSound->TargetVolume.E[chidx] - volume.E[chidx];
                        // rounding of flaot
                        
                        uint32 volumeChunkCount = (uint32)(((volumeDelta / dVolumeChunk.E[chidx]) + 0.5f));
                        if (chunksToMix > volumeChunkCount)
                        {
                            chunksToMix = volumeChunkCount;
                            volumeEnded[chidx] = true;
                        }
                    }
                }

                real32 beginSamplePosition = playingSound->SamplesPlayed;
                real32 endSamplePosition = beginSamplePosition + chunksToMix * dSampleChunk;
                real32 loopIndexC = (endSamplePosition - beginSamplePosition) / (r32) chunksToMix;
                for (u32 loopIndex = 0; loopIndex < chunksToMix; loopIndex++) {
                    real32 samplePosition = beginSamplePosition + loopIndexC * (r32) loopIndex;

                    #if 0
                    real32 offsetSamplePosition = samplePosition + ((real32)sampleOffset * dSample);
                    u32 sampleIndex = FloorReal32ToInt32(offsetSamplePosition);
                    r32 frac = offsetSamplePosition - (r32)sampleIndex;

                    r32 sample0 = (r32)loadedSound->Samples[0][sampleIndex];
                    r32 sample1 = (r32)loadedSound->Samples[0][sampleIndex+1];
                    r32 sampleValue = Lerp(sample0, frac, sample1);
                    #endif

                    __m128 sampleValue = _mm_setr_ps(loadedSound->Samples[0][RoundReal32ToInt32(samplePosition + 0.0f * dSample)],
                                                      loadedSound->Samples[0][RoundReal32ToInt32(samplePosition + 1.0f * dSample)],
                                                      loadedSound->Samples[0][RoundReal32ToInt32(samplePosition + 2.0f * dSample)],
                                                      loadedSound->Samples[0][RoundReal32ToInt32(samplePosition + 3.0f * dSample)]
                                                      );

                    __m128 d0 = _mm_load_ps((float *)&dest0[0]);
                    __m128 d1 = _mm_load_ps((float *)&dest1[0]);

                    d0 = _mm_add_ps(d0, _mm_mul_ps(volume0, sampleValue));
                    d1 = _mm_add_ps(d1, _mm_mul_ps(volume1, sampleValue));

                    _mm_store_ps((float *)&dest0[0], d0);
                    _mm_store_ps((float *)&dest1[0], d1);

                    dest0 ++;
                    dest1 ++;

                    volume += dVolumeChunk;
                    volume0 = _mm_add_ps(volume0, dVolumeChunk0);
                    volume1 = _mm_add_ps(volume1, dVolumeChunk1);
                }

                playingSound->CurrentVolume = volume;

                // TODO(casey): make better code
                for (uint32 chidx=0; chidx < SUPPORTED_CHANNEL_COUNT; chidx++)
                {
                    if (volumeEnded[chidx]) {
                        playingSound->CurrentVolume.E[chidx] = playingSound->TargetVolume.E[chidx];
                        dVolume.E[chidx] = 0.0f;
                    }
                }

                playingSound->SamplesPlayed = endSamplePosition;
                totalChunksToMix -= chunksToMix;
                
                // TODO: this is no an acceptable check for the end
                if (inputEnded)
                {
                    if (IsValid(soundInfo->NextIdToPlay))
                    {
                        playingSound->Id = soundInfo->NextIdToPlay;
                        playingSound->SamplesPlayed -= (real32)loadedSound->SampleCount;
                    }
                    else
                    {
                        isSoundFinished = true;
                    }
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

    {
        __m128 *source0 = realChannel0;
        __m128 *source1 = realChannel1;

        __m128i *sampleOut = (__m128i *)soundBuffer->Samples;
        for (u32 sampleIndex = 0;
             sampleIndex < chunkCount;
             ++sampleIndex)
        {
            __m128 s0 = _mm_load_ps((float *)source0++);
            __m128 s1 = _mm_load_ps((float *)source1++);
            
            // convert to 16bit ints with high saturation
            __m128i l = _mm_cvtps_epi32(s0);
            __m128i r = _mm_cvtps_epi32(s1);

            // interleave L + R channels
            __m128i lr0 = _mm_unpacklo_epi32(l, r);
            __m128i lr1 = _mm_unpackhi_epi32(l, r);

            // pack into output
            *sampleOut++ = _mm_packs_epi32(lr0, lr1);
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
