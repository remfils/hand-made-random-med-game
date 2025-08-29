internal void
RemoveAssetHeaderFromList(asset_memory_header *header)
{
    header->Prev->Next = header->Next;
    header->Next->Prev = header->Prev;

    header->Prev = header->Next = 0;
}

internal asset_memory_block*
InsertBlock(asset_memory_block *prev,  memory_index size, void *mem)
{
    Assert(size > sizeof(asset_memory_block));
    asset_memory_block *block = (asset_memory_block *)mem;
    block->Flags = 0;
    block->Size = size - sizeof(asset_memory_block);
    block->Prev = prev;
    block->Next = prev->Next;
    block->Prev->Next = block;
    block->Next->Prev = block;

    return block;
}

internal asset_memory_block *
FindBlockForSize(game_assets *assets, memory_index size)
{
    // TODO: best match block
    // TODO: this will be sloooow once asset count will be large
    asset_memory_block *result = 0;

    for (
         asset_memory_block * block = assets->MemorySentinel.Next;
         block != &assets->MemorySentinel;
         block = block->Next
         )
    {
        if (block->Size >= size && !(block->Flags & AssetMemory_Used)) {
            result = block;
            break;
        }
    }

    return result;
}

internal b32
MergeIfPossible(game_assets *assets, asset_memory_block *first, asset_memory_block *second)
{
    b32 result = false;
    if (first != &assets->MemorySentinel && second != &assets->MemorySentinel)
    {
        if (!(first->Flags & AssetMemory_Used) && !(second->Flags & AssetMemory_Used))
        {
            u8 *expectedSecond = (u8 *)first + sizeof(asset_memory_block) + first->Size;
            if ((u8 *) second == expectedSecond)
            {
                second->Next->Prev = second->Prev;
                second->Prev->Next = second->Next;

                first->Size += sizeof(asset_memory_header) + second->Size;
                result = true;
            }
        }
    }

    return result;
}

inline void EvictAsset(game_assets *assets, asset *asset);

internal void*
AcquireAssetMemory(game_assets *assets, memory_index size)
{
    void *result = 0;

    asset_memory_block *block = FindBlockForSize(assets, size);
    
    for (;;)
    {
        if (block && size <= block->Size)
        {
            block->Flags |= AssetMemory_Used;
            result = (u8 *)(block + 1);

            memory_index remainingSize = block->Size - size;

            memory_index blockSplitThreshold = 4096;

            if (remainingSize >= blockSplitThreshold)
            {
                block->Size -= remainingSize;
                InsertBlock(block, remainingSize, (u8 *)result + size);
            }
            break;
        }
        else
        {
            for (
                 asset_memory_header *header = assets->LoadedAssetsSentinel.Prev;
                 header != &assets->LoadedAssetsSentinel;
                 header = header->Prev
                 )
            {
                asset *asset = assets->Assets + header->AssetIndex;
                if (asset->State >= AssetState_Loaded) {
                    u32 assetIndex = asset->Header->AssetIndex;

                    RemoveAssetHeaderFromList(asset->Header);

                    block = (asset_memory_block *)asset->Header -1;
                    block->Flags &= ~AssetMemory_Used;
    
                    asset->State = AssetState_Unloaded;
                    asset->Header = 0;

                    if (MergeIfPossible(assets, block->Prev, block)) {
                        block = block->Prev;
                    }

                    MergeIfPossible(assets, block, block->Next);

                    // TODO: make it return block
                    // block = EvictAsset(assets, asset);
                    break;
                }
            }
        }
    }

    return result;
}