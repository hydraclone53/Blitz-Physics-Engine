#include "PhysicsWorld.h"

int PhysicsWorld::Add(float x, float y, float z,
    float ux, float uy, float uz,
    int q, float m)
{
    posX.push_back(x);   posY.push_back(y);   posZ.push_back(z);
    uX.push_back(ux);    uY.push_back(uy);    uZ.push_back(uz);
    gamma.push_back(1.0f);
    accelX.push_back(0.0f); accelY.push_back(0.0f); accelZ.push_back(0.0f);
    smoothedIntensity.push_back(0.0f);
    mass.push_back(m);
    charge.push_back(q);
    forceX.push_back(0.0f);  forceY.push_back(0.0f);  forceZ.push_back(0.0f);
    E_FieldX.push_back(0.0f);  E_FieldY.push_back(0.0f);  E_FieldZ.push_back(0.0f);
    M_FieldX.push_back(0.0f);  M_FieldY.push_back(0.0f);  M_FieldZ.push_back(0.0f);
    eLocalX.push_back(0.0f); eLocalY.push_back(0.0f); eLocalZ.push_back(0.0f);
    eTotalX.push_back(0.0f); eTotalY.push_back(0.0f); eTotalZ.push_back(0.0f);
    polarizability.push_back(0.5f);
    return count++;
}

void PhysicsWorld::Clear()
{
    posX.clear();    posY.clear();    posZ.clear();
    uX.clear();      uY.clear();      uZ.clear();
    gamma.clear();
    accelX.clear();  accelY.clear();  accelZ.clear();
    smoothedIntensity.clear();
    mass.clear();    charge.clear();
    forceX.clear();  forceY.clear();  forceZ.clear();
    E_FieldX.clear();  E_FieldY.clear();  E_FieldZ.clear();
    M_FieldX.clear();  M_FieldY.clear();  M_FieldZ.clear();
    eLocalX.clear(); eLocalY.clear(); eLocalZ.clear();
    eTotalX.clear(); eTotalY.clear(); eTotalZ.clear();
    polarizability.clear();
    count = 0;
}

void PhysicsWorld::ResetAccumulators()
{
    for (int i = 0; i < count; i++)
    {
        forceX[i] = forceY[i] = forceZ[i] = 0.0f;
        E_FieldX[i] = E_FieldY[i] = E_FieldZ[i] = 0.0f;
        M_FieldX[i] = M_FieldY[i] = M_FieldZ[i] = 0.0f;
        eLocalX[i] = eLocalY[i] = eLocalZ[i] = 0.0f;
        eTotalX[i] = eTotalY[i] = eTotalZ[i] = 0.0f;
    }
}

void PhysicsWorld::AddEtotalAll()
{
    for (int i = 0; i < count; i++)
    {
        eTotalX[i] = eLocalX[i];
        eTotalY[i] = eLocalY[i];
        eTotalZ[i] = eLocalZ[i];
    }
}

void PhysicsWorld::StepAllBoris(float dt) noexcept
{
    const float c_sq = PhysConst::c * PhysConst::c;

    for (int idx = 0; idx < count; idx++)
    {
        float ux = uX[idx];
        float uy = uY[idx];
        float uz = uZ[idx];

        float u_sq = ux * ux + uy * uy + uz * uz;
        float gam = std::sqrt(1.0f + u_sq / c_sq);
        gamma[idx] = gam;

        float q_over_m = (float)charge[idx] / mass[idx];

 
      //Step-1 Half electric push

        float electricPushOneX = ux + E_FieldX[idx] * q_over_m * dt * 0.5f;
        float electricPushOneY = uy + E_FieldY[idx] * q_over_m * dt * 0.5f;
        float electricPushOneZ = uz + E_FieldZ[idx] * q_over_m * dt * 0.5f;
        

        // Step-2 magnetic rotation step
        float tx = q_over_m * M_FieldX[idx] * dt * 0.5f / gam;
        float ty = q_over_m * M_FieldY[idx] * dt * 0.5f / gam;
        float tz = q_over_m * M_FieldZ[idx] * dt * 0.5f / gam;

        float t_sq = tx * tx + ty * ty + tz * tz;

        float sx = 2.0f * tx / (1.0f + t_sq);
        float sy = 2.0f * ty / (1.0f + t_sq);
        float sz = 2.0f * tz / (1.0f + t_sq);

        // u' = u_minus + u_minus x t
        float magneticRotOneX = electricPushOneX + (electricPushOneY * tz - electricPushOneZ * ty);
        float magneticRotOneY = electricPushOneY + (electricPushOneZ * tx - electricPushOneX * tz);
        float magneticRotOneZ = electricPushOneZ + (electricPushOneX * ty - electricPushOneY * tx);

        // u_plus = u_minus + u' x s
        float magneticRotTwoX = electricPushOneX + (magneticRotOneY * sz - magneticRotOneZ * sy);
        float magneticRotTwoY = electricPushOneY + (magneticRotOneZ * sx - magneticRotOneX * sz);
        float magneticRotTwoZ = electricPushOneZ + (magneticRotOneX * sy - magneticRotOneY * sx);


        //Step-3 Second half electric push
        float electricPushTwoX = magneticRotTwoX + E_FieldX[idx] * q_over_m * dt * 0.5f;
        float electricPushTwoY = magneticRotTwoY + E_FieldY[idx] * q_over_m * dt * 0.5f;
        float electricPushTwoZ = magneticRotTwoZ + E_FieldZ[idx] * q_over_m * dt * 0.5f;

        float final_uX = electricPushTwoX + forceX[idx] / mass[idx] * dt;
        float final_uY = electricPushTwoY + forceY[idx] / mass[idx] * dt;
        float final_uZ = electricPushTwoZ + forceZ[idx] / mass[idx] * dt;

        accelX[idx] = (final_uX - ux) / dt;
        accelY[idx] = (final_uY - uy) / dt;
        accelZ[idx] = (final_uZ - uz) / dt;

        float un_sq = final_uX * final_uX + final_uY * final_uY + final_uZ * final_uZ;
        float new_gam = std::sqrt(1.0f + un_sq / c_sq);
        float inv_gam = 1.0f / new_gam;

        posX[idx] += final_uX * inv_gam * dt;
        posY[idx] += final_uY * inv_gam * dt;
        posZ[idx] += final_uZ * inv_gam * dt;

        uX[idx] = final_uX;
        uY[idx] = final_uY;
        uZ[idx] = final_uZ;
    }
}

float PhysicsWorld::GetRadiationIntensity(int idx) noexcept
{
    if (charge[idx] == 0) return 0.0f;
    float a_sq = accelX[idx] * accelX[idx] +
        accelY[idx] * accelY[idx] +
        accelZ[idx] * accelZ[idx];
    constexpr float ref = 15.0f * 15.0f;
    float raw = std::min(a_sq / ref, 1.0f);
    smoothedIntensity[idx] = 0.6f * smoothedIntensity[idx] + 0.4f * raw;
    return smoothedIntensity[idx];
}

void PhysicsWorld::GetMomentum(int idx, float& mx, float& my, float& mz) const noexcept
{
    mx = mass[idx] * uX[idx];
    my = mass[idx] * uY[idx];
    mz = mass[idx] * uZ[idx];
}

void PhysicsWorld::GetVelocity(int idx, float& vx, float& vy, float& vz) const noexcept
{
    float inv_g = 1.0f / gamma[idx];
    vx = uX[idx] * inv_g;
    vy = uY[idx] * inv_g;
    vz = uZ[idx] * inv_g;
}

int PhysicsWorld::TempAdd(float x, float y, float z)
{
    tempPosX.push_back(x);
    tempPosY.push_back(y);
    tempPosZ.push_back(z);
    return (int)tempPosX.size() - 1;
}

void PhysicsWorld::TempClear()
{
    tempPosX.clear();
    tempPosY.clear();
    tempPosZ.clear();
}