#pragma once
#include "Common.h"
#include "DescriptorHeap.h"

class DXRSBuffer
{
public:
	struct DescriptorType
	{
		enum Enum
		{
			SRV = 1 << 0,
			CBV = 1 << 1,
			UAV = 1 << 2,
			Structured = 1 << 3,
			Raw = 1 << 4,
		};
	};

	struct Description
	{
		UINT m_noofElements;
		union
		{
			UINT m_elementSize;
			UINT m_size;
		};
		UINT64 m_alignment;
		DXGI_FORMAT m_format;
		UINT m_descriptorType;
		D3D12_RESOURCE_FLAGS m_resourceFlags;
		D3D12_RESOURCE_STATES m_state;
		D3D12_HEAP_TYPE m_heapType;

		Description() :
			m_noofElements(1)
			, m_elementSize(0)
			, m_alignment(0)
			, m_descriptorType(DXRSBuffer::DescriptorType::SRV)
			, m_format(DXGI_FORMAT_UNKNOWN)
			, m_resourceFlags(D3D12_RESOURCE_FLAG_NONE)
			, m_state(D3D12_RESOURCE_STATE_COMMON)
			, m_heapType(D3D12_HEAP_TYPE_DEFAULT)
		{}
	};

	DXRSBuffer(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager, ID3D12GraphicsCommandList* commandList, Description& description, LPCWSTR name = nullptr, unsigned char* data = nullptr);
	DXRSBuffer() {}
	virtual ~DXRSBuffer();

	ID3D12Resource* GetResource() { return m_buffer.Get(); }

	DXRS::DescriptorHandle& GetSRV() { return mDescriptorSRV; }
	DXRS::DescriptorHandle& GetCBV() { return mDescriptorCBV; }

	unsigned char* Map()
	{
		if (m_cbvMappedData == nullptr && m_description.m_descriptorType & DescriptorType::CBV)
		{
			CD3DX12_RANGE readRange(0, 0);
			ThrowIfFailed(m_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvMappedData)));
		}

		return m_cbvMappedData;
	}

private:
	Description m_description;

	UINT m_bufferSize;
	unsigned char* m_data;
	unsigned char* m_cbvMappedData;

	ComPtr<ID3D12Resource> m_buffer;
	ComPtr<ID3D12Resource> m_bufferUpload;

	DXRS::DescriptorHandle mDescriptorCBV;
	DXRS::DescriptorHandle mDescriptorSRV;

	void CreateResources(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager, ID3D12GraphicsCommandList* commandList);
};

