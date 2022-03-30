#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Mesh.h"

class MeshModel
{
public:
	MeshModel();
	MeshModel(std::vector<Mesh> newMeshList);

	size_t GetMeshCount();
	Mesh* GetMesh(size_t idx);

	glm::mat4 GetModel();
	void SetModel(glm::mat4 newModel);

	void DestroyMeshModel();

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice physDevice, VkDevice logicDevice, VkQueue transferQueue,
		VkCommandPool transferPool, aiNode* node, const aiScene* scene, std::vector<size_t> matToTex);
	static Mesh LoadMesh(VkPhysicalDevice physDevice, VkDevice logicDevice, VkQueue transferQueue,
	                     VkCommandPool transferPool, aiMesh* mesh, const aiScene* scene, std::vector<size_t> matToTex);

	~MeshModel();

private:
	std::vector<Mesh> meshList;
	glm::mat4 model;
};