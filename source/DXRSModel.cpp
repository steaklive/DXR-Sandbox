#define NOMINMAX

#include "DXRSModel.h"

DXRSModel::DXRSModel(DXRSGraphics& dxWrapper, const std::string& filename, bool flipUVs)
	: mMeshes(), mMaterials(), mDXWrapper(dxWrapper)
{
	Assimp::Importer importer;

	UINT flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_FlipWindingOrder;
	if (flipUVs)
	{
		flags |= aiProcess_FlipUVs;
	}

	const aiScene* scene = importer.ReadFile(filename, flags);

	if (scene == nullptr)
	{
		throw std::exception(importer.GetErrorString());
	}

	if (scene->HasMaterials())
	{
		for (UINT i = 0; i < scene->mNumMaterials; i++)
		{
			mMaterials.push_back(new DXRSModelMaterial(*this, scene->mMaterials[i]));
		}
	}

	if (scene->HasMeshes())
	{
		for (UINT i = 0; i < scene->mNumMeshes; i++)
		{
			DXRSModelMaterial* material = (mMaterials.size() > i ? mMaterials.at(i) : nullptr);

			DXRSMesh* mesh = new DXRSMesh(*this, *(scene->mMeshes[i]));
			mMeshes.push_back(mesh);
		}
	}

	mFilename = filename;
}

DXRSModel::~DXRSModel()
{
	for (DXRSMesh* mesh : mMeshes)
	{
		delete mesh;
	}

	for (DXRSModelMaterial* material : mMaterials)
	{
		delete material;
	}
}

DXRSGraphics& DXRSModel::GetDXWrapper()
{
	return mDXWrapper;
}

bool DXRSModel::HasMeshes() const
{
	return (mMeshes.size() > 0);
}

bool DXRSModel::HasMaterials() const
{
	return (mMaterials.size() > 0);
}

const std::vector<DXRSMesh*>& DXRSModel::Meshes() const
{
	return mMeshes;
}

const std::vector<DXRSModelMaterial*>& DXRSModel::Materials() const
{
	return mMaterials;
}

std::vector<XMFLOAT3> DXRSModel::GenerateAABB()
{
	std::vector<XMFLOAT3> vertices;

	for (DXRSMesh* mesh : mMeshes)
	{
		vertices.insert(vertices.end(), mesh->Vertices().begin(), mesh->Vertices().end());
	}


	XMFLOAT3 minVertex = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxVertex = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (UINT i = 0; i < vertices.size(); i++)
	{

		//Get the smallest vertex 
		minVertex.x = std::min(minVertex.x, vertices[i].x);    // Find smallest x value in model
		minVertex.y = std::min(minVertex.y, vertices[i].y);    // Find smallest y value in model
		minVertex.z = std::min(minVertex.z, vertices[i].z);    // Find smallest z value in model

		//Get the largest vertex 
		maxVertex.x = std::max(maxVertex.x, vertices[i].x);    // Find largest x value in model
		maxVertex.y = std::max(maxVertex.y, vertices[i].y);    // Find largest y value in model
		maxVertex.z = std::max(maxVertex.z, vertices[i].z);    // Find largest z value in model
	}

	std::vector<XMFLOAT3> AABB;

	// Our AABB [0] is the min vertex and [1] is the max
	AABB.push_back(minVertex);
	AABB.push_back(maxVertex);

	return AABB;
}