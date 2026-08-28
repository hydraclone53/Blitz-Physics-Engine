#pragma once
#include "PhysicsWorld.h"
#include <vector>

class CuboidWall
{
public:
    void Spawn(PhysicsWorld& world,
        float originX, float originY, float originZ,
        int width, int height, int depth,
        float spacing);

    void BuildBonds(PhysicsWorld& world, float bondDistance);
    void ApplyBondForces(PhysicsWorld& world, float strainThreshold);
    // CuboidWall.h — add declaration
    void ApplyTestImpulse(PhysicsWorld& world, int particleIndex, float impulseX, float impulseY, float impulseZ);

    const std::vector<int>& GetParticleIndices() const { return particleIndices; }
    int GetBondCount() const { return (int)bonds.size(); }

private:
    struct Bond
    {
        int a, b;
        float restDist;
        bool broken = false;
    };

    std::vector<int> particleIndices;
    std::vector<Bond> bonds;
};