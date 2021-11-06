#pragma once

#include <vector>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

class AISystem
{
public:
	void step(float elapsed_ms);

	void maybeDrawDeltaBox(Motion* m_salmonMotion, float* m_distance);

	const float FISH_DELTA_DISTANCE = 200;	

	int frameCounter = 0;

	int internalFrameCounter = -1;
};