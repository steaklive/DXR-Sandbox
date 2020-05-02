#include "DXRSRenderTarget.h"

DXRSRenderTarget::DXRSRenderTarget(ID3D12Device* device, int width, int height, DXGI_FORMAT aFormat, D3D12_RESOURCE_FLAGS flags, LPCWSTR name)
{
	mWidth = width;
	mHeight = height;
	mFormat = aFormat;

	XMFLOAT4 clearColor = { 0, 0, 0, 1 };
	DXGI_FORMAT format = aFormat;

	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = flags;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = format;
	optimizedClearValue.Color[0] = clearColor.x;
	optimizedClearValue.Color[1] = clearColor.y;
	optimizedClearValue.Color[2] = clearColor.z;
	optimizedClearValue.Color[3] = clearColor.w;

	mCurrentResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		mCurrentResourceState,
		&optimizedClearValue,
		IID_PPV_ARGS(&mRenderTarget)));

	mRenderTarget->SetName(name);

	D3D12_DESCRIPTOR_HEAP_FLAGS flagsHeap = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = aFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = aFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = aFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	mDescriptorRTV = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, flagsHeap, 1);
	device->CreateRenderTargetView(mRenderTarget.Get(), &rtvDesc, mDescriptorRTV->GetFirstCpuHandle());

	mDescriptorSRV = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flagsHeap, 1);
	device->CreateShaderResourceView(mRenderTarget.Get(), &srvDesc, mDescriptorSRV->GetFirstCpuHandle());

	if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		mDescriptorUAV= std::make_unique<DescriptorHeap>(device,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flagsHeap,  1);
		device->CreateUnorderedAccessView(mRenderTarget.Get(), nullptr, &uavDesc, mDescriptorUAV->GetFirstCpuHandle());
	}
}

void DXRSRenderTarget::TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES stateAfter)
{
	if (stateAfter != mCurrentResourceState)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), mCurrentResourceState, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
	}
}