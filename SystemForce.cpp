#include "SystemForce.h"
#include "PhysConstants.h"
#include <cmath>
#include <chrono>
#include "imgui/imgui.h"

void InterForce::PairForce::CalcCForce(PhysicsWorld& world, SpatialHash& hash)
{
    for (int j = 0; j < world.count; j++)
    {
        hash.QueryNeighbors(j, neighbors);

        float jx = world.posX[j], jy = world.posY[j], jz = world.posZ[j];
        int   q_1 = world.charge[j];

        for (int idx : neighbors)
        {
            if (idx <= j) continue;

            int q_2 = world.charge[idx];
            if (q_1 == 0 && q_2 == 0) continue;

            float dx = jx - world.posX[idx];
            float dy = jy - world.posY[idx];
            float dz = jz - world.posZ[idx];
            float dist_sq = dx * dx + dy * dy + dz * dz + 1.5f;
            float dist_cb = dist_sq * std::sqrt(dist_sq);
            float k = PhysConst::GetK();

            if (q_2 == 0)
            {
                float scale = -k / dist_cb * (float)q_1;
                world.eLocalX[idx] += dx * scale;
                world.eLocalY[idx] += dy * scale;
                world.eLocalZ[idx] += dz * scale;
            }
            else if (q_1 == 0)
            {
                float scale = k / dist_cb * (float)q_2;
                world.eLocalX[j] += dx * scale;
                world.eLocalY[j] += dy * scale;
                world.eLocalZ[j] += dz * scale;
            }

            if (q_1 != 0 && q_2 != 0)
            {
                float scale_2 = k / dist_cb * (float)q_1;
                float scale_1 = k / dist_cb * (float)q_2;
                world.E_FieldX[idx] -= dx * scale_2;
                world.E_FieldY[idx] -= dy * scale_2;
                world.E_FieldZ[idx] -= dz * scale_2;
                world.E_FieldX[j] += dx * scale_1;
                world.E_FieldY[j] += dy * scale_1;
                world.E_FieldZ[j] += dz * scale_1;
            }
        }
    }
}

void InterForce::PairForce::CalcVandForce(PhysicsWorld& world, SpatialHash& hash)
{
    constexpr float sigma = 1.0f ;
    constexpr float epsilon = 2.0f;
    constexpr float r_min_sq = 0.36f;
    constexpr float sigma_sq = sigma * sigma;

    for (auto& p : hash.pairs)  // iterate flat pair list — no hash lookups
    {
        int a = p.a, b = p.b;

        float dx = world.posX[a] - world.posX[b];
        float dy = world.posY[a] - world.posY[b];
        float dz = world.posZ[a] - world.posZ[b];
        float r_sq = dx * dx + dy * dy + dz * dz;
        float r_sq_c = r_sq < r_min_sq ? r_min_sq : r_sq;

        float sr2 = sigma_sq / r_sq_c;
        float sr6 = sr2 * sr2 * sr2;
        float sr12 = sr6 * sr6;
        float scale = 24.0f * epsilon * (2.0f * sr12 - sr6) / r_sq_c;

        world.forceX[b] -= dx * scale;
        world.forceY[b] -= dy * scale;
        world.forceZ[b] -= dz * scale;
        world.forceX[a] += dx * scale;
        world.forceY[a] += dy * scale;
        world.forceZ[a] += dz * scale;
    }
}
void InterForce::FieldForce::CalcEPolar(PhysicsWorld& world, SpatialHash& hash)
{
    for (int j = 0; j < world.count; j++)
    {
        if (world.charge[j] != 0) continue;

        float px = world.eTotalX[j] * world.polarizability[j];
        float py = world.eTotalY[j] * world.polarizability[j];
        float pz = world.eTotalZ[j] * world.polarizability[j];

        float jx = world.posX[j], jy = world.posY[j], jz = world.posZ[j];

        hash.QueryNeighbors(j, neighbors);

        for (int idx : neighbors)
        {
            if (world.charge[idx] == 0) continue;

            float dx = world.posX[idx] - jx;
            float dy = world.posY[idx] - jy;
            float dz = world.posZ[idx] - jz;
            float dist_sq = dx * dx + dy * dy + dz * dz + 1.5f;
            float dist_cb = dist_sq * std::sqrt(dist_sq);

            float dot = dx * px + dy * py + dz * pz;
            float scale_neut = PhysConst::GetK() * (float)world.charge[idx] / dist_cb;
            float scale_charged = PhysConst::GetK()  / dist_cb;

            float forceX = scale_neut * (3.0f * dot / dist_sq * dx - px);
            float forceY = scale_neut * (3.0f * dot / dist_sq * dy - py);
            float forceZ = scale_neut * (3.0f * dot / dist_sq * dz - pz);

            float fieldX = scale_charged * (3.0f * dot / dist_sq * dx - px);
            float fieldY = scale_charged * (3.0f * dot / dist_sq * dy - py);
            float fieldZ = scale_charged * (3.0f * dot / dist_sq * dz - pz);


            world.E_FieldX[idx] += fieldX;
            world.E_FieldY[idx] += fieldY;
            world.E_FieldZ[idx] += fieldZ;
            world.forceX[j] -= forceX;
            world.forceY[j] -= forceY;
            world.forceZ[j] -= forceZ;
        }
    }
}

void InterForce::RadiationForce::ApplyLarmorDamping(float dt,
    PhysicsWorld& world,
    int idx) noexcept
{
    if (world.charge[idx] == 0) return;

    float b = PhysConst::GetB();
    float q2 = (float)(world.charge[idx] * world.charge[idx]);

    accelXholder = world.accelX[idx];
    //jerkX = (world.accelX[idx] - accelXholder) / dt;
    world.forceX[idx] -= world.accelX[idx] * q2 * b;
    world.forceY[idx] -= world.accelY[idx] * q2 * b;
    world.forceZ[idx] -= world.accelZ[idx] * q2 * b;
}