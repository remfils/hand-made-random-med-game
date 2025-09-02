#if !defined(HANDMADE_DEBUG_H)
#define HANDMADE_DEBUG_H


#define TIMED_BLOCK(id) debug_timed_block Block(DebugCounter_##id);

struct debug_timed_block
{
    u32 id;
    u64 counter;
    
    debug_timed_block(u32 id)
    {
        this->id = id;
        this->counter = __rdtsc();
    }

    ~debug_timed_block()
    {
        DebugGlobalMemory->DebugCounters[this->id].CycleCount += __rdtsc() - this->counter;
        DebugGlobalMemory->DebugCounters[this->id].HitCount++;
    }
};


#endif