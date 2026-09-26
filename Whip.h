#pragma once
#include "PhysicsWorld.h"
#include <vector>

class Whip
{
public:
    void Init(PhysicsWorld& world, int trailCount = 12);
    void Trigger();
    void UpdateAim(float startX, float startY, float startZ,
      float dirX, float dirY, float dirZ,
      float rx, float ry, float rz);

    void Update(PhysicsWorld& world, float dt);
    void ApplyPolarizabilityBoost(PhysicsWorld& world, float radius);

    bool IsActive() const { return active; }
    void GetCurrentPosition(float& x, float& y, float& z) const;

    const std::vector<int>& GetTrailIndices() const { return trailIndices; }

private:
    void EvaluateLoopShape(float s, float xi, float& localX, float& localY) const;

    bool active = false;
    float animTime = 0.0f;

    float loopHeight = 0.4f;
    float loopSpeed = 8.0f;
    float whipLength = 7.0f;

    float handleX = 0.0f, handleY = 0.0f, handleZ = 0.0f;
    float dirX = 1.0f, dirY = 0.0f, dirZ = 0.0f;
    float perpX = 0.0f, perpY = 1.0f, perpZ = 0.0f;

    float finalRotationAngle = 1.0f*0.5236f;  // 30 degrees in radians
    float tipOvershootAngle = 2.0f * 0.5236f;
    float accelerationFactor = 2.4f;

    std::vector<int> trailIndices;
};