#include "VoxelRenderer.hpp"

#include "Shaders/Blur_frag.h"
#include "Shaders/voxel_vert.h"
#include "Shaders/voxel_frag.h"
#include "Shaders/postProcessing_frag.h"
#include "Shaders/postProcessing_vert.h"
#include "Shaders/SetSubChunks_comp.h"

#include "pch.h"

using namespace Lexvi::Extensions::VoxelRenderer;

VoxelRenderer::VoxelRenderer(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize)
{
	Init(chunkNum, chunkDimensions, chunkSize);
}

VoxelRenderer::VoxelRenderer(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize, const std::vector<Voxel>& voxels)
{
	Init(chunkNum, chunkDimensions, chunkSize, voxels);
}

void VoxelRenderer::InitShaders()
{
	const bool fromFile = false;
	const char* geomtryShader = "";

	voxelShader = Shader(voxel_vert, voxel_frag, geomtryShader, fromFile);
	blurShader = Shader(postProcessing_vert, Blur_frag, geomtryShader, fromFile);
	PostProcessingShader = Shader(postProcessing_vert, postProcessing_frag, geomtryShader, fromFile);

	SetSubChunksShader = ComputeShader(SetSubChunks_comp, fromFile);

	SetSubChunksShader.use();
	SetSubChunksShader.setiVec3("VoxelCountPerChunk", chunkDimensions);
	SetSubChunksShader.setiVec3("chunkNum", chunkNum);
	SetSubChunksShader.setFloat("chunkSize", chunkSize);
	SetSubChunksShader.setiVec3("chunkSubDivision", chunkSubDivision);

	voxelShader.use();
	voxelShader.setUint("chunkNum", (unsigned int)totalChunkNum);
	voxelShader.setFloat("chunkSize", chunkSize);
	voxelShader.setiVec3("VoxelCountPerChunk", chunkDimensions);
	voxelShader.setiVec3("chunkSubDivision", chunkSubDivision);
	voxelShader.setInt("pointLightNum", 0);
}

void VoxelRenderer::InitSettings()
{
	voxelShader.use();

	voxelShader.setBool("u_EnableShadows", ShadowEnabled.getValue());
	voxelShader.setFloat("u_ShadowDist", ShadowDist.getValue());
	voxelShader.setFloat("u_ShadowFar", ShadowFar.getValue());

	// Reflections
	voxelShader.setBool("u_EnableReflections", enableReflections.getValue());
	voxelShader.setFloat("u_ReflectionFar", reflectionFar.getValue());
	voxelShader.setInt("u_MaxReflectionNum", maxReflectionNum.getValue());

	// Transparncy
	voxelShader.setBool("u_EnableTransparency", enableTransparency.getValue());
	voxelShader.setInt("u_MaxTransparencyNum", maxTransparencyNum.getValue());

	// Lighting
	voxelShader.setInt("u_MultipleLights", multipleLights.getValue());
}

void VoxelRenderer::InitBuffers()
{
	voxelFrameBuffer = FrameBuffer(FrameBufferAttachments::COLOR | FrameBufferAttachments::DEPTH, 2, 1080, 1024);
	
	for (size_t i = 0; i < 2; ++i) {
		pingpongFBO[i] = FrameBuffer(FrameBufferAttachments::COLOR, 1080, 1024);
	}

	FrameOutput = Quad(1.0f, 1.0f);

	std::vector<uint32_t> compressedChunks;
	compressedChunks.reserve(totalChunkNum);

	for (uint32_t x = 0; x < chunkNum.x; ++x) {
		for (uint32_t y = 0; y < chunkNum.y; ++y) {
			for (uint32_t z = 0; z < chunkNum.z; ++z) {
				compressedChunks.emplace_back(encodeChunk(x, y, z));
			}
		}
	}

	CreateSSBO(chunkSSBO, ChunkMemorySize * totalChunkNum, 0);
	UpdateSSBO(chunkSSBO, compressedChunks.data(), ChunkMemorySize * totalChunkNum, 0);

	CreateSSBO(voxelSSBO, VoxelMemorySize * voxelNum, 1);

	chunkEmpty.resize(totalChunkNum * subDivisionCount);
	std::fill(chunkEmpty.begin(), chunkEmpty.end(), 1);

	CreateSSBO(emptyChunkSSBO, totalChunkNum * subDivisionCount * sizeof(uint32_t), 2);
	UpdateSSBO(emptyChunkSSBO, chunkEmpty.data(), totalChunkNum * subDivisionCount * sizeof(uint32_t), 0);

	filledSubchunks = std::vector<uint32_t>(subDivisionCount, 1);

	UpdateSubChunks();
}

void VoxelRenderer::UpdateSubChunks() {
	SetSubChunksShader.use();

	for (size_t i = 0; i < totalChunkNum; ++i) {
		uint32_t offsetBytes = static_cast<uint32_t>(i * subDivisionCount  * sizeof(uint32_t));

		UpdateSSBO(emptyChunkSSBO, filledSubchunks.data(), subDivisionCount * sizeof(uint32_t), offsetBytes);
		glDispatchCompute(
			chunkDimensions.x / 8,
			chunkDimensions.y / 8,
			chunkDimensions.z / 8);
	}
}

void VoxelRenderer::UpdateSubChunk(uint32_t chunkIndex) {
	SetSubChunksShader.use();
	uint32_t offsetBytes = static_cast<uint32_t>(chunkIndex * subDivisionCount * sizeof(uint32_t));

	UpdateSSBO(emptyChunkSSBO, filledSubchunks.data(), subDivisionCount * sizeof(uint32_t), offsetBytes);
	glDispatchCompute(
		chunkDimensions.x / 8,
		chunkDimensions.y / 8,
		chunkDimensions.z / 8);
}

void VoxelRenderer::Init(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize)
{
	this->chunkDimensions = chunkDimensions;
	this->chunkSize = chunkSize;

	this->voxelNum = static_cast<size_t>(chunkDimensions.x) * chunkDimensions.y * chunkDimensions.z;

	this->totalChunkNum = static_cast<size_t>(chunkNum.x) * chunkNum.y * chunkNum.z;

	this->chunkNum = chunkNum;

	this->voxelSize = this->chunkSize / glm::vec3(this->chunkDimensions);

	this->InitShaders();
	this->InitBuffers();
	this->InitSettings();
}

void VoxelRenderer::Init(glm::uvec3 chunkNum, glm::uvec3 chunkDimensions, float chunkSize, const std::vector<Voxel>& voxels)
{
	this->chunkDimensions = chunkDimensions;
	this->chunkSize = chunkSize;

	this->voxelNum = static_cast<size_t>(chunkDimensions.x) * chunkDimensions.y * chunkDimensions.z;

	this->totalChunkNum = static_cast<size_t>(chunkNum.x) * chunkNum.y * chunkNum.z;

	this->chunkNum = chunkNum;

	this->voxelSize = this->chunkSize / glm::vec3(this->chunkDimensions);

	this->InitShaders();
	this->InitBuffers();
	this->InitSettings();
}

void VoxelRenderer::UpdateVoxel(Voxel voxel)
{
	uint32_t packedColor = packRGBA(voxel.color);
	size_t offset = flattenIndex(voxel.index);
	uint32_t voxelNum = 1;

	SendSubData(&packedColor, voxelNum, offset);

	glm::vec3 voxelPos = glm::vec3(voxel.index) * voxelSize;
	glm::ivec3 chunkIdx = glm::floor(voxelPos / chunkSize);

	if (glm::any(glm::lessThan(chunkIdx, glm::ivec3(0))) || glm::any(glm::greaterThanEqual(chunkIdx, glm::ivec3(chunkNum)))) {
		return; // Out of bounds
	}

	uint32_t chunkIndex = chunkIdx.x * (chunkNum.y * chunkNum.z) + chunkIdx.y * chunkNum.z + chunkIdx.z;

	UpdateSubChunk(chunkIndex);
}

void VoxelRenderer::UpdateAllVoxels(const std::vector<Voxel>& voxels)
{
	if (voxels.size() != voxelNum) return;

	std::vector<uint32_t> packedVoxels;
	packedVoxels.reserve(voxels.size());

	for (const Voxel& voxel : voxels) {
		packedVoxels.push_back(packRGBA(voxel.color));
	}

	SendAllData(packedVoxels);

	UpdateSubChunks();
}

void Lexvi::Extensions::VoxelRenderer::VoxelRenderer::ClearVoxel(Voxel voxel)
{
	UpdateSSBO(voxelSSBO, nullptr, VoxelMemorySize, flattenIndex(voxel.index));
}

void Lexvi::Extensions::VoxelRenderer::VoxelRenderer::ClearAllVoxels()
{
	// Resizing ssbo clears it and re-allocates it
	// Better than copying huge vecotr of empty voxels
	ResizeSSBO(voxelSSBO, voxelNum * VoxelMemorySize);
}

void Lexvi::Extensions::VoxelRenderer::VoxelRenderer::MoveVoxel(Voxel lastVoxel, VoxelIndex newPosition)
{
	ClearVoxel(lastVoxel);

	UpdateVoxel(Voxel{
		.index = newPosition,
		.color = lastVoxel.color
		}
	);
}

void Lexvi::Extensions::VoxelRenderer::VoxelRenderer::SetViewingMode(ViewingMode mode)
{
	viewingMode = mode;
}

void VoxelRenderer::Draw(const Camera& camera, Renderer& renderer)
{
	BindSSBO(voxelSSBO);
	BindSSBO(chunkSSBO);
	BindSSBO(emptyChunkSSBO);

	voxelShader.use();
	voxelShader.setVec3("uOrigin", camera.getPosition());
	voxelShader.setVec3("uRight", camera.getRight());
	voxelShader.setVec3("uUp", camera.getUp());
	voxelShader.setFloat("far", camera.getZNearAndZFar().y);
	voxelShader.setFloat("uFOV", glm::radians(camera.getFOV()));
	voxelShader.setFloat("uRayOffset", 0.0f);
	voxelShader.setMat4("uViewMatrix", camera.getViewMatrix());
	voxelShader.setMat4("uProjectionMatrix", camera.getProjectionMatrix());

	glm::vec3 lightDir = { 0.0, -1.0, 0.0 };
	float ambient = 0.2f;
	float diffuse = 0.8f;
	float specular = 0.25f;
	glm::vec3 lightcolor = glm::vec3(1.0f, 0.95f, 0.9f);

	voxelShader.setVec3("dirlight.direction", lightDir);
	voxelShader.setVec3("dirlight.ambient", glm::vec3(ambient));
	voxelShader.setVec3("dirlight.diffuse", glm::vec3(diffuse));
	voxelShader.setVec3("dirlight.specular", glm::vec3(specular));
	voxelShader.setVec3("dirlight.color", lightcolor);

	voxelShader.setInt("ViewingMode", viewingMode);

	// Draw Voxels To Quad
	{
		voxelFrameBuffer.BindFrameBuffer();

		renderer.ClearBuffers();

		FrameOutput.Draw(&voxelShader);

		voxelFrameBuffer.UnBindFrameBuffer();
	}

	// Bloom
	bool horizontalBloomPass = true;
	{
		bool first_iteration = true;
		unsigned int textureID = voxelFrameBuffer.getAttachment(FrameBufferAttachments::COLOR, 1)->id;
		int amount = 10;
		blurShader.use();

		blurShader.setInt("image", 0);

		for (int i = 0; i < amount; i++)
		{
			pingpongFBO[horizontalBloomPass].BindFrameBuffer();

			renderer.ClearBuffers();

			blurShader.setInt("horizontal", horizontalBloomPass);

			if (!first_iteration) {
				textureID = pingpongFBO[!horizontalBloomPass].getAttachment(FrameBufferAttachments::COLOR)->id;
			}

			BindTexture(0, textureID);

			FrameOutput.Draw(&blurShader);

			horizontalBloomPass = !horizontalBloomPass;
			if (first_iteration) first_iteration = false;
		}
		pingpongFBO[static_cast<int>(!horizontalBloomPass)].UnBindFrameBuffer();
	}

	PostProcessingShader.use();
	PostProcessingShader.setFloat("near", camera.getZNearAndZFar().x);
	PostProcessingShader.setFloat("far", camera.getZNearAndZFar().y);
	PostProcessingShader.setFloat("editRadius", 0.0f);
	PostProcessingShader.setFloat("radiusOpacity", 0.0f);

	PostProcessingShader.setFloat("fog.Start", fog.Start);
	PostProcessingShader.setFloat("fog.End", fog.End);
	PostProcessingShader.setFloat("fog.Density", fog.Density);
	PostProcessingShader.setVec3("fog.Color", fog.Color);

	PostProcessingShader.setFloat("exposure", 1.5f);
	PostProcessingShader.setFloat("bloomIntensity", 2.5f);

	PostProcessingShader.setInt("screenTexture", 0);
	PostProcessingShader.setInt("bloomTexture", 1);
	PostProcessingShader.setInt("depthTexture", 2);

	// PostProcessing
	{
		BindTexture(0, voxelFrameBuffer.getAttachment(FrameBufferAttachments::COLOR, 0)->id);
		BindTexture(1, pingpongFBO[static_cast<int>(!horizontalBloomPass)].getAttachment(FrameBufferAttachments::COLOR)->id);
		BindTexture(2, voxelFrameBuffer.getAttachment(FrameBufferAttachments::DEPTH)->id);

		FrameOutput.Draw(&PostProcessingShader);
	}
}

void Lexvi::Extensions::VoxelRenderer::VoxelRenderer::OnResize(unsigned int width, unsigned int height)
{
	voxelFrameBuffer.ResizeFrameBuffer(width, height);

	for (size_t i = 0; i < 2; ++i) {
		pingpongFBO[i].ResizeFrameBuffer(width, height);
	}

	voxelShader.use();
	voxelShader.setVec2("uResolution", (float)width, (float)height);

	PostProcessingShader.use();
	PostProcessingShader.setVec2("uResolution", (float)width, (float)height);
}

uint32_t VoxelRenderer::flattenIndex(VoxelIndex index) const
{
	glm::vec3 voxelPos = glm::vec3(index) * voxelSize;

	glm::ivec3 chunkIdx = glm::floor(voxelPos / chunkSize);
	if (glm::any(glm::lessThan(chunkIdx, glm::ivec3(0))) || glm::any(glm::greaterThanEqual(chunkIdx, glm::ivec3(chunkNum)))) {
		return 0; // Out of bounds
	}

	uint32_t chunkIndex = chunkIdx.x * (chunkNum.y * chunkNum.z) + chunkIdx.y * chunkNum.z + chunkIdx.z;
	glm::vec3 chunkPos = glm::vec3(chunkIdx) * chunkSize;

	glm::ivec3 voxelIndex = glm::ivec3((voxelPos - chunkPos) / voxelSize);

	if (glm::any(glm::lessThan(voxelIndex, glm::ivec3(0))) || glm::any(glm::greaterThanEqual(voxelIndex, glm::ivec3(chunkDimensions)))) {
		return 0; // Skip out-of-bounds voxels
	}

	uint32_t flattened_index = voxelIndex.x + chunkDimensions.x * (voxelIndex.y + chunkDimensions.y * voxelIndex.z);
	return flattened_index + chunkIndex;
}

uint32_t VoxelRenderer::packRGBA(Color color)
{
	// Clamp and convert to 0–255 range\n"
	uint32_t r = static_cast<uint32_t>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
	uint32_t g = static_cast<uint32_t>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
	uint32_t b = static_cast<uint32_t>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
	uint32_t a = static_cast<uint32_t>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
	
	 // Pack into a single uint: RRRRRRRR GGGGGGGG BBBBBBBB AAAAAAAA
	 return (r << 24) | (g << 16) | (b << 8) | a;
}

VoxelRenderer::Color VoxelRenderer::unpackVoxel(uint32_t color)
{
	float r = static_cast<float>((color >> 24) & 0xFFu) / 255.0f;
	float g = static_cast<float>((color >> 16) & 0xFFu) / 255.0f;
	float b = static_cast<float>((color >> 8)  & 0xFFu) / 255.0f;
	float a = static_cast<float>(color & 0xFFu) / 255.0f;

	return Color(r, g, b, a);
}

uint32_t Lexvi::Extensions::VoxelRenderer::VoxelRenderer::encodeChunk(int x, int y, int z)
{
	auto pack = [](int value) -> uint32_t {
		return static_cast<uint32_t>(value & 0x3FF); // mask to 10 bits
		};
	return (pack(x) << 20) | (pack(y) << 10) | pack(z);
}

void VoxelRenderer::SendSubData(uint32_t* data, uint32_t voxelNum, size_t voxel_offset)
{
	UpdateSSBO(voxelSSBO, data, voxelNum * VoxelMemorySize, static_cast<uint32_t>(voxel_offset * VoxelMemorySize));
}

void VoxelRenderer::SendAllData(const std::vector<uint32_t>& voxels)
{
	UpdateSSBO(voxelSSBO, voxels.data(), voxelNum * VoxelMemorySize, 0);
}
