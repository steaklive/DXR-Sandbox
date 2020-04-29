#pragma once

#include "Common.h"

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

struct aiMesh;
class DXRSModel;
class DXRSModelMaterial;

class DXRSMesh
{
public:
	DXRSMesh(DXRSModel& model, DXRSModelMaterial* material);
	DXRSMesh(DXRSModel& model, aiMesh& mesh);
	~DXRSMesh();

	DXRSModel& GetModel();
	DXRSModelMaterial* GetMaterial();
	const std::string& Name() const;

	const std::vector<XMFLOAT3>& Vertices() const;
	const std::vector<XMFLOAT3>& Normals() const;
	const std::vector<XMFLOAT3>& Tangents() const;
	const std::vector<XMFLOAT3>& BiNormals() const;
	const std::vector<std::vector<XMFLOAT3>*>& TextureCoordinates() const;
	const std::vector<std::vector<XMFLOAT4>*>& VertexColors() const;
	UINT FaceCount() const;
	const std::vector<UINT>& Indices() const;

	void CreateIndexBuffer(ComPtr<ID3D12Resource> indexBuffer, ComPtr<ID3D12Resource> indexBufferUploadHeap);
private:

	DXRSMesh(const DXRSMesh& rhs);
	DXRSMesh& operator=(const DXRSMesh& rhs);

	DXRSModel& mModel;
	DXRSModelMaterial* mMaterial;

	std::string mName;
	std::vector<XMFLOAT3> mVertices;
	std::vector<XMFLOAT3> mNormals;
	std::vector<XMFLOAT3> mTangents;
	std::vector<XMFLOAT3> mBiNormals;
	std::vector<std::vector<XMFLOAT3>*> mTextureCoordinates;
	std::vector<std::vector<XMFLOAT4>*> mVertexColors;
	UINT mFaceCount;
	std::vector<UINT> mIndices;
};
