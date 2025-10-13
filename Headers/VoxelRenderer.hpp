#pragma once

#include <functional>
#include <LexviEngine.hpp>
#include <Game/Game.hpp>

#include <Shader/Shader.hpp>

#include <Camera/Camera.hpp>
namespace VoxelRenderer {
	class VoxelRenderer {
	private:
		Lexvi::Engine engine;

	public:
		void Init();

		void UpdateVoxels();

	private:
		void render();
	};
}