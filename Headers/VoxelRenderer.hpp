#pragma once

#include <Renderable/Primitives/Quad.hpp>
#include <Shader/ComputeShader.hpp>
#include <Utils/FrameBuffer.hpp>
#include <Renderer/Renderer.hpp>
#include <Settings/Settings.hpp>
#include <Shader/Shader.hpp>
#include <Camera/Camera.hpp>
#include <Utils/SSBO.hpp>

#include <glm/glm.hpp>

#include <array>

namespace Lexvi {
	namespace Extensions {
		namespace VoxelRenderer {

			class VoxelRenderer {
			public:
				using VoxelIndex = glm::uvec3;
				using Color = glm::vec4;

			public:
				struct Voxel {
					VoxelIndex index;
					Color color;
				};

			private:
				struct Fog {
					float Start = 0.0f;
					float End = 1.0f;
					float Density = 0.5f;
					glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
				};

			public:
				enum ViewingMode {
					RENDER,
					COLOR,
					NORMAL,
					DISTANCE
				};

			private:
				Shader blurShader{};
				Shader voxelShader{};
				Shader PostProcessingShader{};

				ComputeShader SetSubChunksShader{};
				
				SSBO voxelSSBO{};
				SSBO chunkSSBO{};
				SSBO emptyChunkSSBO{};

				FrameBuffer voxelFrameBuffer{};
				std::array<FrameBuffer, 2> pingpongFBO;
				Quad FrameOutput{};

				Fog fog{};

				ViewingMode viewingMode = RENDER;

				std::vector<uint32_t> filledSubchunks;

			public:
				Setting<bool> ShadowEnabled = true;
				Setting<float> ShadowDist = 100.0f;
				Setting<float> ShadowFar = 100.0f;

				Setting<bool> enableReflections = false;
				Setting<float> reflectionFar = 100.0f;
				Setting<int> maxReflectionNum = 2;

				Setting<bool> enableTransparency = false;
				Setting<int> maxTransparencyNum = 2;

				Setting<int> multipleLights = 0;

			private:
				size_t voxelNum = 0;
				float chunkSize = 0.0f;
				size_t totalChunkNum = 0;
				glm::uvec3 chunkNum = glm::uvec3(0);
				glm::vec3 voxelSize = glm::vec3(0.0f);
				glm::uvec3 chunkDimensions = glm::uvec3(0);

				const size_t VoxelMemorySize = sizeof(uint32_t);
				const size_t ChunkMemorySize = sizeof(uint32_t);

			private:
				const glm::ivec3 chunkSubDivision = { 16, 16, 16 };
				const unsigned int subDivisionCount = chunkSubDivision.x * chunkSubDivision.y * chunkSubDivision.z;
				std::vector<uint32_t> chunkEmpty{};

			public:
				VoxelRenderer() = default;
				VoxelRenderer(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize);
				VoxelRenderer(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize, const std::vector<Voxel>& voxels);

			public:
				// Initializes the voxel grid
				void Init(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize);
				void Init(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize, const std::vector<Voxel>& voxels);

				void UpdateVoxel(Voxel voxel);
				void UpdateAllVoxels(const std::vector<Voxel>& voxels);

				void ClearVoxel(Voxel voxel);
				void ClearAllVoxels();

				void MoveVoxel(Voxel lastVoxel, VoxelIndex newPosition);

				void SetViewingMode(ViewingMode mode);

				void Draw(const Camera& camera, Renderer& renderer);

				void OnResize(unsigned int width, unsigned int height);

			public:
				uint32_t flattenIndex(VoxelIndex index) const;

			private:
				uint32_t packRGBA(Color color);
				Color unpackVoxel(uint32_t color);

				uint32_t encodeChunk(int x, int y, int z);

			private:
				void SendSubData(uint32_t* data, uint32_t voxelNum, size_t offset);
				void SendAllData(const std::vector<uint32_t>& voxels);

				void InitShaders();
				void InitBuffers();
				void InitSettings();

				void UpdateSubChunks();
				void UpdateSubChunk(uint32_t chunkIndex);
			};
		}
	}
}