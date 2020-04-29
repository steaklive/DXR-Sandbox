#define NOMINMAX

#include "DXRSMesh.h"
#include "DXRSModel.h"
#include <assimp/scene.h>

DXRSMesh::DXRSMesh(DXRSModel& model, aiMesh& mesh)
	: mModel(model), mMaterial(nullptr), mName(mesh.mName.C_Str()), mVertices(), mNormals(), mTangents(), mBiNormals(), mTextureCoordinates(), mVertexColors(), mFaceCount(0), mIndices()
{
	mMaterial = mModel.Materials().at(mesh.mMaterialIndex);

	// Vertices
	mVertices.reserve(mesh.mNumVertices);
	for (UINT i = 0; i < mesh.mNumVertices; i++)
	{
		mVertices.push_back(XMFLOAT3(reinterpret_cast<const float*>(&mesh.mVertices[i])));
	}

	// Normals
	if (mesh.HasNormals())
	{
		mNormals.reserve(mesh.mNumVertices);
		for (UINT i = 0; i < mesh.mNumVertices; i++)
		{
			mNormals.push_back(XMFLOAT3(reinterpret_cast<const float*>(&mesh.mNormals[i])));
		}
	}

	// Tangents and Binormals
	if (mesh.HasTangentsAndBitangents())
	{
		mTangents.reserve(mesh.mNumVertices);
		mBiNormals.reserve(mesh.mNumVertices);
		for (UINT i = 0; i < mesh.mNumVertices; i++)
		{
			mTangents.push_back(XMFLOAT3(reinterpret_cast<const float*>(&mesh.mTangents[i])));
			mBiNormals.push_back(XMFLOAT3(reinterpret_cast<const float*>(&mesh.mBitangents[i])));
		}
	}

	// Texture Coordinates
	UINT uvChannelCount = mesh.GetNumUVChannels();
	for (UINT i = 0; i < uvChannelCount; i++)
	{
		std::vector<XMFLOAT3>* textureCoordinates = new std::vector<XMFLOAT3>();
		textureCoordinates->reserve(mesh.mNumVertices);
		mTextureCoordinates.push_back(textureCoordinates);

		aiVector3D* aiTextureCoordinates = mesh.mTextureCoords[i];
		for (UINT j = 0; j < mesh.mNumVertices; j++)
		{
			textureCoordinates->push_back(XMFLOAT3(reinterpret_cast<const float*>(&aiTextureCoordinates[j])));
		}
	}

	// Vertex Colors
	UINT colorChannelCount = mesh.GetNumColorChannels();
	for (UINT i = 0; i < colorChannelCount; i++)
	{
		std::vector<XMFLOAT4>* vertexColors = new std::vector<XMFLOAT4>();
		vertexColors->reserve(mesh.mNumVertices);
		mVertexColors.push_back(vertexColors);

		aiColor4D* aiVertexColors = mesh.mColors[i];
		for (UINT j = 0; j < mesh.mNumVertices; j++)
		{
			vertexColors->push_back(XMFLOAT4(reinterpret_cast<const float*>(&aiVertexColors[j])));
		}
	}

	// Faces (note: could pre-reserve if we limit primitive types)
	if (mesh.HasFaces())
	{
		mFaceCount = mesh.mNumFaces;
		for (UINT i = 0; i < mFaceCount; i++)
		{
			aiFace* face = &mesh.mFaces[i];

			for (UINT j = 0; j < face->mNumIndices; j++)
			{
				mIndices.push_back(face->mIndices[j]);
			}
		}
	}
}

DXRSMesh::~DXRSMesh()
{
	for (std::vector<XMFLOAT3>* textureCoordinates : mTextureCoordinates)
	{
		delete textureCoordinates;
	}

	for (std::vector<XMFLOAT4>* vertexColors : mVertexColors)
	{
		delete vertexColors;
	}
}

DXRSModel& DXRSMesh::GetModel()
{
	return mModel;
}

DXRSModelMaterial* DXRSMesh::GetMaterial()
{
	return mMaterial;
}

const std::string& DXRSMesh::Name() const
{
	return mName;
}

const std::vector<XMFLOAT3>& DXRSMesh::Vertices() const
{
	return mVertices;
}

const std::vector<XMFLOAT3>& DXRSMesh::Normals() const
{
	return mNormals;
}

const std::vector<XMFLOAT3>& DXRSMesh::Tangents() const
{
	return mTangents;
}

const std::vector<XMFLOAT3>& DXRSMesh::BiNormals() const
{
	return mBiNormals;
}

const std::vector<std::vector<XMFLOAT3>*>& DXRSMesh::TextureCoordinates() const
{
	return mTextureCoordinates;
}

const std::vector<std::vector<XMFLOAT4>*>& DXRSMesh::VertexColors() const
{
	return mVertexColors;
}

UINT DXRSMesh::FaceCount() const
{
	return mFaceCount;
}

const std::vector<UINT>& DXRSMesh::Indices() const
{
	return mIndices;
}

void DXRSMesh::CreateIndexBuffer(ComPtr<ID3D12Resource> indexBuffer, ComPtr<ID3D12Resource> indexBufferUploadHeap)
{
	// create default heap to hold index buffer
	mModel.GetDXWrapper().GetD3DDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(mIndices.size()), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
		nullptr, // optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&indexBuffer));

	indexBuffer->SetName(L"Index Buffer Resource Heap");

	// create upload heap to upload index buffer
	ID3D12Resource* iBufferUploadHeap;
	mModel.GetDXWrapper().GetD3DDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(mIndices.size()), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));
	indexBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(mIndices[0]); // pointer to our index array
	indexData.RowPitch = mIndices.size(); // size of all our index buffer
	indexData.SlicePitch = mIndices.size(); // also the size of our index buffer

}