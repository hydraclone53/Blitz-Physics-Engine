#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "PhysConstants.h"

struct PhysicsWorld
{
    // --- POSITION ---
    std::vector<float> posX, posY, posZ;
    std::vector<float> tempPosX, tempPosY, tempPosZ;

    int TempAdd(float x, float y, float z);
    void TempClear();

    // --- MOMENTUM (u = γv) ---
    std::vector<float> uX, uY, uZ;

    // --- DERIVED ---
    std::vector<float> gamma;
    std::vector<float> accelX, accelY, accelZ;
    std::vector<float> prevAccelX, prevAccelY, prevAccelZ;
    std::vector<float> smoothedIntensity;

    // --- PROPERTIES ---
    std::vector<float> mass;
    std::vector<int>   charge;
    std::vector<float> polarizability;

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
    void GetKE(int idx, float& k) const noexcept;
    void GetVelocity(int idx, float& vx, float& vy, float& vz) const noexcept;

    // --- TESTING RETARDATION FIELDS ---
    struct history
    {
        std::vector<double> r_time;
        std::vector<float> r_wX;
        std::vector<float> r_wY;
        std::vector<float> r_wZ;
        std::vector<float> r_vX;
        std::vector<float> r_vY;
        std::vector<float> r_vZ;
        std::vector<float> r_aX;
        std::vector<float> r_aY;
        std::vector<float> r_aZ;
    };
    std::vector<history> retar;
    double total_time = 0.0;
};