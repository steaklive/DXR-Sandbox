#include "DescriptorHeap.h"
#include "DXRSGraphics.h"

namespace DXRS
{
	DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool isReferencedByShader)
		: m_HeapType(heapType)
		, m_MaxNoofDescriptors(numDescriptors)
		, m_ReferencedByShader(isReferencedByShader)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		heapDesc.NumDescriptors = m_MaxNoofDescriptors;
		heapDesc.Type = m_HeapType;
		heapDesc.Flags = m_ReferencedByShader ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

		m_DescriptorHeapCPUStart = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		if (m_ReferencedByShader)
		{
			m_DescriptorHeapGPUStart = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		}

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(m_HeapType);
	}

	DescriptorHeap::~DescriptorHeap()
	{
	}

	CPUDescriptorHeap::CPUDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors)
		: DescriptorHeap(device, heapType, numDescriptors, false)
	{
		m_CurrentDescriptorIndex = 0;
		m_ActiveHandleCount = 0;
	}

	CPUDescriptorHeap::~CPUDescriptorHeap()
	{
		m_FreeDescriptors.clear();
	}

	DescriptorHandle CPUDescriptorHeap::GetNewHandle()
	{
		UINT newHandleID = 0;

		if (m_CurrentDescriptorIndex < m_MaxNoofDescriptors)
		{
			newHandleID = m_CurrentDescriptorIndex;
			m_CurrentDescriptorIndex++;
		}
		else if (m_FreeDescriptors.size() > 0)
		{
			newHandleID = m_FreeDescriptors.back();
			m_FreeDescriptors.pop_back();
		}
		else
		{
			std::exception("Ran out of dynamic descriptor heap handles, need to increase heap size.");
		}

		DescriptorHandle newHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_DescriptorHeapCPUStart;
		cpuHandle.ptr += newHandleID * m_DescriptorSize;
		newHandle.SetCPUHandle(cpuHandle);
		newHandle.SetHeapIndex(newHandleID);
		m_ActiveHandleCount++;

		return newHandle;
	}

	void CPUDescriptorHeap::FreeHandle(DescriptorHandle handle)
	{
		m_FreeDescriptors.push_back(handle.GetHeapIndex());

		if (m_ActiveHandleCount == 0)
		{
			std::exception("Freeing heap handles when there should be none left");
		}
		m_ActiveHandleCount--;
	}

	GPUDescriptorHeap::GPUDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors)
		: DescriptorHeap(device, heapType, numDescriptors, true)
	{
		m_CurrentDescriptorIndex = 0;
	}

	DescriptorHandle GPUDescriptorHeap::GetHandleBlock(UINT count)
	{
		UINT newHandleID = 0;
		UINT blockEnd = m_CurrentDescriptorIndex + count;

		if (blockEnd < m_MaxNoofDescriptors)
		{
			newHandleID = m_CurrentDescriptorIndex;
			m_CurrentDescriptorIndex = blockEnd;
		}
		else
		{
			std::exception("Ran out of GPU descriptor heap handles, need to increase heap size.");
		}

		DescriptorHandle newHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_DescriptorHeapCPUStart;
		cpuHandle.ptr += newHandleID * m_DescriptorSize;
		newHandle.SetCPUHandle(cpuHandle);

		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_DescriptorHeapGPUStart;
		gpuHandle.ptr += newHandleID * m_DescriptorSize;
		newHandle.SetGPUHandle(gpuHandle);

		newHandle.SetHeapIndex(newHandleID);

		return newHandle;
	}

	void GPUDescriptorHeap::Reset()
	{
		m_CurrentDescriptorIndex = 0;
	}

	DescriptorHeapManager::DescriptorHeapManager(ID3D12Device* device)
	{
		ZeroMemory(m_CPUDescriptorHeaps, sizeof(m_CPUDescriptorHeaps));

		static const int MaxNoofSRVDescriptors = 4 * 4096;

		m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = new CPUDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxNoofSRVDescriptors);
		m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = new CPUDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128);
		m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = new CPUDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);
		m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = new CPUDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16);

		for (UINT i = 0; i < DXRSGraphics::MAX_BACK_BUFFER_COUNT; i++)
		{
			ZeroMemory(m_GPUDescriptorHeaps[i], sizeof(m_GPUDescriptorHeaps[i]));
			m_GPUDescriptorHeaps[i][D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = new GPUDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxNoofSRVDescriptors);
			m_GPUDescriptorHeaps[i][D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = new GPUDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16);
		}
	}

	DescriptorHeapManager::~DescriptorHeapManager()
	{
		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
		{
			if (m_CPUDescriptorHeaps[i])
				delete m_CPUDescriptorHeaps[i];

			for (UINT j = 0; j < DXRSGraphics::MAX_BACK_BUFFER_COUNT; j++)
			{
				if (m_GPUDescriptorHeaps[j][i])
					delete m_GPUDescriptorHeaps[j][i];
			}
		}
	}

	DescriptorHandle DescriptorHeapManager::CreateCPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		const UINT currentFrame = DXRSGraphics::mBackBufferIndex;

		return m_CPUDescriptorHeaps[heapType]->GetNewHandle();
	}

	DescriptorHandle DescriptorHeapManager::CreateGPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT count)
	{
		const UINT currentFrame = DXRSGraphics::mBackBufferIndex;

		return m_GPUDescriptorHeaps[currentFrame][heapType]->GetHandleBlock(count);
	}
}