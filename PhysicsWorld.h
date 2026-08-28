#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "PhysConstants.h"

struct PhysicsWorld
{
    // --- POSITION ---
    std::vector<float> posX, posY, posZ;

    // --- MOMENTUM (u = γv) ---
    std::vector<float> uX, uY, uZ;

    // --- DERIVED ---
    std::vector<float> gamma;
    std::vector<float> accelX, accelY, accelZ;
    std::vector<float> smoothedIntensity;

    // --- PROPERTIES ---
    std::vector<float> mass;
    std::vector<int>   charge;

    // --- FORCE ACCUMULATORS ---
    std::vector<float> forceX, forceY, forceZ;
    std::vector<float> E_FieldX, E_FieldY, E_FieldZ;
    std::vector<float> M_FieldX, M_FieldY, M_FieldZ;
    std::vector<float> eLocalX, eLocalY, eLocalZ;
    std::vector<float> eTotalX, eTotalY, eTotalZ;

    int count = 0;

    int Add(float x, float y, float z,
        float ux, float uy, float uz,
        int q, float m = 1.0f);

    void Clear();
    void ResetAccumulators();
    void AddEtotalAll();
    void StepAllBoris(float dt) noexcept;
    float GetRadiationIntensity(int idx) noexcept;
    void GetMomentum(int idx, float& mx, float& my, float& mz) const noexcept;
    void GetVelocity(int idx, float& vx, float& vy, float& vz) const noexcept;
};