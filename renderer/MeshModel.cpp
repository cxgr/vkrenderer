#include "MeshModel.h"

#include <iostream>
#include <utility>

MeshModel::MeshModel()
{
}

MeshModel::MeshModel(std::vector<Mesh> newMeshList) :
meshList(std::move(newMeshList)),
model(glm::mat4(1.0))
{
}

size_t MeshModel::GetMeshCount()
{
	return meshList.size();
}

Mesh* MeshModel::GetMesh(size_t idx)
{
	if (idx < 0 || idx >= meshList.size())
		throw std::runtime_error("oor mesh access");

	return &meshList[idx];
}

glm::mat4 MeshModel::GetModel()
{
	return model;
}

void MeshModel::SetModel(glm::mat4 newModel)
{
	model = newModel;
}

void MeshModel::DestroyMeshModel()
{
	for (auto& m : meshList)
		m.DestroyBuffers();
}

std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
{
	std::vector<std::string> texList(scene->mNumMaterials);

	std::cout << "mats " + std::to_string(scene->mNumMaterials) << std::endl;

	for (size_t i = 0; i < scene->mNumMaterials; ++i)
	{
		const auto mat = scene->mMaterials[i];
		texList[i] = "";

		std::cout << mat->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
		std::cout << mat->GetTextureCount(aiTextureType_UNKNOWN) << std::endl;
		std::cout << mat->GetName().C_Str() << std::endl;
		if (mat->GetName().C_Str() == "cottage_texture")
			std::cout << "yes" << std::endl;
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString path;
			if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				auto pathStr = std::string(path.data);
				std::cout << pathStr << std::endl;
				auto idx = pathStr.rfind('\\');
				auto fileName = pathStr.substr(idx + 1);

				texList[i] = fileName;
			}
		}
	}

	return texList;
}

std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice physDevice, VkDevice logicDevice, VkQueue transferQueue,
	VkCommandPool transferPool, aiNode* node, const aiScene* scene, std::vector<size_t> matToTex)
{
	std::vector<Mesh> meshList;

	for (size_t i = 0; i < node->mNumMeshes; ++i)
	{
		meshList.push_back(LoadMesh(physDevice, logicDevice, transferQueue, transferPool,
			scene->mMeshes[node->mMeshes[i]], scene, matToTex));
	}

	for (size_t i = 0; i < node->mNumChildren; ++i)
	{
		auto subMeshList = LoadNode(physDevice, logicDevice, transferQueue, transferPool, node->mChildren[i], scene, matToTex);
		meshList.insert(meshList.end(), subMeshList.begin(), subMeshList.end());
	}

	return meshList;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice physDevice, VkDevice logicDevice, VkQueue transferQueue,
	VkCommandPool transferPool, aiMesh* mesh, const aiScene* scene, std::vector<size_t> matToTex)
{
	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;

	verts.resize(mesh->mNumVertices);

	for (size_t i = 0; i < mesh->mNumVertices; ++i)
	{
		verts[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		if (mesh->mTextureCoords[0])
			verts[i].uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		else
			verts[i].uv = { 0.0f, 0.0f };
		verts[i].col = { 1.0, 1.0, 1.0 };
	}

	for (size_t i = 0; i < mesh->mNumFaces; ++i)
	{
		auto face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; ++j)
			indices.push_back(face.mIndices[j]);
	}

	auto newMesh = Mesh(physDevice, logicDevice, transferQueue, transferPool, &verts, &indices, matToTex[mesh->mMaterialIndex]);

	return newMesh;
}

MeshModel::~MeshModel()
{
}
