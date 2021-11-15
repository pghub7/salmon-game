// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <math.h>

#include "physics_system.hpp"

// Game configuration
const size_t MAX_TURTLES = 15;
const size_t MAX_FISH = 5;
const size_t TURTLE_DELAY_MS = 1000 * 3;
const size_t FISH_DELAY_MS = 1000 * 3;
const size_t MAX_VORTICES = 2;
const size_t VORTEX_DELAY = 5000 * 5;
const size_t PEBBLE_DELAY_MS = 1000 * 2;
const int NUM_DEATH_PARTICLES = 1000;

namespace {
	vec2 computeCollisionVelocity(Entity entity, Entity entity_other) {
		auto& motion_entity = registry.motions.get(entity);
		auto& motion_entity_other = registry.motions.get(entity_other);
		auto& physics_entity = registry.physics.get(entity);
		auto& physics_entity_other = registry.physics.get(entity_other);

		float distSquared = (float)pow(motion_entity.position.x - motion_entity_other.position.x, 2) + pow(motion_entity.position.y - motion_entity_other.position.y, 2);

		vec2 num = (motion_entity.velocity - motion_entity_other.velocity) * (motion_entity.position - motion_entity_other.position);
		num = num / distSquared;
		num = num * (motion_entity.position - motion_entity_other.position);
		float coeff = (2 * physics_entity_other.mass) / (physics_entity.mass + physics_entity_other.mass);

		vec2 new_velocity = motion_entity.velocity - (coeff * num);

		if (physics_entity.affectedByGravity) {
			// new_velocity *= -1.f;
		}

		return new_velocity;
	}
}

// Create the fish world
WorldSystem::WorldSystem()
	: points(0)
	, next_turtle_spawn(0.f)
	, next_fish_spawn(0.f)
	, next_vortex_spawn(5000.f)
	, next_pebble_spawn(0.f) {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (salmon_dead_sound != nullptr)
		Mix_FreeChunk(salmon_dead_sound);
	if (salmon_eat_sound != nullptr)
		Mix_FreeChunk(salmon_eat_sound);
	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window(int width, int height) {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(width, height, "Salmon Game Assignment", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	salmon_dead_sound = Mix_LoadWAV(audio_path("salmon_dead.wav").c_str());
	salmon_eat_sound = Mix_LoadWAV(audio_path("salmon_eat.wav").c_str());

	if (background_music == nullptr || salmon_dead_sound == nullptr || salmon_eat_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("music.wav").c_str(),
			audio_path("salmon_dead.wav").c_str(),
			audio_path("salmon_eat.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	// Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
    restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
	// Get the screen dimensions
	int screen_width, screen_height;
	glfwGetFramebufferSize(window, &screen_width, &screen_height);

	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
	    registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size()-1; i>=0; --i) {
	    Motion& motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
		    registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// Spawning new turtles
	next_turtle_spawn -= elapsed_ms_since_last_update * current_speed;
	if (registry.hardShells.components.size() <= MAX_TURTLES && next_turtle_spawn < 0.f) {
		// Reset timer
		next_turtle_spawn = (TURTLE_DELAY_MS / 2) + uniform_dist(rng) * (TURTLE_DELAY_MS / 2);
		// Create turtle
		Entity entity = createTurtle(renderer, {0,0});
		// Setting random initial position and constant velocity
		Motion& motion = registry.motions.get(entity);
		motion.position =
			vec2(screen_width + 50.f, 
				 50.f + uniform_dist(rng) * (screen_height - 100.f));
		motion.velocity = vec2(-100.f, 0.f);
	}

	// Spawning new fish
	next_fish_spawn -= elapsed_ms_since_last_update * current_speed;
	if (registry.softShells.components.size() <= MAX_FISH && next_fish_spawn < 0.f) {
		// Reset timer
		next_fish_spawn = (FISH_DELAY_MS / 2) + uniform_dist(rng) * (FISH_DELAY_MS / 2);
		// !!!  TODO A1: Create new fish with createFish({0,0}), as for the Turtles above
		Entity entity = createFish(renderer, { screen_width + 50.f, 50.f + uniform_dist(rng) * (screen_height - 100.f) });
		Motion& motion = registry.motions.get(entity);
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 gen(rd()); // seed the generator
		std::uniform_int_distribution<> distr(-200, 200); // define the range
		motion.velocity = vec2(-200.f, distr(gen));
	}

	// Spawn new Vortex
	if (registry.mode.has(player_salmon) && registry.mode.get(player_salmon).basicMode == false) {
		next_vortex_spawn -= elapsed_ms_since_last_update * current_speed;
		if (registry.pits.components.size() <= MAX_VORTICES && next_vortex_spawn < 0.f) {
			next_vortex_spawn = (VORTEX_DELAY / 2) + uniform_dist(rng) * (VORTEX_DELAY / 2);
			Entity entity = createVortex(renderer, { screen_width + 50.f, 50.f + uniform_dist(rng) * (screen_height - 100.f) });
		}

		// rotate Vortex
		for (auto& vortexEntity : registry.pits.entities) {
			auto& motion = registry.motions.get(vortexEntity);
			// motion.angle = motion.angle + ((90/ 360 ) * 2 * M_PI);
			motion.angle += 0.5;
			if (motion.angle >= (2 * M_PI)) {
				motion.angle = 0;
			}
		}

	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE SPAWN HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	next_pebble_spawn -= elapsed_ms_since_last_update * current_speed;
	if (next_pebble_spawn < 0.f) {
		// Reset timer
		next_pebble_spawn = (PEBBLE_DELAY_MS / 15) + uniform_dist(rng) * (PEBBLE_DELAY_MS / 15);
		// next_pebble_spawn = PEBBLE_DELAY_MS / 50;
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		float radius = 30 * (uniform_dist(rng) + 0.3f); // range 0.3 .. 1.3
		Entity pebble = createPebble({ uniform_dist(rng) * w, h - uniform_dist(rng) * 20 }, { radius, radius });
		auto& physicsComponent = registry.physics.get(pebble);
		physicsComponent.mass *=  0.1* radius;
		float brightness = uniform_dist(rng) * 0.5 + 0.5;
		registry.colors.insert(pebble, { brightness, brightness, brightness });
		auto& motion = registry.motions.get(pebble);
		motion.position = registry.motions.get(player_salmon).position;

		
		std::random_device rd; // obtain a random number from hardware
		std::random_device rd2;
		std::mt19937 gen(rd()); // seed the generator
		std::mt19937 gen2(rd2());
		std::uniform_int_distribution<> distr(150, 200); // define the range
		std::uniform_int_distribution<> distr2(120, 170); // define the range
		// float randNum = (float)((rand() % 50 - 10) * 7);
		int randNum1 = distr(gen);
		int randNum2 = distr2(gen2);
		motion.velocity = vec2(randNum1, randNum2);
		if (randNum1 % 2 == 0) {
			motion.velocity.x *= -1.;
		}
		if (randNum2 % 3 == 0) {
			motion.velocity.y *= -1.;
		}
		//else if (randNum2 > 40 && randNum2 < 50) {
		//	motion.velocity.x *= -1.;
		//}
	}


	// Processing the salmon state
	assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];
	
	float min_counter_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		// progress timer
		DeathTimer& counter = registry.deathTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if(counter.counter_ms < min_counter_ms){
		    min_counter_ms = counter.counter_ms;
		}

		// restart the game once the death timer expired
		if (counter.counter_ms < 0) {
			registry.deathTimers.remove(entity);
			screen.darken_screen_factor = 0;
            restart_game();
			return true;
		}
	}
	// reduce window brightness if any of the present salmons is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	// !!! TODO A1: update LightUp timers and remove if time drops below zero, similar to the death counter
	for (Entity entity : registry.lightUpTimers.entities) {
		LightUpTimer& counter = registry.lightUpTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;

		if (counter.counter_ms < 0) {
			registry.lightUpTimers.remove(entity);
		}
	}

	// update state of death particles
	for (Entity entity : registry.deathParticles.entities) {
		DeathParticle& deathParticles = registry.deathParticles.get(entity);
		for (int i = 0; i < deathParticles.deathParticles.size(); i++) {
			auto& particle = deathParticles.deathParticles[i];
			// for (auto& particle : deathParticles.deathParticles) {
			particle.Life -= elapsed_ms_since_last_update;
			// if (particle.Life > 0.f) {
			particle.motion.position.x -= particle.motion.velocity.y * (rand() % 17) * 0.3f;
			particle.motion.position.y -= particle.motion.velocity.x * (rand() % 17) * 0.3f;
			particle.Color.a -= 0.05f * 0.01f;
			particle.motion.angle += 0.5;
			if (particle.motion.angle >= (2 * M_PI)) {
				particle.motion.angle = 0;
			}
			deathParticles.positions[i * 4 + 0] = particle.motion.position.x;
			deathParticles.positions[i * 4 + 1] = particle.motion.position.y;
			deathParticles.positions[i * 4 + 2] = 1.;
			deathParticles.positions[i * 4 + 3] = particle.Life;
			// }
			if (particle.Life <= 0) {
				deathParticles.fadedParticles++;
			}
		}
		if (deathParticles.fadedParticles >= NUM_DEATH_PARTICLES - 5) {
			deathParticles.faded = true;
			registry.deathParticles.remove(entity);
		}
	}

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 0.4f;

	// Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

	// Debugging for memory/component leaks
	registry.list_all_components();

	// Create a new salmon
	player_salmon = createSalmon(renderer, { 100, 200 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});
	registry.mode.emplace(player_salmon);

	// !! TODO A3: Enable static pebbles on the ground
	// Create pebbles on the floor for reference
	//for (uint i = 0; i < 20; i++) {
	//	int w, h;
	//	glfwGetWindowSize(window, &w, &h);
	//	float radius = 30 * (uniform_dist(rng) + 0.3f); // range 0.3 .. 1.3
	//	Entity pebble = createPebble({ uniform_dist(rng) * w, h - uniform_dist(rng) * 20 }, 
	//		         { radius, radius });
	//	float brightness = uniform_dist(rng) * 0.5 + 0.5;
	//	registry.colors.insert(pebble, { brightness, brightness, brightness});
	//}
	
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions; // TODO: @Tim, is the reference here needed?
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;

		// For now, we are only interested in collisions that involve the salmon
		if (registry.players.has(entity)) {
			// Checking Player - HardShell collisions
			if (registry.hardShells.has(entity_other)) {
				// initiate death unless already dying
				if (!registry.deathTimers.has(entity)) {
					// Scream, reset timer, and make the salmon sink
					registry.deathTimers.emplace(entity);
					Mix_PlayChannel(-1, salmon_dead_sound, 0);
					registry.motions.get(entity).angle = 3.1415f;
					registry.motions.get(entity).velocity = { 0, 80 };

					// !!! TODO A1: change the salmon color on death
					registry.colors.get(entity) = { 1.f, 0.f, 0.f };
				}
			}
			// Checking Player - SoftShell collisions
			else if (registry.softShells.has(entity_other)) {
				if (!registry.deathTimers.has(entity)) {
					// chew, count points, and set the LightUp timer
					registry.remove_all_components_of(entity_other);
					Mix_PlayChannel(-1, salmon_eat_sound, 0);
					++points;

					// !!! TODO A1: create a new struct called LightUp in components.hpp and add an instance to the salmon entity by modifying the ECS registry
					registry.lightUpTimers.emplace(entity);
					
					if (!registry.deathParticles.has(entity)) {
						DeathParticle particleEffects;
						for (int p = 0; p < NUM_DEATH_PARTICLES; p++) {
							auto& motion = registry.motions.get(entity);
							DeathParticle particle;
							float random1 = ((rand() % 100) - 50) / 10.0f;
							float random2 = ((rand() % 200) - 100) / 10.0f;
							float rColor = 0.5f + ((rand() % 100) / 100.0f);
							// particle.motion.position = motion.position + random + vec2({ 20,20 });
							particle.motion.position.x = motion.position.x + random1 + 20.f;
							particle.motion.position.y = motion.position.y + random2 + 40.f;
							particle.Color = glm::vec4(rColor, rColor, rColor, 1.0f);
							particle.motion.velocity *= 0.1f;
							particle.motion.scale = vec2({ 10, 10 });
							particleEffects.deathParticles.push_back(particle);
							particleEffects.positions[p * 4 + 0] = particle.motion.position.x;
							particleEffects.positions[p * 4 + 1] = particle.motion.position.y;
							particleEffects.positions[p * 4 + 2] = 1.;
							particleEffects.positions[p * 4 + 3] = particle.Life;
						}
						registry.deathParticles.insert(entity, particleEffects);
					}
				}
			}
			// Checking player - vortex collisions
			else if (registry.pits.has(entity_other)) {
				if (!registry.deathTimers.has(entity)) {
					// DeathTimer t;
					// t.counter_ms = 1500;
					registry.deathTimers.emplace(entity);
					Mix_PlayChannel(-1, salmon_dead_sound, 0);
					registry.colors.get(entity) = { 1.f, 0.f, 0.f };
					registry.motions.get(entity).angle = 3.1415f;
					registry.motions.get(entity).velocity = { 0, 80 };
					playerDead = true;
					registry.motions.get(entity).position.x = registry.motions.get(entity_other).position.x;
					// restart_game();
				}
			}
		} else if (registry.deathTimers.entities.size() <= 0 )
			{
			if (registry.pits.has(entity)) {
				registry.remove_all_components_of(entity_other);
			}
			if (registry.pits.has(entity_other)) {
				registry.remove_all_components_of(entity);
			}
		}

		// handle collisions that have a physics component
		//if (registry.physics.has(entity) && registry.physics.has(entity_other)) {
		//	// printf("collision for physics components detected!!\n");

		//	vec2 new_vel_entity = computeCollisionVelocity(entity, entity_other);
		//	vec2 new_vel_entity_other = computeCollisionVelocity(entity_other, entity);

		//	auto& motion_entity = registry.motions.get(entity);
		//	motion_entity.velocity = new_vel_entity;
		//	motion_entity.position = motion_entity.position +
		//	
		//	auto& motion_entity_other = registry.motions.get(entity_other);
		//	motion_entity_other.velocity = new_vel_entity_other;


		//	if (registry.hardShells.has(entity)) {
		//		printf("velocity of pebble:%f, %f\n", motion_entity_other.velocity.x, motion_entity_other.velocity.y);
		//		printf("velocity of turtle:%f, %f\n", motion_entity.velocity.x, motion_entity.velocity.y);
		//	}
		//}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE SALMON MOVEMENT HERE
	// key is of 'type' GLFW_KEY_
	// action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

        restart_game();
	}

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	auto& salmon_motion = registry.motions.get(player_salmon);
	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;

		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	current_speed = fmax(0.f, current_speed);


	// Control salmon's linear motion with left/right directional keys
	if (!registry.deathTimers.has(player_salmon)) {
		if (action == GLFW_REPEAT && key == GLFW_KEY_LEFT) {
			salmon_motion.velocity.x = 10;
			salmon_motion.position.x -= salmon_motion.velocity.x;
		}
		if (action == GLFW_REPEAT && key == GLFW_KEY_RIGHT) {
			salmon_motion.velocity.x = 10;
			salmon_motion.position.x += salmon_motion.velocity.x;
		}
		// Control salmon's vertical motiom with up/down keys
		if (action == GLFW_REPEAT && key == GLFW_KEY_DOWN) {
			// salmon_motion.velocity.y = 10;
			// salmon_motion.position.y += salmon_motion.velocity.y;
			salmon_motion.velocity.y = 10 * sin(salmon_motion.angle);
			salmon_motion.velocity.x = 10 * cos(salmon_motion.angle);
			// salmon_motion.position.y += salmon_motion.velocity.y;
			salmon_motion.position = vec2(salmon_motion.position.x + salmon_motion.velocity.x, salmon_motion.position.y + salmon_motion.velocity.y);
		}
		if (action == GLFW_REPEAT && key == GLFW_KEY_UP) {
			salmon_motion.velocity.y = 10 * sin(salmon_motion.angle);
			salmon_motion.velocity.x = 10 * cos(salmon_motion.angle);
			// salmon_motion.position.y += salmon_motion.velocity.y;
			salmon_motion.position = vec2(salmon_motion.position.x + salmon_motion.velocity.x, salmon_motion.position.y + salmon_motion.velocity.y);
		}

		if (!registry.deathTimers.has(player_salmon)) {
			if (action == GLFW_PRESS && key == GLFW_KEY_LEFT) {
				salmon_motion.velocity.x = 10;
				salmon_motion.position.x -= salmon_motion.velocity.x;
			}
			if (action == GLFW_PRESS && key == GLFW_KEY_RIGHT) {
				salmon_motion.velocity.x = 10;
				salmon_motion.position.x += salmon_motion.velocity.x;
			}
			// Control salmon's vertical motiom with up/down keys
			if (action == GLFW_PRESS && key == GLFW_KEY_DOWN) {
				// salmon_motion.velocity.y = 10;
				// salmon_motion.position.y += salmon_motion.velocity.y;
				salmon_motion.velocity.y = 10 * sin(salmon_motion.angle);
				salmon_motion.velocity.x = 10 * cos(salmon_motion.angle);
				// salmon_motion.position.y += salmon_motion.velocity.y;
				salmon_motion.position = vec2(salmon_motion.position.x + salmon_motion.velocity.x, salmon_motion.position.y + salmon_motion.velocity.y);
			}
			if (action == GLFW_PRESS && key == GLFW_KEY_UP) {
				// salmon_motion.velocity.y = 10;
				// salmon_motion.position.y -= salmon_motion.velocity.y;
				salmon_motion.velocity.y = 10 * sin(salmon_motion.angle);
				salmon_motion.velocity.x = 10 * cos(salmon_motion.angle);
				// salmon_motion.position.y += salmon_motion.velocity.y;
				salmon_motion.position = vec2(salmon_motion.position.x + salmon_motion.velocity.x, salmon_motion.position.y + salmon_motion.velocity.y);
			}
		}

		if (action == GLFW_PRESS && key == GLFW_KEY_A) {
			current_speed = 3.f;
			registry.mode.get(player_salmon).basicMode = false;
		}

		if (action == GLFW_PRESS && key == GLFW_KEY_B) {
			current_speed = 1.f;
			registry.mode.get(player_salmon).basicMode = true;
		}
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE SALMON ROTATION HERE
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	auto& motion = registry.motions.get(player_salmon);
	float newAngle = atan2(mouse_position.y - motion.position.y, mouse_position.x - motion.position.x);
	motion.angle = newAngle;
}
