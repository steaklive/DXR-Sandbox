#pragma once

#include "Common.h"
#include "DXRSGraphics.h"
#include "DXRSMesh.h"
#include "DXRSModelMaterial.h"
#include "DXRSBuffer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <map>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;
using namespace DirectX;


class DXRSModel
{
	__declspec(align(16)) struct ModelConstantBuffer
	{
		XMMATRIX	World;
	};

public:
	DXRSModel(DXRSGraphics& dxWrapper, const std::string& filename, bool flipUVs = false, XMMATRIX tranformWorld = XMMatrixIdentity());
	~DXRSModel();

	void UpdateWorldMatrix(XMMATRIX matrix);

	DXRSGraphics& GetDXWrapper();

	DXRSBuffer* GetCB() { return mBufferCB; }
	bool HasMeshes() const;
	bool HasMaterials() const;

	void Render(ID3D12GraphicsCommandList* commandList);

	const std::vector<DXRSMesh*>& Meshes() const;
	const std::vector<DXRSModelMaterial*>& Materials() const;
	const std::string GetFileName() { return mFilename; }
	const char* GetFileNameChar() { return mFilename.c_str(); }
	std::vector<XMFLOAT3> GenerateAABB();


private:
	DXRSModel(const DXRSModel& rhs);
	DXRSModel& operator=(const DXRSModel& rhs);

	DXRSGraphics& mDXWrapper;

	DXRSBuffer* mBufferCB;

	std::vector<DXRSMesh*> mMeshes;
	std::vector<DXRSModelMaterial*> mMaterials;
	std::string mFilename;
};

