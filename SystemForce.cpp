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
    constexpr float sigma = 4.0f ;
    constexpr float epsilon = 2.0f;
    constexpr float r_min_sq = 5.76f;//0.36f;
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
float lerp(float a, float b, float f) {
    return a + f * (b - a);
}
void InterForce::RadiationForce::RetardationFields(PhysicsWorld& world, float dt) noexcept
{
    const float c = PhysConst::c;
    const float c_sq = c * c;
    const float k_const = PhysConst::GetK();
    const double T = world.total_time - (double)dt;
    for (int k = 0; k < world.count; k++)
    {
        if (world.charge[k] == 0) continue;  
        for (int j = 0; j < world.count; j++)
        {
            if (k != j && world.charge[j] != 0)
            {
                int count = 0;
                const auto& h = world.retar[j];
                if (h.r_time.empty()) continue;          // no history yet: skip this source

                double rX1 = world.posX[k] - h.r_wX[0];
                double rY1 = world.posY[k] - h.r_wY[0];
                double rZ1 = world.posZ[k] - h.r_wZ[0];
                double dist1 = std::sqrt(rX1 * rX1 + rY1 * rY1 + rZ1 * rZ1);

                double reach1 = (double)c * (T - h.r_time[0]);
                if (dist1 > reach1) continue;
                const int rows = (int)h.r_time.size();
                int r = rows - 1;
                float f = 0.0f;
                double prev_dist = dist1 - reach1;

                for (int i = 1; i < rows && count == 0; i++)
                {
                    double rX = (double)world.posX[k] - world.retar[j].r_wX[i];
                    double rY = (double)world.posY[k] - world.retar[j].r_wY[i];
                    double rZ = (double)world.posZ[k] - world.retar[j].r_wZ[i];
                    double r_sqrt = std::sqrt(rX * rX + rY * rY + rZ * rZ);
                    double reach = (double)c * (T - h.r_time[i]);
                    double new_dist = r_sqrt - reach;
                    if (new_dist > 0.0f)
                    {
                        count = 1;
                        r = i-1;
                        f = (float)(-prev_dist / (new_dist - prev_dist));

                    }
                    prev_dist = new_dist;
                }
                int r1;
                if (r + 1 < rows) r1 = r + 1;  
                else r1 = r;
                

                const float wX = lerp(h.r_wX[r], h.r_wX[r1], f), wY = lerp(h.r_wY[r], h.r_wY[r1], f), wZ = lerp(h.r_wZ[r], h.r_wZ[r1], f);
                const float vX = lerp(h.r_vX[r], h.r_vX[r1], f), vY = lerp(h.r_vY[r], h.r_vY[r1], f), vZ = lerp(h.r_vZ[r], h.r_vZ[r1], f);
                const float aX = lerp(h.r_aX[r], h.r_aX[r1], f), aY = lerp(h.r_aY[r], h.r_aY[r1], f), aZ = lerp(h.r_aZ[r], h.r_aZ[r1], f);
                               
                float Rx = world.posX[k] - wX;
                float Ry = world.posY[k] - wY;
                float Rz = world.posZ[k] - wZ;
                float R = std::sqrt(Rx * Rx + Ry * Ry + Rz * Rz);
                if (R < 1e-6f) continue;

                float pdx = world.posX[k] - world.posX[j];
                float pdy = world.posY[k] - world.posY[j];
                float pdz = world.posZ[k] - world.posZ[j];
                float Rp2 = pdx * pdx + pdy * pdy + pdz * pdz;
                float s = Rp2 / (Rp2 + 1.5f);
                float soften = s * std::sqrt(s);

                float uRx = Rx / R, uRy = Ry / R, uRz = Rz / R;

                float Ux = c * uRx - vX;
                float Uy = c * uRy - vY;
                float Uz = c * uRz - vZ;

                float D = Rx * Ux + Ry * Uy + Rz * Uz;
                float Dmin = 1e-4f * c * R;
                if (D < Dmin) D = Dmin;

                float v_sq = vX * vX + vY * vY + vZ * vZ;
                float V = c_sq - v_sq;

                float cx = Uy * aZ - Uz * aY;
                float cy = Uz * aX - Ux * aZ;
                float cz = Ux * aY - Uy * aX;

                float X = V * Ux + Ry * cz - Rz * cy;
                float Y = V * Uy + Rz * cx - Rx * cz;
                float Z = V * Uz + Rx * cy - Ry * cx;

                float constant = k_const * world.charge[j] * R / (D * D * D) * soften;
                world.E_FieldX[k] += constant * X;
                world.E_FieldY[k] += constant * Y;
                world.E_FieldZ[k] += constant * Z;

                // world.M_FieldX[k] += (uRy * Ez - uRz * Ey) / c;
                // world.M_FieldY[k] += (uRz * Ex - uRx * Ez) / c;
                // world.M_FieldZ[k] += (uRx * Ey - uRy * Ex) / c;  
            }
        }
    }
}


// Retarded E of source j alone, at point P, at time T.
// Returns false if j is neutral or its signal hasn't reached P yet.
bool InterForce::RadiationForce::ProbeE(const PhysicsWorld& world, int j,
    float px, float py, float pz, double T,
    float& Ex, float& Ey, float& Ez) const noexcept
{
    Ex = Ey = Ez = 0.0f;
    if (world.charge[j] == 0) return false;

    const auto& h = world.retar[j];
    const int rows = (int)h.r_time.size();
    if (rows == 0) return false;

    const float c = PhysConst::c;
    const float c_sq = c * c;
    const float k_const = PhysConst::GetK();

    // gap(i) = distance(P, row i) - light travel since row i.   <= 0 means row i has arrived
    auto gap = [&](int i) -> double
        {
            double dx = (double)px - h.r_wX[i];
            double dy = (double)py - h.r_wY[i];
            double dz = (double)pz - h.r_wZ[i];
            return std::sqrt(dx * dx + dy * dy + dz * dz) - (double)c * (T - h.r_time[i]);
        };

    if (gap(0) > 0.0) return false;           // the signal hasn't reached P at all yet

    // binary search for the first row still in flight (gap > 0); row 0 is known to have arrived
    int lo = 1, hi = rows;
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        if (gap(mid) > 0.0) hi = mid;
        else                lo = mid + 1;
    }
    // lo = first in-flight row, or rows if there is none

    int r = lo - 1;                           // last row that has arrived
    float f = 0.0f;
    if (lo < rows)
    {
        double prev_dist = gap(r);
        double new_dist = gap(lo);
        f = (float)(-prev_dist / (new_dist - prev_dist));
    }
    int r1;
    if (r + 1 < rows) r1 = r + 1;
    else              r1 = r;

    const float wX = lerp(h.r_wX[r], h.r_wX[r1], f), wY = lerp(h.r_wY[r], h.r_wY[r1], f), wZ = lerp(h.r_wZ[r], h.r_wZ[r1], f);
    const float vX = lerp(h.r_vX[r], h.r_vX[r1], f), vY = lerp(h.r_vY[r], h.r_vY[r1], f), vZ = lerp(h.r_vZ[r], h.r_vZ[r1], f);
    const float aX = lerp(h.r_aX[r], h.r_aX[r1], f), aY = lerp(h.r_aY[r], h.r_aY[r1], f), aZ = lerp(h.r_aZ[r], h.r_aZ[r1], f);

    float Rx = px - wX;
    float Ry = py - wY;
    float Rz = pz - wZ;
    float R = std::sqrt(Rx * Rx + Ry * Ry + Rz * Rz);
    if (R < 1e-6f) return false;

    // softening from the PRESENT separation, same as RetardationFields
    float pdx = px - world.posX[j];
    float pdy = py - world.posY[j];
    float pdz = pz - world.posZ[j];
    float Rp2 = pdx * pdx + pdy * pdy + pdz * pdz;
    float s = Rp2 / (Rp2 + 1.5f);
    float soften = s * std::sqrt(s);

    float uRx = Rx / R, uRy = Ry / R, uRz = Rz / R;

    float Ux = c * uRx - vX;
    float Uy = c * uRy - vY;
    float Uz = c * uRz - vZ;

    float D = Rx * Ux + Ry * Uy + Rz * Uz;
    float Dmin = 1e-4f * c * R;
    if (D < Dmin) D = Dmin;

    float v_sq = vX * vX + vY * vY + vZ * vZ;
    float V = c_sq - v_sq;

    float cx = Uy * aZ - Uz * aY;
    float cy = Uz * aX - Ux * aZ;
    float cz = Ux * aY - Uy * aX;

    float X = V * Ux + Ry * cz - Rz * cy;
    float Y = V * Uy + Rz * cx - Rx * cz;
    float Z = V * Uz + Rx * cy - Ry * cx;

    float constant = k_const * (float)world.charge[j] * R / (D * D * D) * soften;
    Ex = constant * X;
    Ey = constant * Y;
    Ez = constant * Z;
    return true;
}