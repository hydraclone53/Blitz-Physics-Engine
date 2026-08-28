#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>
#include "PhysicsBody.h"

// Using unorderedmap
/*class SpatialHash
{
public:
	SpatialHash(float cellsize) noexcept;
	~SpatialHash() = default;
	SpatialHash() = default;
	void BuildHash(std::vector<PhysicsBody*>& bodies);
	void QueryNeighbors(PhysicsBody* body, std::vector<PhysicsBody*>& result);
private:
	std::unordered_map<size_t, std::vector<PhysicsBody*>> table;  // member here

	size_t Hash(int cx, int cy, int cz) const;
	void GetCell(PhysicsBody* body, int& cx, int& cy, int& cz) const;
	float cellsize;
};
*/ 


// Using flat array
class SpatialHash {
public:
	int lastCapacity = 0;
	float lastFillMs = 0.0f;
	float lastInsertMs = 0.0f;
	float lastPairMs = 0.0f;
	SpatialHash() : cellSize(1.0f), tableSize(16384) { Init(); }
	SpatialHash(float cellSize, int tableSize)
		: cellSize(cellSize), tableSize(tableSize) {
		Init();
	}
	void BuildHash(const float* posX, const float* posY,
		const float* posZ, int count);
	void QueryNeighbors(int bodyIdx, std::vector<int>& result);
	void BuildHashNoPairs(const float* posX, const float* posY,
		const float* posZ, int count);
	//void BuildHash(std::vector<PhysicsBody*>& bodies);
	//void QueryNeighbors(PhysicsBody* body, std::vector<PhysicsBody*>& result);
	struct NeighborPair { int a, b; };
	std::vector<NeighborPair> pairs;
private:
	float cellSize;
	int tableSize;


	struct Entry
	{
		int bodyIndex = -1;  // index instead of pointer
		int cx, cy, cz;
		int next = -1;
	};
	// SpatialHash.h private members
	std::vector<int>   cellCount;
	std::vector<int>   cellStart;
	std::vector<int>   cellPos;
	std::vector<Entry> sortedEntries;

	std::vector<int>   buckets;
	std::vector<Entry> entries;

	void Init()
	{
		buckets.resize(tableSize, -1);
		entries.reserve(10000);
		cellCount.resize(tableSize, 0);
		cellStart.resize(tableSize, 0);
		cellPos.resize(tableSize, 0);
		sortedEntries.reserve(10000);
		pairs.reserve(10000);
	}

	int Hash(int cx, int cy, int cz) const;
	void GetCell(int idx, const std::vector<float>& posX,
		const std::vector<float>& posY,
		const std::vector<float>& posZ,
		int& cx, int& cy, int& cz) const;
	// void GetCell(PhysicsBody* body, int& cx, int& cy, int& cz) const;
};