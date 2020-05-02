#pragma once
#include "Common.h"

class DXRSRenderTarget
{
public:
	DXRSRenderTarget(ID3D12Device* device, int width, int height, DXGI_FORMAT aFormat, D3D12_RESOURCE_FLAGS flags, LPCWSTR name);
	~DXRSRenderTarget();

	ID3D12Resource* GetResource() { return mRenderTarget.Get(); }

	int GetWidth() { return mWidth; }
	int GetHeight() { return mHeight; }
	void TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES stateAfter);

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV()
	{
		return mDescriptorRTV->GetFirstCpuHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV()
	{
		return mDescriptorSRV->GetFirstCpuHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV()
	{
		return mDescriptorUAV->GetFirstCpuHandle();
	}

private:

	int mWidth, mHeight;
	DXGI_FORMAT mFormat;
	D3D12_RESOURCE_STATES mCurrentResourceState;

	U_PTR<DescriptorHeap> mDescriptorUAV;
	U_PTR<DescriptorHeap> mDescriptorSRV;
	U_PTR<DescriptorHeap> mDescriptorRTV;
	ComPtr<ID3D12Resource> mRenderTarget;
};