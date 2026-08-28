#include "CuboidWall.h"
#include <cmath>

void CuboidWall::Spawn(PhysicsWorld& world,
    float originX, float originY, float originZ,
    int width, int height, int depth,
    float spacing)
{
    particleIndices.clear();
    particleIndices.reserve(width * height * depth);

    for (int i = 0; i < width; i++)
        for (int j = 0; j < height; j++)
            for (int k = 0; k < depth; k++)
            {
                float x = originX + i * spacing;
                float y = originY + j * spacing;
                float z = originZ + k * spacing;

                int idx = world.Add(x, y, z, 0.0f, 0.0f, 0.0f, 0, 1.0f);
                particleIndices.push_back(idx);
            }
}

void CuboidWall::BuildBonds(PhysicsWorld& world, float bondDistance)
{
    bonds.clear();
    float bondDist_sq = bondDistance * bondDistance;

    int n = (int)particleIndices.size();
    for (int i = 0; i < n; i++)
    {
        int a = particleIndices[i];
        float ax = world.posX[a], ay = world.posY[a], az = world.posZ[a];

        for (int j = i + 1; j < n; j++)
        {
            int b = particleIndices[j];
            float dx = ax - world.posX[b];
            float dy = ay - world.posY[b];
            float dz = az - world.posZ[b];
            float dist_sq = dx * dx + dy * dy + dz * dz;

            if (dist_sq < bondDist_sq)
            {
                float restDist = std::sqrt(dist_sq);
                bonds.push_back({ a, b, restDist, false });
            }
        }
    }
}

void CuboidWall::ApplyBondForces(PhysicsWorld& world, float strainThreshold)
{
    constexpr float epsilon = 2.0f;
    constexpr float equilibrium_correction = 0.8908987f;

    for (Bond& bond : bonds)
    {
        if (bond.broken) continue;

        int a = bond.a, b = bond.b;

        float dx = world.posX[a] - world.posX[b];
        float dy = world.posY[a] - world.posY[b];
        float dz = world.posZ[a] - world.posZ[b];
        float r_sq = dx * dx + dy * dy + dz * dz;
        float r = std::sqrt(r_sq);

        // NEW — check for bond breaking
        if (r > bond.restDist * strainThreshold)
        {
            bond.broken = true;
            continue;  // skip force calculation for this now-broken bond
        }

        float sigma = bond.restDist * equilibrium_correction;
        float sigma_sq = sigma * sigma;

        constexpr float r_min_sq = 0.36f;
        float r_sq_c = r_sq < r_min_sq ? r_min_sq : r_sq;

        float sr2 = sigma_sq / r_sq_c;
        float sr6 = sr2 * sr2 * sr2;
        float sr12 = sr6 * sr6;
        float scale = 24.0f * epsilon * (2.0f * sr12 - sr6) / r_sq_c;

        world.forceX[a] += dx * scale;
        world.forceY[a] += dy * scale;
        world.forceZ[a] += dz * scale;
        world.forceX[b] -= dx * scale;
        world.forceY[b] -= dy * scale;
        world.forceZ[b] -= dz * scale;
    }
}

// CuboidWall.cpp
void CuboidWall::ApplyTestImpulse(PhysicsWorld& world, int particleIndex,
    float impulseX, float impulseY, float impulseZ)
{
    // directly modify momentum (u) — simulates a sudden impact
    world.uX[particleIndex] += impulseX;
    world.uY[particleIndex] += impulseY;
    world.uZ[particleIndex] += impulseZ;
}