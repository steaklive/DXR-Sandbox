#include "DXRSExampleScene.h"
#include "DescriptorHeap.h"

DXRSExampleScene::DXRSExampleScene()
{
    mSandboxFramework = new DXRSGraphics();
}

DXRSExampleScene::~DXRSExampleScene()
{
	if (mSandboxFramework)
		mSandboxFramework->WaitForGpu();

    delete mLightingCB;
    delete mLightsInfoCB;

    //delete mDepthStencil;
}

void DXRSExampleScene::Init(HWND window, int width, int height)
{
    mGamePad = std::make_unique<DirectX::GamePad>();
    mKeyboard = std::make_unique<DirectX::Keyboard>();
    mMouse = std::make_unique<DirectX::Mouse>();
    mMouse->SetWindow(window);

    mSandboxFramework->SetWindow(window, width, height);

    mSandboxFramework->CreateResources();
    mStates = std::make_unique<CommonStates>(mSandboxFramework->GetD3DDevice());
    mGraphicsMemory = std::make_unique<GraphicsMemory>(mSandboxFramework->GetD3DDevice());

    mSandboxFramework->CreateWindowResources();
    CreateWindowResources();

    ID3D12Device* device = mSandboxFramework->GetD3DDevice();

    // create a null descriptor for unbound textures
    auto descriptorManager = mSandboxFramework->GetDescriptorHeapManager();
    mNullDescriptor = descriptorManager->CreateCPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(nullptr, &srvDesc, mNullDescriptor.GetCPUHandle());

    //create dragon model
    CreateDragonMeshResources();
    mDragonModel = U_PTR<DXRSModel>(new DXRSModel(*mSandboxFramework, "C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\models\\dragon.fbx", true));

    mDepthStencil = new DXRSDepthBuffer(device, descriptorManager, width, height, DXGI_FORMAT_D32_FLOAT);

    //states
    D3D12_DEPTH_STENCIL_DESC depthStateRW;
    depthStateRW.DepthEnable = TRUE;
    depthStateRW.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStateRW.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStateRW.StencilEnable = FALSE;
    depthStateRW.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStateRW.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStateRW.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateRW.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStateRW.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStateRW.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStateRW.BackFace = depthStateRW.FrontFace;
  
    D3D12_DEPTH_STENCIL_DESC depthStateDisabled;
    depthStateDisabled.DepthEnable = FALSE;
    depthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateDisabled.StencilEnable = FALSE;
    depthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStateDisabled.BackFace = depthStateRW.FrontFace;

    D3D12_BLEND_DESC blend = {};
    blend.IndependentBlendEnable = FALSE;
    blend.RenderTarget[0].BlendEnable = FALSE;
    blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RASTERIZER_DESC rasterizer;
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    //create resources for g-buffer pass 
    {
        D3D12_SAMPLER_DESC sampler;
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;

        DXGI_FORMAT rtFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, width, height, rtFormat, flags, L"Albedo"));

        rtFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
        mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, width, height, rtFormat, flags, L"Normals"));

        // root signature
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        mGbufferRS.Reset(2, 1);
        mGbufferRS.InitStaticSampler(0, sampler, D3D12_SHADER_VISIBILITY_PIXEL);
        mGbufferRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
        mGbufferRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);
        mGbufferRS.Finalize(device, L"GPrepassRS", rootSignatureFlags);

        //Create Pipeline State Object
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ID3DBlob* errorBlob = nullptr;

        ThrowIfFailed(D3DCompileFromFile(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\GBuffer.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));

        compileFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

        ThrowIfFailed(D3DCompileFromFile(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\GBuffer.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        DXGI_FORMAT formats[2];
        formats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        formats[1] = DXGI_FORMAT_R16G16B16A16_SNORM;

        mGbufferPSO.SetRootSignature(mGbufferRS);
        mGbufferPSO.SetRasterizerState(rasterizer);
        mGbufferPSO.SetBlendState(blend);
        mGbufferPSO.SetDepthStencilState(depthStateRW);
        mGbufferPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
        mGbufferPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        mGbufferPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
        mGbufferPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
        mGbufferPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
        mGbufferPSO.Finalize(device);

        //create constant buffer for pass
        DXRSBuffer::Description cbDesc;
        cbDesc.m_elementSize = sizeof(GBufferCBData);
        cbDesc.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
        cbDesc.m_descriptorType = DXRSBuffer::DescriptorType::CBV;

        mGbufferCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandList(), cbDesc, L"GBuffer CB");

    }

    // lighting pass
    {
        //RTs
        mLightingRTs.push_back(new DXRSRenderTarget(device, descriptorManager, width, height, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, L"Light Diffuse"));
        mLightingRTs.push_back(new DXRSRenderTarget(device, descriptorManager, width, height, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, L"Light Specular"));

        //create root signature
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            //	D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        mLightingRS.Reset(2, 0);
        mLightingRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
        mLightingRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
        mLightingRS.Finalize(device, L"Lighting pass RS", rootSignatureFlags);

        //PSO
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        ID3DBlob* errorBlob = nullptr;

        ThrowIfFailed(D3DCompileFromFile(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\Lighting.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob));
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        ThrowIfFailed(D3DCompileFromFile(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\Lighting.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob));
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        DXGI_FORMAT m_rtFormats[2];
        m_rtFormats[0] = DXGI_FORMAT_R11G11B10_FLOAT;
        m_rtFormats[1] = DXGI_FORMAT_R11G11B10_FLOAT;

        mLightingPSO.SetRootSignature(mLightingRS);
        mLightingPSO.SetRasterizerState(rasterizer);
        mLightingPSO.SetBlendState(blend);
        mLightingPSO.SetDepthStencilState(depthStateDisabled);
        mLightingPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
        mLightingPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        mLightingPSO.SetRenderTargetFormats(_countof(m_rtFormats), m_rtFormats, DXGI_FORMAT_D32_FLOAT);
        mLightingPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
        mLightingPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
        mLightingPSO.Finalize(device);

        //CB
        DXRSBuffer::Description cbDesc;
        cbDesc.m_elementSize = sizeof(LightingCBData);
        cbDesc.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
        cbDesc.m_descriptorType = DXRSBuffer::DescriptorType::CBV;

        mLightingCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandList(), cbDesc, L"Lighting Pass CB");

        cbDesc.m_elementSize = sizeof(LightsInfoCBData);
        mLightsInfoCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandList(), cbDesc, L"Lights Info CB");
    }

    //create resources for composite pass
    {
        //create root signature
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        mCompositeRS.Reset(1, 0);
        //mCompositeRS.InitStaticSampler(0, SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        mCompositeRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, D3D12_SHADER_VISIBILITY_PIXEL);
        mCompositeRS.Finalize(device, L"Composite RS", rootSignatureFlags);

        //create pipeline state object
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        ID3DBlob* errorBlob = nullptr;

        ThrowIfFailed(D3DCompileFromFile(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\Composite.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob));
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        ThrowIfFailed(D3DCompileFromFile(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\Composite.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob));

        mCompositePSO = mLightingPSO;

        mCompositePSO.SetRootSignature(mCompositeRS);
        mCompositePSO.SetRenderTargetFormat(mSandboxFramework->GetBackBufferFormat(), DXGI_FORMAT_D32_FLOAT);
        mCompositePSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
        mCompositePSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
        mCompositePSO.Finalize(device);
    }
}

void DXRSExampleScene::Clear()
{
    auto commandList = mSandboxFramework->GetCommandList();

    auto rtvDescriptor = mSandboxFramework->GetRenderTargetView();
    auto dsvDescriptor = mSandboxFramework->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::Gray, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    auto viewport = mSandboxFramework->GetScreenViewport();
    auto scissorRect = mSandboxFramework->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void DXRSExampleScene::Run()
{
    mTimer.Run([&]()
    {
        Update(mTimer);
    });

    Render();
}

void DXRSExampleScene::Update(DXRSTimer const& timer)
{
    Vector3 eye(0.0f, 1.0f, 1.5f);
    Vector3 at(0.0f, 1.0f, 0.0f);

    mView = Matrix::CreateLookAt(eye, at, Vector3::UnitY);

    mWorld = XMMatrixIdentity();

    //mDragonBasicEffectXTK->SetView(mView);
    float width = mSandboxFramework->GetOutputSize().right;
    float height = mSandboxFramework->GetOutputSize().bottom;
    GBufferCBData gbufferPassData;
    gbufferPassData.ViewProjection = mView * mProjection;
    gbufferPassData.InvViewProjection = XMMatrixInverse(nullptr, gbufferPassData.ViewProjection);
    gbufferPassData.MipBias = 0.0f;
    gbufferPassData.CameraPos = XMFLOAT4(eye.x,eye.y,eye.z,1);
    gbufferPassData.RTSize = { width, height, 1.0f / width, 1.0f / height };
    memcpy(mGbufferCB->Map(), &gbufferPassData, sizeof(gbufferPassData));

    LightingCBData lightPassData = {};
    lightPassData.InvViewProjection = XMMatrixInverse(nullptr, gbufferPassData.ViewProjection);
    lightPassData.CameraPos = XMFLOAT4(eye.x, eye.y, eye.z, 1);
    lightPassData.RTSize = { width, height, 1.0f / width, 1.0f / height };
    memcpy(mLightingCB->Map(), &lightPassData, sizeof(lightPassData));

    UpdateLights();

    //TODO map model cb
    XMMATRIX local = mWorld * Matrix::CreateScale(0.3f, 0.3f, 0.3f) *  Matrix::CreateTranslation(0, 0, -10.0f);
    mDragonModel->UpdateWorldMatrix(local);

    auto pad = mGamePad->GetState(0);
    if (pad.IsConnected())
    {
        mGamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            //ExitSample();
        }
    }
    else
    {
        mGamePadButtons.Reset();
    }

    auto kb = mKeyboard->GetState();
    mKeyboardButtons.Update(kb);

    if (kb.Escape)
    {
        //ExitSample();
    }

    auto mouse = mMouse->GetState();
    mouse;
}

void DXRSExampleScene::UpdateLights()
{
    XMVECTOR skyColour = XMVectorSet(0.9f, 0.8f, 1.0f, 0.0f);

    //TODO move from update
    LightsInfoCBData lightData = {};
    DirectionalLightData dirLight = {};
    dirLight.Colour = XMFLOAT4(mDirectionalLightColor[0],mDirectionalLightColor[1],mDirectionalLightColor[2],mDirectionalLightColor[3]);
    dirLight.Direction = XMFLOAT4(mDirectionalLightDir[0], mDirectionalLightDir[1], mDirectionalLightDir[2], mDirectionalLightDir[3]);
    dirLight.Intensity = mDirectionalLightIntensity;

    lightData.DirectionalLight.Colour = dirLight.Colour;
    lightData.DirectionalLight.Direction = dirLight.Direction;
    lightData.DirectionalLight.Intensity = dirLight.Intensity;

    memcpy(mLightsInfoCB->Map(), &lightData, sizeof(lightData));
}

void DXRSExampleScene::Render()
{
    if (mTimer.GetFrameCount() == 0)
        return;

    // Prepare the command list to render a new frame.
    mSandboxFramework->Prepare();
    Clear();

    auto commandList = mSandboxFramework->GetCommandList();
    auto device = mSandboxFramework->GetD3DDevice();
    auto descriptorHeapManager = mSandboxFramework->GetDescriptorHeapManager();

    DXRS::GPUDescriptorHeap* gpuDescriptorHeap = descriptorHeapManager->GetGPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    gpuDescriptorHeap->Reset();

    ID3D12DescriptorHeap* ppHeaps[] = { gpuDescriptorHeap->GetHeap() };

    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
    CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &rect);

    //gbuffer
    commandList->SetPipelineState(mGbufferPSO.GetPipelineStateObject());
    commandList->SetGraphicsRootSignature(mGbufferRS.GetSignature());

    //transition buffers to rendertarget outputs
    mSandboxFramework->ResourceBarriersBegin(mBarriers);
    mGbufferRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mGbufferRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mLightingRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mLightingRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
   // mDepthStencil->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] =
    {
        mGbufferRTs[0]->GetRTV().GetCPUHandle(),
        mGbufferRTs[1]->GetRTV().GetCPUHandle()
    };

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDepthStencil->GetDSV().GetCPUHandle());
    commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &/*dsvHandle*/ mSandboxFramework->GetDepthStencilView());

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float clearNormal[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(rtvHandles[0], clearColor, 0, nullptr);
    commandList->ClearRenderTargetView(rtvHandles[1], clearNormal, 0, nullptr);
    //commandList->ClearDepthStencilView(mDepthStencil->GetDSV().GetCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    DXRS::DescriptorHandle cbvHandle;
    DXRS::DescriptorHandle srvHandle;

    srvHandle = gpuDescriptorHeap->GetHandleBlock(0);

    for (int i = 0; i < 1; i++)
    {
       //if (i < textures.size())
       //{
       //    gpuDescriptorHeap->AddToHandle(srvHandle, textures[i]->GetSRV());
       //}
       //else
        {
            gpuDescriptorHeap->AddToHandle(device, srvHandle, mNullDescriptor);
        }
    }

    //for (ModelInstance* modelInstance : scene->GetModelInstances())
    {
        cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
        gpuDescriptorHeap->AddToHandle(device, cbvHandle, mGbufferCB->GetCBV());
        gpuDescriptorHeap->AddToHandle(device, cbvHandle, mDragonModel->GetCB()->GetCBV());

        commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
        commandList->SetGraphicsRootDescriptorTable(1, srvHandle.GetGPUHandle());

        mDragonModel->Render(commandList);
    }

    //transition rendertargets to shader resources 
    //mSandboxFramework->ResourceBarriersBegin(mBarriers);
    //mDepthStencil->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

    // lighting pass
    const float clearColorLighting[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesLighting[] =
    {
        mLightingRTs[0]->GetRTV().GetCPUHandle(),
        mLightingRTs[1]->GetRTV().GetCPUHandle()
    };

    commandList->OMSetRenderTargets(_countof(rtvHandlesLighting), rtvHandlesLighting, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvHandlesLighting[0], clearColorLighting, 0, nullptr);
    commandList->ClearRenderTargetView(rtvHandlesLighting[1], clearColorLighting, 0, nullptr);
    commandList->SetPipelineState(mLightingPSO.GetPipelineStateObject());
    commandList->SetGraphicsRootSignature(mLightingRS.GetSignature());

    DXRS::DescriptorHandle cbvHandleLighting = gpuDescriptorHeap->GetHandleBlock(2);
    gpuDescriptorHeap->AddToHandle(device, cbvHandleLighting, mLightingCB->GetCBV());
    gpuDescriptorHeap->AddToHandle(device, cbvHandleLighting, mLightsInfoCB->GetCBV());

    DXRS::DescriptorHandle srvHandleLighting = gpuDescriptorHeap->GetHandleBlock(3);
    gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mGbufferRTs[0]->GetSRV());
    gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mGbufferRTs[1]->GetSRV());
    gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mDepthStencil->GetSRV());

    commandList->SetGraphicsRootDescriptorTable(0, cbvHandleLighting.GetGPUHandle());
    commandList->SetGraphicsRootDescriptorTable(1, srvHandleLighting.GetGPUHandle());

    commandList->IASetVertexBuffers(0, 1,  &mSandboxFramework->GetFullscreenQuadBufferView());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->DrawInstanced(4, 1, 0, 0);

    //mSandboxFramework->ResourceBarriersBegin(mBarriers);
    //mLightingRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mLightingRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

    // composite and copy to backbuffer
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesFinal[] =
    {
         mSandboxFramework->GetRenderTargetView()
    };

    commandList->OMSetRenderTargets(_countof(rtvHandlesFinal), rtvHandlesFinal, FALSE, nullptr);
    commandList->SetPipelineState(mCompositePSO.GetPipelineStateObject());
    commandList->SetGraphicsRootSignature(mCompositeRS.GetSignature());

    DXRS::DescriptorHandle srvHandleComposite = gpuDescriptorHeap->GetHandleBlock(2);
    gpuDescriptorHeap->AddToHandle(device, srvHandleComposite, mLightingRTs[0]->GetSRV());
    gpuDescriptorHeap->AddToHandle(device, srvHandleComposite, mLightingRTs[1]->GetSRV());

    commandList->SetGraphicsRootDescriptorTable(0, srvHandleComposite.GetGPUHandle());
    commandList->IASetVertexBuffers(0, 1, &mSandboxFramework->GetFullscreenQuadBufferView());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->DrawInstanced(4, 1, 0, 0);

    //mSandboxFramework->ResourceBarriersBegin(mBarriers);
    //mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);
    // Show the new frame.
    mSandboxFramework->Present();
    mGraphicsMemory->Commit(mSandboxFramework->GetCommandQueue());
}

void DXRSExampleScene::CreateDragonMeshResources()
{
    auto device = mSandboxFramework->GetD3DDevice();

    //mResourceDescriptors = std::make_unique<DescriptorHeap>(device,
    //    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    //    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    //    DragonDescriptors::Count);

    //{
    //    ResourceUploadBatch resourceUpload(device);

    //    resourceUpload.Begin();

    //    ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\textures\\seafloor.dds", mDragonTextureAlbedo.ReleaseAndGetAddressOf()));

    //    CreateShaderResourceView(device, mDragonTextureAlbedo.Get(), mResourceDescriptors->GetCpuHandle(DragonDescriptors::SeaFloor));

    //    RenderTargetState rtState(mSandboxFramework->GetBackBufferFormat(), mSandboxFramework->GetDepthBufferFormat());

    //    {
    //        EffectPipelineStateDescription pd(
    //            &GeometricPrimitive::VertexType::InputLayout,
    //            CommonStates::Opaque,
    //            CommonStates::DepthDefault,
    //            CommonStates::CullNone,
    //            rtState);

    //        mDragonBasicEffectXTK = std::make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
    //        mDragonBasicEffectXTK->EnableDefaultLighting();
    //        mDragonBasicEffectXTK->SetTexture(mResourceDescriptors->GetGpuHandle(DragonDescriptors::SeaFloor), mStates->AnisotropicWrap());
    //    }

    //    // Upload the resources to the GPU.
    //    auto uploadResourcesFinished = resourceUpload.End(mSandboxFramework->GetCommandQueue());
    //    // Wait for the upload thread to terminate
    //    uploadResourcesFinished.wait();
    //}
}

void DXRSExampleScene::CreateWindowResources()
{
    auto size = mSandboxFramework->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    mProjection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );

   // mDragonBasicEffectXTK->SetProjection(mProjection);
}

void DXRSExampleScene::OnWindowSizeChanged(int width, int height)
{
    if (!mSandboxFramework->WindowSizeChanged(width, height))
        return;

    CreateWindowResources();
}

