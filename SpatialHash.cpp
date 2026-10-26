#include "SpatialHash.h"
#include <cmath> 
#include <chrono>

// Vers: 1.unordered
/*size_t SpatialHash::Hash(int cx, int cy, int cz) const
{
   return ((size_t)cx * 73856093u) ^
        ((size_t)cy * 19349663u) ^
        ((size_t)cz * 83492791u);
}

void SpatialHash::GetCell(PhysicsBody* body, int& cx, int& cy, int& cz) const
{
    cx = (int)std::floor(body->GetX() / cellsize);
    cy = (int)std::floor(body->GetY() / cellsize);
    cz = (int)std::floor(body->GetZ() / cellsize);
}

SpatialHash::SpatialHash(float cellsize) noexcept : cellsize(cellsize)
{
}

void SpatialHash::BuildHash(std::vector<PhysicsBody*>& bodies)
{
    table.clear();
    for (PhysicsBody* b : bodies)
    {
        int cx, cy, cz;
        GetCell(b, cx, cy, cz);
        table[Hash(cx, cy, cz)].push_back(b);
    }
}

void SpatialHash::QueryNeighbors(PhysicsBody* body, std::vector<PhysicsBody*>& result)
{
    result.clear();
    int cx, cy, cz;
    GetCell(body, cx, cy, cz);

    std::vector<PhysicsBody*> neighbors;
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dz = -1; dz <= 1; dz++)
            {
                auto it = table.find(Hash(cx + dx, cy + dy, cz + dz));
                if (it != table.end())
                    for (PhysicsBody* b : it->second)
                        if (b != body)
                            result.push_back(b);
            }
} 
*/

// Vers: 2.array

int SpatialHash::Hash(int cx, int cy, int cz) const
{
    size_t h = ((size_t)cx * 73856093u) ^
        ((size_t)cy * 19349663u) ^
        ((size_t)cz * 83492791u);
    return (int)(h & (size_t)(tableSize - 1));
}
/*
void SpatialHash::GetCell(PhysicsBody* body, int& cx, int& cy, int& cz) const
{
    // faster than std::floor for typical position values
    cx = (int)(body->GetX() / cellSize);
    cy = (int)(body->GetY() / cellSize);
    cz = (int)(body->GetZ() / cellSize);

    // correct for negative positions — (int) truncates toward zero, not negative infinity
    if (body->GetX() < 0) cx--;
    if (body->GetY() < 0) cy--;
    if (body->GetZ() < 0) cz--;
}
void SpatialHash::BuildHash(std::vector<PhysicsBody*>& bodies)
{

    auto t0 = std::chrono::high_resolution_clock::now();
    std::fill(buckets.begin(), buckets.end(), -1);

    auto t1 = std::chrono::high_resolution_clock::now();
    entries.clear();
    entries.reserve(bodies.size());

    for (PhysicsBody* b : bodies)
    {
        int cx, cy, cz; 
        GetCell(b, cx, cy, cz);
        int h = Hash(cx, cy, cz);

        entries.push_back({ b, cx, cy, cz, buckets[h] });
        buckets[h] = (int)entries.size() - 1;

    }

    auto t2 = std::chrono::high_resolution_clock::now();
    lastFillMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    lastInsertMs = std::chrono::duration<float, std::milli>(t2 - t1).count();
    lastCapacity = (int)entries.capacity();
} 

void SpatialHash::QueryNeighbors(PhysicsBody* body, std::vector<PhysicsBody*>& result)
{
    result.clear();

    int cx, cy, cz;
   GetCell(body, cx, cy, cz);

    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dz = -1; dz <= 1; dz++)
            {
                int ncx = cx + dx;
                int ncy = cy + dy;
                int ncz = cz + dz;
                int h = Hash(ncx, ncy, ncz);

                int idx = buckets[h];
                while (idx != -1)
                {
                    Entry& e = entries[idx];
                    if (e.cx == ncx && e.cy == ncy && e.cz == ncz)
                        if (e.body != body)
                            result.push_back(e.body);
                    idx = e.next;
                }
            }
}
*/

void SpatialHash::GetCell(int idx, const std::vector<float>& posX,
    const std::vector<float>& posY,
    const std::vector<float>& posZ,
    int& cx, int& cy, int& cz) const
{
    float x = posX[idx];
    float y = posY[idx];
    float z = posZ[idx];
    cx = (int)(x / cellSize); if (x < 0) cx--;
    cy = (int)(y / cellSize); if (y < 0) cy--;
    cz = (int)(z / cellSize); if (z < 0) cz--;
}

void SpatialHash::BuildHash(const float* posX, const float* posY,
    const float* posZ, int count)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    // reset buckets
    std::fill(buckets.begin(), buckets.end(), -1);
    std::fill(cellCount.begin(), cellCount.end(), 0);
    std::fill(cellPos.begin(), cellPos.end(), 0);
    entries.resize(count);
    sortedEntries.resize(count);

    auto t1 = std::chrono::high_resolution_clock::now();

    // loop 1 — insert all entries
    for (int i = 0; i < count; i++)
    {
        float x = posX[i], y = posY[i], z = posZ[i];
        int cx = (int)(x / cellSize); if (x < 0) cx--;
        int cy = (int)(y / cellSize); if (y < 0) cy--;
        int cz = (int)(z / cellSize); if (z < 0) cz--;
        int h = Hash(cx, cy, cz);
        entries[i] = { i, cx, cy, cz, buckets[h] };
        buckets[h] = i;
        cellCount[h]++;
    }

    // loop 2 — prefix sum for cellStart
    cellStart[0] = 0;
    for (int i = 1; i < tableSize; i++)
        cellStart[i] = cellStart[i - 1] + cellCount[i - 1];

    // loop 3 — sort entries into contiguous cell groups
    for (int i = 0; i < count; i++)
    {
        int h = Hash(entries[i].cx, entries[i].cy, entries[i].cz);
        sortedEntries[cellStart[h] + cellPos[h]] = entries[i];
        cellPos[h]++;
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    // loop 4 — build pairs sequentially
    pairs.clear();
    for (int i = 0; i < count; i++)
    {
        const Entry& self = sortedEntries[i];
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
                for (int dz = -1; dz <= 1; dz++)
                {
                    int ncx = self.cx + dx;
                    int ncy = self.cy + dy;
                    int ncz = self.cz + dz;
                    int h = Hash(ncx, ncy, ncz);

                    int start = cellStart[h];
                    int end = start + cellCount[h];
                    for (int j = start; j < end; j++)
                    {
                        if (sortedEntries[j].cx == ncx &&
                            sortedEntries[j].cy == ncy &&
                            sortedEntries[j].cz == ncz &&
                            sortedEntries[j].bodyIndex > self.bodyIndex)
                        {
                            pairs.push_back({ self.bodyIndex, sortedEntries[j].bodyIndex });
                        }
                    }
                }
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    lastFillMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    lastInsertMs = std::chrono::duration<float, std::milli>(t2 - t1).count();
    lastPairMs = std::chrono::duration<float, std::milli>(t3 - t2).count();
    lastCapacity = (int)entries.capacity();
}

void SpatialHash::QueryNeighbors(int bodyIdx, std::vector<int>& result)
{
    result.clear();

    // cell coords already stored during BuildHash
    const Entry& self = entries[bodyIdx];
    int cx = self.cx, cy = self.cy, cz = self.cz;

    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dz = -1; dz <= 1; dz++)
            {
                int ncx = cx + dx, ncy = cy + dy, ncz = cz + dz;
                int h = Hash(ncx, ncy, ncz);
                int idx = buckets[h];
                while (idx != -1)
                {
                    Entry& e = entries[idx];
                    if (e.cx == ncx && e.cy == ncy && e.cz == ncz && e.bodyIndex != bodyIdx)
                        result.push_back(e.bodyIndex);
                    idx = e.next;
                }
            }
}

// SpatialHash.cpp
void SpatialHash::BuildHashNoPairs(const float* posX, const float* posY,
    const float* posZ, int count)
{
    std::fill(buckets.begin(), buckets.end(), -1);
    entries.resize(count);

    for (int i = 0; i < count; i++)
    {
        float x = posX[i], y = posY[i], z = posZ[i];
        int cx = (int)(x / cellSize); if (x < 0) cx--;
        int cy = (int)(y / cellSize); if (y < 0) cy--;
        int cz = (int)(z / cellSize); if (z < 0) cz--;
        int h = Hash(cx, cy, cz);
        entries[i] = { i, cx, cy, cz, buckets[h] };
        buckets[h] = i;
    }

}
