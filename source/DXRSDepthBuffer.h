#pragma once
#include "Common.h"

class DXRSDepthBuffer
{
public:
	DXRSDepthBuffer(ID3D12Device* device, int width, int height, DXGI_FORMAT format);
	~DXRSDepthBuffer();

	ID3D12Resource* GetResource() { return mDepthStencilResource.Get(); }
	DXGI_FORMAT GetFormat() { return mFormat; }
	void TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES stateAfter);

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV()
	{
		return mDescriptorDSV->GetFirstCpuHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV()
	{
		return mDescriptorSRV->GetFirstCpuHandle();
	}

private:

	int mWidth, mHeight;
	DXGI_FORMAT mFormat;
	D3D12_RESOURCE_STATES mCurrentResourceState;

	U_PTR<DescriptorHeap> mDescriptorDSV;
	U_PTR<DescriptorHeap> mDescriptorSRV;
	ComPtr<ID3D12Resource> mDepthStencilResource;
};