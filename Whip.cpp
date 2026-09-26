#include "Whip.h"
#include <cmath>

void Whip::Init(PhysicsWorld& world, int trailCount)
{
    trailIndices.clear();
    for (int i = 0; i < trailCount; i++)
    {
        int idx = world.TempAdd(0.0f, -10000.0f, 0.0f);
        trailIndices.push_back(idx);
    }
}

void Whip::Trigger()
{
    active = true;
    animTime = 0.0f;
}


//The math for this was derived from the paper "Shape of a Cracking Whip" by Tyler McMillen and Alain Goriely
void Whip::EvaluateLoopShape(float s, float xi, float& localX, float& localY) const
{
    //parameter to control how wide the loop effects. 
    float arg = xi / loopHeight;

    //Controls compression (I wont be using this)
    localX =  - 2.0f * loopHeight * std::tanh(arg);

    //Affects how tall the loop is. 
    // the maxima of localY is when xi = 0 showing that the loop is the tallest when its directly on the sphere
    localY = 2.0f * loopHeight / std::cosh(arg); 
}

void Whip::Update(PhysicsWorld& world, float dt)
{
    if (!active)
    {
        for (int idx : trailIndices)
        {
            world.tempPosX[idx] = 0.0f;
            world.tempPosY[idx] = -10000.0f;
            world.tempPosZ[idx] = 0.0f;
        }
        return;
    }

    animTime += dt;
    float loopPosition;
    for (size_t i = 0; i < trailIndices.size(); i++)
    {
        //distance of each sphere from the initial sphere
        float s = ((float)i / (float)(trailIndices.size() - 1)) * whipLength; 
        loopPosition = loopSpeed * animTime + accelerationFactor * animTime * animTime; 

        //Tells if the loop is ahead or behind (+ve is behind, -ve is ahead)
        float xi = s - loopPosition; 

        // A lower blendwidth means the blend is more compact
        float blendWidth = whipLength * 0.3f; 

        //blend is used to interpolate. Here the range is [0,1]
        float blend = 0.5f * (1.0f + std::tanh(-xi / blendWidth)); 

        //initial 2D coordinates
        float straightX = s; //X-Axis is parallel to the whip
        float straightY = 0.0f; //Y-Axos is perpendicular to the whip
 
        //This variable deals with overshootangle - sphere location relationship
        //ensures that spheres farther away have a larger overshoot. 
        //simulates the fact that whips gets thinner and hence overshoot more.
        float overshootFactor = s / whipLength;

        //This variable deals with sphere-loop relationship.
        //Ensures that the overshoot occurs when the loop is near the sphere.
        /* A parabola is used since it returns 0 when the-
        blend is either 0 or 1 and its maxima lies at 1/2 (blend=0.5 occurs when xi=0) */
        float overshootEnvelope = 4.0f * blend * (1.0f - blend);

        //Multiplication ensures that all conditons must be present (AND operator in a way)
        float overshootAngle = tipOvershootAngle * overshootFactor * overshootEnvelope;

        //This is the polar coordinate of the final whip position in a 2D plane (rcos(theta),rsin(theta)
        float overshootX = s * std::cos(finalRotationAngle + overshootAngle);
        float overshootY = s * std::sin(finalRotationAngle + overshootAngle);

        //overshootX&Y is the final position. 
        //baseX&Y is the interpolated part 
        //baseX = s, when blend =0 (initial position)
        //When blend = 1/2 the sphere reaches maximum overshoot angle
        //baseX = overshootX, when blend = 1 (final position)
        float baseX = straightX * (1.0f - blend) + overshootX * blend;
        float baseY = straightY * (1.0f - blend) + overshootY * blend; 

        float loopLocalX, loopLocalY;
        EvaluateLoopShape(s, xi, loopLocalX, loopLocalY);
        //float compressOffset = loopLocalX;
        float compressOffset = 0;
        float bulgeOffset = loopLocalY;

        float localX = baseX + compressOffset;
        float localY = baseY + bulgeOffset;


        float worldX = handleX + localX * dirX + localY * perpX;
        float worldY = handleY + localX * dirY + localY * perpY;
        float worldZ = handleZ + localX * dirZ + localY * perpZ;

        world.tempPosX[trailIndices[i]] = worldX;
        world.tempPosY[trailIndices[i]] = worldY;
        world.tempPosZ[trailIndices[i]] = worldZ;
    }

    if (loopPosition - 1.5f > whipLength + loopHeight * 3.0f)
    {
        active = false;
        animTime = 0.0f;
    }

}

void Whip::GetCurrentPosition(float& x, float& y, float& z) const
{
    float s = std::min(loopSpeed * animTime, whipLength);
    if (s < 0.0f) s = 0.0f;
    float localX, localY;
    EvaluateLoopShape(s, 0.0f, localX, localY);

    x = handleX + localX * dirX + localY * perpX;
    y = handleY + localX * dirY + localY * perpY;
    z = handleZ + localX * dirZ + localY * perpZ;
}

void Whip::ApplyPolarizabilityBoost(PhysicsWorld& world, float radius)
{
    if (!active) return;

    float wx, wy, wz;
    GetCurrentPosition(wx, wy, wz);
    float radius_sq = radius * radius;

    for (int i = 0; i < world.count; i++)
    {
        if (world.charge[i] != 0) continue;

        float dx = world.posX[i] - wx;
        float dy = world.posY[i] - wy;
        float dz = world.posZ[i] - wz;
        float dist_sq = dx * dx + dy * dy + dz * dz;

        if (dist_sq < radius_sq)
            world.polarizability[i] = 0.05f;
    }
}

void Whip::UpdateAim(float startX, float startY, float startZ,
    float directionX, float directionY, float directionZ,
    float rx, float ry, float rz)
{
    handleX = startX; handleY = startY; handleZ = startZ;

    float mag = std::sqrt(directionX * directionX + directionY * directionY + directionZ * directionZ);
    if (mag < 1e-6f) mag = 1.0f;
    dirX = directionX / mag;
    dirY = directionY / mag;
    dirZ = directionZ / mag;

    perpX = -rx;
    perpY = -ry;
    perpZ = -rz;
    float pmag = std::sqrt(perpX * perpX + perpY * perpY + perpZ * perpZ);
    if (pmag < 1e-6f) pmag = 1.0f;
    perpX /= pmag; perpY /= pmag; perpZ /= pmag;
}


