// This code is borrowed from FeaxRenderer https://github.com/KostasAAA/FeaxRenderer

#pragma once
#include "Common.h"
#include "DXRSGraphics.h"

namespace DXRS
{
	class DescriptorHandle
	{
	public:
		DescriptorHandle()
		{
			m_CPUHandle.ptr = NULL;
			m_GPUHandle.ptr = NULL;
			m_HeapIndex = 0;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() { return m_CPUHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() { return m_GPUHandle; }
		UINT GetHeapIndex() { return m_HeapIndex; }

		void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) { m_CPUHandle = cpuHandle; }
		void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) { m_GPUHandle = gpuHandle; }
		void SetHeapIndex(UINT heapIndex) { m_HeapIndex = heapIndex; }

		bool IsValid() { return m_CPUHandle.ptr != NULL; }
		bool IsReferencedByShader() { return m_GPUHandle.ptr != NULL; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
		UINT m_HeapIndex;
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool isReferencedByShader = false);
		virtual ~DescriptorHeap();

		ID3D12DescriptorHeap* GetHeap() { return m_DescriptorHeap.Get(); }
		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() { return m_HeapType; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetHeapCPUStart() { return m_DescriptorHeapCPUStart; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetHeapGPUStart() { return m_DescriptorHeapGPUStart; }
		UINT GetMaxNoofDescriptors() { return m_MaxNoofDescriptors; }
		UINT GetDescriptorSize() { return m_DescriptorSize; }

		void AddToHandle(ID3D12Device* device, DXRS::DescriptorHandle& destCPUHandle, DXRS::DescriptorHandle& sourceCPUHandle)
		{
			device->CopyDescriptorsSimple(1, destCPUHandle.GetCPUHandle(), sourceCPUHandle.GetCPUHandle(), m_HeapType);
			destCPUHandle.GetCPUHandle().ptr += m_DescriptorSize;
		}

		void AddToHandle(ID3D12Device* device, DXRS::DescriptorHandle& destCPUHandle, D3D12_CPU_DESCRIPTOR_HANDLE& sourceCPUHandle)
		{
			device->CopyDescriptorsSimple(1, destCPUHandle.GetCPUHandle(), sourceCPUHandle, m_HeapType);
			destCPUHandle.GetCPUHandle().ptr += m_DescriptorSize;
		}

	protected:
		ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
		D3D12_CPU_DESCRIPTOR_HANDLE m_DescriptorHeapCPUStart;
		D3D12_GPU_DESCRIPTOR_HANDLE m_DescriptorHeapGPUStart;
		UINT m_MaxNoofDescriptors;
		UINT m_DescriptorSize;
		bool m_ReferencedByShader;
	};

	class CPUDescriptorHeap : public DescriptorHeap
	{
	public:
		CPUDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors);
		~CPUDescriptorHeap() final;

		DescriptorHandle GetNewHandle();
		void FreeHandle(DescriptorHandle handle);

	private:
		std::vector<UINT> m_FreeDescriptors;
		UINT m_CurrentDescriptorIndex;
		UINT m_ActiveHandleCount;
	};

	class GPUDescriptorHeap : public DescriptorHeap
	{
	public:
		GPUDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors);
		~GPUDescriptorHeap() final {};

		void Reset();
		DescriptorHandle GetHandleBlock(UINT count);

	private:
		UINT m_CurrentDescriptorIndex;
	};

	class DescriptorHeapManager
	{
	public:
		DescriptorHeapManager(ID3D12Device* device);
		~DescriptorHeapManager();

		DescriptorHandle CreateCPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		DescriptorHandle CreateGPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT count);

		GPUDescriptorHeap* GetGPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
		{
			return m_GPUDescriptorHeaps[DXRSGraphics::mBackBufferIndex][heapType];
		}

	private:
		CPUDescriptorHeap* m_CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		GPUDescriptorHeap* m_GPUDescriptorHeaps[DXRSGraphics::MAX_BACK_BUFFER_COUNT][D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	};
}

