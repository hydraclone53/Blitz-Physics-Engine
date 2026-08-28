#pragma once
#include "PhysicsWorld.h"
#include "SpatialHash.h"

namespace InterForce
{
    class PairForce
    {
    public:
        void CalcCForce(PhysicsWorld& world, SpatialHash& hash);
        void CalcVandForce(PhysicsWorld& world, SpatialHash& hash);
        int totalNeighbors;
    private:
        std::vector<int> neighbors;
    };

    class FieldForce
    {
    public:
        void CalcEPolar(PhysicsWorld& world, SpatialHash& hash);
    private:
        std::vector<int> neighbors;
    };

    class RadiationForce
    {
    public:
        void ApplyLarmorDamping(float dt, PhysicsWorld& world, int idx) noexcept;
    };
}