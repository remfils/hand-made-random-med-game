inline void
ReleaseAssetMemory(game_assets *assets, memory_index size, void *memory)
{
    if (memory) {
        PlatformAPI.FreeMemory(memory);
        assets->TotalMemoryUsed -= size;
    }
}

internal void*
AcquireAssetMemory(game_assets *assets, memory_index size)
{

    void *result = 0;
    
    EvictAssetsAsNecessary(assets);

    result = PlatformAPI.AllocateMemory(size);

    if (result) {
        assets->TotalMemoryUsed += size;
    }
    

    return result;
}