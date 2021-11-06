// internal
#include "ai_system.hpp"
#include "world_init.hpp"

void AISystem::step(float elapsed_ms)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE FISH AI HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will likely want to write new functions and need to create
	// new data structures to implement a more sophisticated Fish AI.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// if (internalFrameCounter % frameCounter == 0) {
	if (registry.players.components.size() > 0) {
		Motion& salmonMotion = registry.motions.get(registry.players.entities[0]);
		for (auto& entity : registry.softShells.entities) {
			Motion& fishMotion = registry.motions.get(entity);
			vec2 d = salmonMotion.position - fishMotion.position;
			float distance = sqrt(dot(d, d));
			if (distance < AISystem::FISH_DELTA_DISTANCE) {
				registry.softShells.get(entity).inDeltaRange = true;
				// printf("fish and salmon have reached delta stage\n");
				float angle = (180. / M_PI) * atan2(fishMotion.position.y - salmonMotion.position.y, fishMotion.position.x - salmonMotion.position.x);
				// printf("angle: %f\n", angle);
				debugging.in_freeze_mode = true;
				if (angle < 0) {
					fishMotion.velocity.y = -180;
				}
				else {
					// fishMotion.velocity = { 0, 150 };
					fishMotion.velocity.y = 180;
				}
				AISystem::maybeDrawDeltaBox(&salmonMotion, &distance);
			}
			else {
				 if (registry.softShells.get(entity).inDeltaRange) {
				 fishMotion.velocity = { -200., 0. };
				 }
			}
		}
	}
	(void)elapsed_ms; // placeholder to silence unused warning until implemented
}


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO A2: DRAW DEBUG INFO HERE on AI path
// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
// You will want to use the createLine from world_init.hpp
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void AISystem::maybeDrawDeltaBox(Motion* m_salmonMotion, float* m_distance) {
	if (debugging.in_debug_mode && m_distance && m_salmonMotion) {
		vec2 line_scale_vert = { m_salmonMotion->scale.x / 20, *m_distance * 2};
		vec2 line_scale_hori = { *m_distance * 2, m_salmonMotion->scale.x / 20. };

		Entity line1 = createLine({ m_salmonMotion->position.x, m_salmonMotion->position.y - *m_distance }, line_scale_hori);
		Entity line2 = createLine({ m_salmonMotion->position.x, m_salmonMotion->position.y + *m_distance }, line_scale_hori);
		Entity line3 = createLine({ m_salmonMotion->position.x + *m_distance, m_salmonMotion->position.y }, line_scale_vert);
		Entity line4 = createLine({ m_salmonMotion->position.x - *m_distance, m_salmonMotion->position.y }, line_scale_vert);
	}
}