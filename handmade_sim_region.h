#if !defined(HANDMADE_SIM_REGION_H)
#define HANDMADE_SIM_REGION_H


struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
};



#define HIT_POINT_SUB_COUNT 4

struct hit_point
{
    // TODO: collapse into one variable
    uint8 Flags;
    uint32 FilledAmount;

};

enum entity_type
{
    EntityType_NULL,
    EntityType_Hero,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monster,
    EntityType_Sword
};

struct sim_entity;
union entity_reference
{
    uint32 Index;
    sim_entity *Ptr;
};

enum sim_entity_flags
{
    EntityFlag_Collides = (1 << 1),
    EntityFlag_Nonspacial = (1 << 2)
};

struct sim_entity
{

    // only for sim region
    uint32 StorageIndex;
    bool32 Updatable;
    

    entity_type Type;
    uint32 Flags;

    v3 P;
    v3 dP;
    v3 Dim;

    uint32 FacingDirection;
    real32 TBobing;

    // TODO: should hit points be entities
    uint32 HitPointMax;
    hit_point HitPoints[16];

    entity_reference Sword;
    real32 DistanceLimit;
};

struct sim_entity_hash
{
    sim_entity *Ptr;
    uint32 Index;
};

struct sim_region
{
    // TODO: hash table stored -> sim
    real32 MaxEntityRadius;
    real32 MaxEntityVelocity;

    world *World;

    world_position Origin;
    rectangle3 Bounds;
    rectangle3 UpdatableBounds;

    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity *Entities;


    // TODO(casey): is hash needed?
    // NOTE: must be a power of two for hash
    sim_entity_hash Hash[4096];
};



#endif
