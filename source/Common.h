#pragma once

#include <DirectXMath.h>
#include <DirectXColors.h>

#include "d3dx12.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <stdio.h>

#include "Audio.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "DDSTextureLoader.h"
#include "DescriptorHeap.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PrimitiveBatch.h"
#include "ResourceUploadBatch.h"
#include "RenderTargetState.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h" 
#include "VertexTypes.h"


// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

using namespace DirectX;
using namespace DirectX::SimpleMath;

// sorry, but I am very lazy to type std::unique_ptr<>...
template<class T> using U_PTR = std::unique_ptr<T>;

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}