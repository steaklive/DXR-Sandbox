#include "DXRSDepthBuffer.h"

DXRSDepthBuffer::DXRSDepthBuffer(ID3D12Device* device, int width, int height, DXGI_FORMAT aFormat)
{
	mWidth = width;
	mHeight = height;
	mFormat = aFormat;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = aFormat;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS;

	if (aFormat == DXGI_FORMAT_D16_UNORM)
	{
		format = DXGI_FORMAT_R16_TYPELESS;
	}

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		mCurrentResourceState,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&mDepthStencilResource)
	));

	// DSV
	D3D12_DESCRIPTOR_HEAP_FLAGS flagsHeap = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	mDescriptorDSV = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, flagsHeap, 1);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = aFormat;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(mDepthStencilResource.Get(), &depthStencilDesc, mDescriptorDSV->GetFirstCpuHandle());

	// SRV
	mDescriptorSRV = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flagsHeap, 1);

	format = DXGI_FORMAT_R32_FLOAT;
	if (aFormat == DXGI_FORMAT_D16_UNORM)
	{
		format = DXGI_FORMAT_R16_UNORM;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(mDepthStencilResource.Get(), &srvDesc, mDescriptorSRV->GetFirstCpuHandle());
}

void DXRSDepthBuffer::TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES stateAfter)
{
	if (stateAfter != mCurrentResourceState)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), mCurrentResourceState, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
	}
}
