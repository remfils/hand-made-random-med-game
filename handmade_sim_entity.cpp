inline void
UpdateFamiliar(sim_region *simRegion, real32 dt, sim_entity *simEntity)
{
    sim_entity *closestHero = 0;
    real32 closestHeroRadSq = Square(90.0f);

    // TODO(casey): make spacial queries
    for (uint32 entityCount = 0;
         entityCount < simRegion->EntityCount;
         ++entityCount)
    {
        sim_entity *testEntity = simRegion->Entities + entityCount;
        if (testEntity->Type == EntityType_Hero)
        {
            real32 testD = LengthSq(testEntity->P - simEntity->P);
            if (testD < closestHeroRadSq)
            {
                closestHero = testEntity;
                closestHeroRadSq = testD;
            }
        }
    }
    
    v2 ddp = {};
    if (closestHero && closestHeroRadSq > 4.0f)
    {
        real32 acceleration = 0.5f;
        real32 oneOverLength = acceleration / SquareRoot(closestHeroRadSq);
        
        ddp = oneOverLength * (closestHero->P - simEntity->P);
        
    }

    move_spec moveSpec;
    moveSpec.UnitMaxAccelVector = true;
    moveSpec.Speed = 50.0f;
    moveSpec.Drag = 8.0f;
    MoveEntity(simRegion, simEntity, dt, &moveSpec, ddp);
}

inline void
UpdateMonster(sim_region *simRegion, real32 dt, sim_entity *entity)
{
    
}

inline void
UpdateSword(sim_region *simRegion, real32 dt, sim_entity *simEntity)
{

    if (IsSet(simEntity, EntityFlag_Nonspacial))
    {
        
    }
    else
    {
        move_spec moveSpec;
        moveSpec.UnitMaxAccelVector = false;
        moveSpec.Speed = 0;
        moveSpec.Drag = 0;

        v2 oldP = simEntity->P;
        MoveEntity(simRegion, simEntity, dt, &moveSpec, V2(0,0));
        real32 discanceTraveled = Length(simEntity->P - oldP);

        simEntity->DistanceRemaining -= discanceTraveled;

        if (simEntity->DistanceRemaining < 0.0f) {
            MakeEntityNonSpacial(simEntity);
        }
    }
}
