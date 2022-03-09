#if !defined(HANDMADE_SIM_REGION_H)
#define HANDMADE_SIM_REGION_H


struct sim_entity
{
    uint32 StorageIndex;

    v2 P;
    uint32 AbsTileZ;    
};

struct sim_region
{
    // TODO: hash table stored -> sim

    world *World;

    world_position Origin;
    rectangle2 Bounds;

    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity *Entities;
};


#endif
