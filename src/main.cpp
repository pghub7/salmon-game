
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>
#include <thread>
#include <iostream>

// internal
#include "ai_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"

using Clock = std::chrono::high_resolution_clock;

const int window_width_px = 1200;
const int window_height_px = 800;

//void framebuffer_size_callback(GLFWwindow* window, int width, int height)
//{
//	glViewport(0, 0, width, height);
//}

// Entry point
int main()
{
	// Global systems
	WorldSystem world;
	RenderSystem renderer;
	PhysicsSystem physics;
	AISystem ai;
	ai.frameCounter = 5;

	// Initializing window
	GLFWwindow* window = world.create_window(window_width_px, window_height_px);
	if (!window) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}
	// glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// initialize the main systems
	renderer.init(window_width_px, window_height_px, window);
	world.init(&renderer);

	// variable timestep loop
	auto t = Clock::now();
	while (!world.is_over()) {
		// Processes system messages, if this wasn't present the window would become
		// unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;
		
		world.step(elapsed_ms);
		if (ai.internalFrameCounter == 100) {
			ai.internalFrameCounter = 0;
		}
		else {
			ai.internalFrameCounter++;

		}
		ai.step(elapsed_ms);
		physics.step(elapsed_ms, window_width_px, window_height_px);
		world.handle_collisions();

		renderer.draw();

		// TODO A2: you can implement the debug freeze here but other places are possible too.
		if (debugging.in_freeze_mode && debugging.in_debug_mode) {
			printf("inside freeze\n");
			// std::chrono::milliseconds timespan(500);
			// std::this_thread::sleep_for(timespan);
			// Sleep(50);
			debugging.in_freeze_mode = false;
		}
		
	}

	return EXIT_SUCCESS;
}
