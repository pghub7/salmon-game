// internal
#include "physics_system.hpp"
#include "world_init.hpp"

namespace {
	vec2 computeCollisionVelocity(Entity entity, Entity entity_other) {
		auto& motion_entity = registry.motions.get(entity);
		auto& motion_entity_other = registry.motions.get(entity_other);
		auto& physics_entity = registry.physics.get(entity);
		auto& physics_entity_other = registry.physics.get(entity_other);

		float distSquared = (float)pow(motion_entity.position.x - motion_entity_other.position.x, 2) + (float)pow(motion_entity.position.y - motion_entity_other.position.y, 2);

		vec2 num = (motion_entity.velocity - motion_entity_other.velocity) * (motion_entity.position - motion_entity_other.position);
		num = num / distSquared;
		num = num * (motion_entity.position - motion_entity_other.position);
		float coeff = (2.f * physics_entity_other.mass) / (physics_entity.mass + physics_entity_other.mass);

		vec2 new_velocity = motion_entity.velocity - (coeff * num);

		if (physics_entity.affectedByGravity) {
			// new_velocity *= -1.f;
		}

		return new_velocity;
	}
}

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	//vec2 dp = motion1.position - motion2.position;
	//float dist_squared = dot(dp,dp);
	//const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	//const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	//const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	//const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	//const float r_squared = max(other_r_squared, my_r_squared);
	//if (dist_squared < r_squared)
	//	return true;
	//return false;
	vec2 bounding_box_1 = get_bounding_box(motion1);
	vec2 bounding_box_2 = get_bounding_box(motion2);
	float radius1 = sqrt(dot(bounding_box_1 / 2.f, bounding_box_1 / 2.f));
	float radius2 = sqrt(dot(bounding_box_2 / 2.f, bounding_box_2 / 2.f));

	// calc distance between two circles
	float distX = motion1.position.x - motion2.position.x;
	float distY = motion1.position.y - motion2.position.y;
	float distance = sqrt((distX * distX) + (distY * distY));

	if (distance <= radius1 + radius2) {
		return true;
	}
	return false;
}

bool checkPreciseCollisionWithSalmon(const Motion& salmonMotion, const Motion& motion2)
{
	vec2 bounding_box_non_salmon = get_bounding_box(motion2);
	float rx = motion2.position.x - bounding_box_non_salmon.x / 2.;
	float ry = motion2.position.y - bounding_box_non_salmon.y / 2.;
	float rw = bounding_box_non_salmon.x;
	float rh = bounding_box_non_salmon.y;

	auto& mesh = registry.meshPtrs.get(registry.players.entities[0]);
	for (auto& vertex : mesh->vertices) {
		Transform transform;
		transform.translate(salmonMotion.position);
		transform.rotate(salmonMotion.angle);
		transform.scale(salmonMotion.scale);

		vec3 p = transform.mat * vec3(vertex.position.x, vertex.position.y, 1.0);
		if (p.x >= rx &&         // right of the left edge AND
			p.x <= rx + rw &&    // left of the right edge AND
			p.y >= ry &&         // below the top AND
			p.y <= ry + rh) {    // above the bottom
			return true;
		}
	}
	return false;
}


void PhysicsSystem::step(float elapsed_ms, float window_width_px, float window_height_px)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	for(uint i = 0; i< motion_registry.size(); i++)
	{
		// !!! TODO A1: update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];

		if (registry.players.has(entity) && registry.players.get(entity).collidesWithTopWall == true) {
			// printf("inside detection statement\n");
			vec2 bonding_box_i = get_bounding_box(motion);
			float radius = sqrt(dot(bonding_box_i / 2.f, bonding_box_i / 2.f));
			motion.position.y = radius;
		} 
		else if (registry.players.has(entity) && registry.players.get(entity).collidesWithBottomWall == true) {
			// printf("inside detection statement\n");
			vec2 bonding_box_i = get_bounding_box(motion);
			float radius = sqrt(dot(bonding_box_i / 2.f, bonding_box_i / 2.f));
			motion.position.y = window_height_px - radius;
		}
		else {
		float step_seconds = 1.0f * (elapsed_ms / 1000.f);
		if (registry.physics.has(entity)) {
			auto& physics = registry.physics.get(entity);
			if (physics.affectedByGravity) {
				motion.velocity.y += physics.gravityAccel * elapsed_ms;
			}
		}
		motion.position.x = motion.position.x + (step_seconds * motion.velocity.x);
		motion.position.y = motion.position.y + (step_seconds * motion.velocity.y);
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	// Check for collisions between all moving entities
    ComponentContainer<Motion> &motion_container = registry.motions;
	for(uint i = 0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];

		// check for fish collisions with the walls
		if (registry.softShells.has(entity_i)) {
			vec2 bounding_box_i = get_bounding_box(motion_i);
			float radius = sqrt(dot(bounding_box_i / 2.f, bounding_box_i / 2.f));
			float upperEdgeY = motion_i.position.y - (radius / 2.);
			float bottomEdgeY = motion_i.position.y + (radius / 2.);

			if (upperEdgeY < 0.2) {
				// printf("fish collides with upper wall\n");
				motion_i.velocity.y *= -1;
			}
			if (bottomEdgeY >= window_height_px - 0.2) {
				// printf("fish collides with below wall\n");
				motion_i.velocity.y *= -1;
			}
		}

		for(uint j = 0; j<motion_container.components.size(); j++) // i+1
		{
			if (i == j)
				continue;

			Motion& motion_j = motion_container.components[j];	
			if (collides(motion_i, motion_j)) {
				Entity entity_j = motion_container.entities[j];
				
				// handle collisions involing pebbles
				if (registry.physics.has(entity_i) && registry.physics.has(entity_j)) {
					vec2 new_vel_entity_i = computeCollisionVelocity(entity_i, entity_j);					
					float step_seconds = 1.0f * (elapsed_ms / 1000.f);
					motion_i.velocity = new_vel_entity_i;
				}
				// check for precise collisions for salmon
				Entity* salmon = nullptr;
				Entity* other = nullptr;
				if (registry.players.has(entity_i)) {
					salmon = &entity_i;
					other = &entity_j;
				} else if (registry.players.has(entity_j)) {
					salmon = &entity_j;
					other = &entity_i;
				}
				
				if (salmon && registry.physics.has(*other) && registry.physics.get(*other).affectedByGravity == true) {
					break;
					// ignore
				}
				if (salmon && checkPreciseCollisionWithSalmon(registry.motions.get(*salmon), registry.motions.get(*other))) {
					registry.collisions.emplace_with_duplicates(entity_i, entity_j);
					registry.collisions.emplace_with_duplicates(entity_j, entity_i);
					break;
				}
				 // Create a collisions event
				 // We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				if (!salmon) {
					registry.collisions.emplace_with_duplicates(entity_i, entity_j);
					registry.collisions.emplace_with_duplicates(entity_j, entity_i);
				}
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE SALMON - WALL collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	vec2 upperWallCoord1 = { 0, 0 };
	vec2 upperWallCoord2 = { 0, window_width_px };

	//check bounding box collisions with wall
	if (registry.players.components.size() > 0) {
		registry.players.components.at(0).collidesWithTopWall = false;
		registry.players.components.at(0).collidesWithBottomWall = false;
		bool checkNarrowPhase = false;
		Motion& salmon_motion = registry.motions.get(registry.players.entities[0]);
		vec2 bonding_box_i = get_bounding_box(salmon_motion);
		float radius = sqrt(dot(bonding_box_i / 2.f, bonding_box_i / 2.f));
		vec2 upperRightCorner = { salmon_motion.position.x + radius, salmon_motion.position.y - radius };
		vec2 bottomLeftCorner = { salmon_motion.position.x - radius, salmon_motion.position.y + radius };
		if (upperRightCorner.y < 0.5) {
			checkNarrowPhase = true;
		}
		if (bottomLeftCorner.y >= window_height_px - 0.5) {
			checkNarrowPhase = true;
		}

		if (checkNarrowPhase == true) {
			Motion& salmon_motion = registry.motions.get(registry.players.entities[0]);
			auto& mesh = registry.meshPtrs.get(registry.players.entities[0]);
			for (auto& vertex : mesh->vertices) {
				Transform transform;
				transform.translate(salmon_motion.position);
				transform.rotate(salmon_motion.angle);
				transform.scale(salmon_motion.scale);

				vec3 world_coord = transform.mat * vec3(vertex.position.x, vertex.position.y, 1.0);
				if (world_coord.y <= 0.2) {
					registry.players.components.at(0).collidesWithTopWall = true;
					// printf("exact collision detected with top wall for salmon\n");
					break;
				}
				if (world_coord.y >= window_height_px - 0.2) {
					registry.players.components.at(0).collidesWithBottomWall = true;
					// printf("exact collision detected with bottom wall for salmon\n");
					break;
				}
			}
		}
	}
	
	// you may need the following quantities to compute wall positions
	(float)window_width_px; (float)window_height_px;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: DRAW DEBUG INFO HERE on Salmon mesh collision
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will want to use the createLine from world_init.hpp
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// debugging of bounding boxes
	if (debugging.in_debug_mode)
	{
		uint size_before_adding_new = (uint)motion_container.components.size();
		for (uint i = 0; i < size_before_adding_new; i++)
		{
			Motion& motion_i = motion_container.components[i];
			Entity entity_i = motion_container.entities[i];

			if (registry.debugComponents.has(entity_i)) {
				continue;
			}

			// visualize the radius with two axis-aligned lines
			const vec2 bonding_box = get_bounding_box(motion_i);
			float radius = sqrt(dot(bonding_box/2.f, bonding_box/2.f));

			// Entity lowerLeftCorner = createLine({ motion_i.position.x - radius, motion_i.position.y + radius }, { 15, 15 });
			// Entity upperRightCorner = createLine({ motion_i.position.x + radius, motion_i.position.y - radius }, { 15, 15 });

			
			vec2 line_scale1 = { motion_i.scale.x / 20, 2*radius };
			// Entity line1 = createLine(motion_i.position, line_scale1);
			
			vec2 line_scale2 = { 2*radius, motion_i.scale.x / 20};
			// Entity line2 = createLine(motion_i.position, line_scale2);

			Entity line3 = createLine({ motion_i.position.x, motion_i.position.y + radius }, line_scale2);

			Entity line4 = createLine({ motion_i.position.x, motion_i.position.y - radius }, line_scale2);

			Entity line5 = createLine({ motion_i.position.x + radius, motion_i.position.y }, line_scale1);

			Entity line6 = createLine({ motion_i.position.x - radius, motion_i.position.y }, line_scale1);
			// !!! TODO A2: implement debugging of bounding boxes and mesh

			if (registry.meshPtrs.has(entity_i)) {
				auto& mesh = registry.meshPtrs.get(entity_i);
				for (auto& vertex : mesh->vertices) {
					Transform transform;
					transform.translate(motion_i.position);
					transform.rotate(motion_i.angle);
					transform.scale(motion_i.scale);

					vec3 world_coord =  transform.mat * vec3(vertex.position.x, vertex.position.y, 1.0);
					// printf("vertices: %f, %f", vertex.position.x, vertex.position.y);
					Entity debugPoint = createLine({ world_coord.x, world_coord.y }, { 6, 6});
				}
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}