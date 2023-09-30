inline LARGE_INTEGER Win32GetWallClock(void);

int32 STEP_MAX_VAL = 5000;
bool32 IS_REMFILS_BLOCKED_LOGGING = 1;

struct remfils_step {
    char Key[3];
    int64 Start;
    int64 End;
};

struct remfils_step_state {
    int64 GlobalPerfCounterFrequency;
    remfils_step Steps[5000];
    int64 CurrentStep;
};

remfils_step_state* RemfilsInitState(int64 globalPerfCounterFrequency)
{
    remfils_step_state* result = (remfils_step_state*) VirtualAlloc(0, sizeof(remfils_step_state), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);;
    result->GlobalPerfCounterFrequency = globalPerfCounterFrequency;

    result->CurrentStep = 0;

    return result;
}

void RemfilsStartStep(remfils_step_state* state, char key[3])
{
    if (IS_REMFILS_BLOCKED_LOGGING) return;
    
    LARGE_INTEGER clock = Win32GetWallClock();

    for (int32 i=0; i<3; i++) {
        state->Steps[state->CurrentStep].Key[i] = key[i];
    }

    state->Steps[state->CurrentStep].Start = clock.QuadPart;
}

void RemfilsEndStep(remfils_step_state* state)
{
    if (IS_REMFILS_BLOCKED_LOGGING) return;
    
    LARGE_INTEGER clock = Win32GetWallClock();
    state->Steps[state->CurrentStep].End = clock.QuadPart;
    
    state->CurrentStep++;

    if (state->CurrentStep >= STEP_MAX_VAL) {
        state->CurrentStep = 0;

        HANDLE fileHandle = CreateFileA(
                                        "report.csv",
                                        GENERIC_WRITE,
                                        0,
                                        0,
                                        CREATE_ALWAYS,
                                        0,
                                        0);

        // dump state
        for (int32 i=0; i<STEP_MAX_VAL; i++) {
            real32 duration = (real32)(state->Steps[i].End - state->Steps[i].Start) / (real32)state->GlobalPerfCounterFrequency;
            char outputBuffer[50];
            int32 charCount = sprintf_s(outputBuffer, "%s;%f\n", state->Steps[i].Key, duration);

            DWORD bytesWritten;
            WriteFile(fileHandle, outputBuffer, sizeof(char) * charCount, &bytesWritten, 0);
        }
    }
}
