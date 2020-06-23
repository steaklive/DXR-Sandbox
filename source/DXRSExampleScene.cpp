#include "DXRSExampleScene.h"

#include "DescriptorHeap.h"

D3D12_HEAP_PROPERTIES UploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
D3D12_HEAP_PROPERTIES DefaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

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
    mSandboxFramework->CreateFullscreenQuadBuffers();

    mDragonModel = U_PTR<DXRSModel>(new DXRSModel(*mSandboxFramework, "C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\models\\dragon.fbx", true, XMMatrixIdentity(), XMFLOAT4(0, 1, 0, 1)));
    mPlaneModel = U_PTR<DXRSModel>(new DXRSModel(*mSandboxFramework, "C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\models\\plane.fbx", true, XMMatrixIdentity(), XMFLOAT4(0.5,0.2,0.6,1)));

    CreateRaytracingAccelerationStructures();
    CreateRaytracingShaders();
    CreateRaytracingPSO();

    mSandboxFramework->FinalizeResources();
    ID3D12Device* device = mSandboxFramework->GetD3DDevice();

    //RT output buffer
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.DepthOrArraySize = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Width = 1920;
    resDesc.Height = 1080;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    ThrowIfFailed(device->CreateCommittedResource(&DefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&mRaytracingOutputResource)));

    //camera buffer
    D3D12_RESOURCE_DESC cambufDesc = {};
    cambufDesc.Alignment = 0;
    cambufDesc.DepthOrArraySize = 1;
    cambufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cambufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    cambufDesc.Format = DXGI_FORMAT_UNKNOWN;
    cambufDesc.Height = 1;
    cambufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    cambufDesc.MipLevels = 1;
    cambufDesc.SampleDesc.Count = 1;
    cambufDesc.SampleDesc.Quality = 0;
    cambufDesc.Width = 4 * sizeof(XMMATRIX);
    ThrowIfFailed(device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &cambufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mCameraBuffer)));

    CreateRaytracingResourceHeap();
    CreateRaytracingShaderTable();

    mStates = std::make_unique<CommonStates>(device);
    mGraphicsMemory = std::make_unique<GraphicsMemory>(device);

    mSandboxFramework->CreateWindowResources();
    SetProjectionMatrix();

    // create a null descriptor for unbound textures
    auto descriptorManager = mSandboxFramework->GetDescriptorHeapManager();
    mNullDescriptor = descriptorManager->CreateCPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(nullptr, &srvDesc, mNullDescriptor.GetCPUHandle());

    mDepthStencil = new DXRSDepthBuffer(device, descriptorManager, 1920, 1080, DXGI_FORMAT_D32_FLOAT);

    #pragma region States
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
#pragma endregion

    // create resources for g-buffer pass 
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

        mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, 1920, 1080, rtFormat, flags, L"Albedo"));

        rtFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
        mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, 1920, 1080, rtFormat, flags, L"Normals"));

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
        cbDesc.mElementSize = sizeof(GBufferCBData);
        cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
        cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

        mGbufferCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandList(), cbDesc, L"GBuffer CB");

    }

    // create resources lighting pass
    {
        //RTs
        mLightingRTs.push_back(new DXRSRenderTarget(device, descriptorManager, 1920, 1080, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, L"Light Diffuse"));
        mLightingRTs.push_back(new DXRSRenderTarget(device, descriptorManager, 1920, 1080, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, L"Light Specular"));

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
        cbDesc.mElementSize = sizeof(LightingCBData);
        cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
        cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

        mLightingCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandList(), cbDesc, L"Lighting Pass CB");

        cbDesc.mElementSize = sizeof(LightsInfoCBData);
        mLightsInfoCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandList(), cbDesc, L"Lights Info CB");
    }

    // create resources composite pass
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

void DXRSExampleScene::CreateRaytracingPSO()
{
    ID3D12Device5* device = mSandboxFramework->GetDXRDevice();
    RayTracingPipelineGenerator pipeline(device);

    pipeline.AddLibrary(mRaygenBlob, { L"RayGen" });
    pipeline.AddLibrary(mMissBlob, { L"Miss" });
    pipeline.AddLibrary(mClosestHitBlob, { L"ClosestHitObject", L"ClosestHitPlane" });
    
    pipeline.AddHitGroup(L"PlaneHitGroup", L"ClosestHitPlane");
    pipeline.AddHitGroup(L"HitGroup", L"ClosestHitObject");
    
    pipeline.AddRootSignatureAssociation(mRaygenRS.GetSignature(), { L"RayGen" });
    pipeline.AddRootSignatureAssociation(mMissRS.GetSignature(), { L"Miss" });
    pipeline.AddRootSignatureAssociation(mClosestHitRS.GetSignature(), { L"PlaneHitGroup" , L"HitGroup" });
    
    pipeline.SetMaxPayloadSize(sizeof(XMFLOAT4)); // RGB + distance
    pipeline.SetMaxAttributeSize(sizeof(XMFLOAT2)); // barycentric coordinates
    pipeline.SetMaxRecursionDepth(1);
    
    // Compile the pipeline for execution on the GPU
    mRaytracingPSO = pipeline.Generate();
    
    // Cast the state object into a properties object, allowing to later access
    // the shader pointers by name
    ThrowIfFailed(
        mRaytracingPSO->QueryInterface(IID_PPV_ARGS(&mRaytracingPSOProperties)));
}

void DXRSExampleScene::CreateRaytracingAccelerationStructures()
{
    ID3D12Device5* device = mSandboxFramework->GetDXRDevice();
    ID3D12GraphicsCommandList4* commandList = (ID3D12GraphicsCommandList4*)mSandboxFramework->GetCommandList();

    //Create BLAS
    {
        //plane mesh
        {
            DXRSMesh* mesh = mPlaneModel->Meshes()[0];

            D3D12_RAYTRACING_GEOMETRY_DESC desc;
            desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Triangles.VertexBuffer.StartAddress = mesh->GetVertexBuffer()->GetGPUVirtualAddress();
            desc.Triangles.VertexBuffer.StrideInBytes = mesh->GetVertexBufferView().StrideInBytes;
            desc.Triangles.VertexCount = static_cast<UINT>(mesh->Vertices().size());
            desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            desc.Triangles.IndexBuffer = mesh->GetIndexBuffer()->GetGPUVirtualAddress();
            desc.Triangles.IndexFormat = mesh->GetIndexBufferView().Format;
            desc.Triangles.IndexCount = static_cast<UINT>(mesh->Indices().size());
            desc.Triangles.Transform3x4 = 0;
            desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

            // Get the size requirements for the BLAS buffers
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
            ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            ASInputs.pGeometryDescs = &desc;
            ASInputs.NumDescs = 1;
            ASInputs.Flags = buildFlags;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
            device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

            ASPreBuildInfo.ScratchDataSizeInBytes = Align(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
            ASPreBuildInfo.ResultDataMaxSizeInBytes = Align(ASPreBuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

            // Create the BLAS scratch buffer
            DXRSBuffer::Description blasdesc;
            blasdesc.mAlignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
            blasdesc.mSize = (UINT)ASPreBuildInfo.ScratchDataSizeInBytes;
            blasdesc.mState = D3D12_RESOURCE_STATE_COMMON;
            blasdesc.mDescriptorType = DXRSBuffer::DescriptorType::Raw;
            blasdesc.mResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            DXRSBuffer* blasScratchBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, blasdesc, L"BLAS Scratch Buffer");

            // Create the BLAS buffer
            blasdesc.mElementSize = (UINT)ASPreBuildInfo.ResultDataMaxSizeInBytes;
            blasdesc.mState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

            DXRSBuffer* blasBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, blasdesc, L"BLAS Buffer");

            // Describe and build the bottom level acceleration structure
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
            buildDesc.Inputs = ASInputs;
            buildDesc.ScratchAccelerationStructureData = blasScratchBuffer->GetResource()->GetGPUVirtualAddress();
            buildDesc.DestAccelerationStructureData = blasBuffer->GetResource()->GetGPUVirtualAddress();

            commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

            // Wait for the BLAS build to complete
            D3D12_RESOURCE_BARRIER uavBarrier;
            uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarrier.UAV.pResource = blasBuffer->GetResource();
            uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            commandList->ResourceBarrier(1, &uavBarrier);

            mPlaneModel->SetBlasBuffer(blasBuffer);

            //delete blasScratchBuffer;
        }
        //dragon mesh
        {
            DXRSMesh* mesh = mDragonModel->Meshes()[0];
        
            D3D12_RAYTRACING_GEOMETRY_DESC desc;
            desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Triangles.VertexBuffer.StartAddress = mesh->GetVertexBuffer()->GetGPUVirtualAddress();
            desc.Triangles.VertexBuffer.StrideInBytes = mesh->GetVertexBufferView().StrideInBytes;
            desc.Triangles.VertexCount = static_cast<UINT>(mesh->Vertices().size());
            desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            desc.Triangles.IndexBuffer = mesh->GetIndexBuffer()->GetGPUVirtualAddress();
            desc.Triangles.IndexFormat = mesh->GetIndexBufferView().Format;
            desc.Triangles.IndexCount = static_cast<UINT>(mesh->Indices().size());
            desc.Triangles.Transform3x4 = 0;
            desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        
            // Get the size requirements for the BLAS buffers
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
            ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            ASInputs.pGeometryDescs = &desc;
            ASInputs.NumDescs = 1;
            ASInputs.Flags = buildFlags;
        
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
            device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);
        
            ASPreBuildInfo.ScratchDataSizeInBytes = Align(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
            ASPreBuildInfo.ResultDataMaxSizeInBytes = Align(ASPreBuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        
            // Create the BLAS scratch buffer
            DXRSBuffer::Description blasdesc;
            blasdesc.mAlignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
            blasdesc.mSize = (UINT)ASPreBuildInfo.ScratchDataSizeInBytes;
            blasdesc.mState = D3D12_RESOURCE_STATE_COMMON;
            blasdesc.mDescriptorType = DXRSBuffer::DescriptorType::Raw;
            blasdesc.mResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        
            DXRSBuffer* blasScratchBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, blasdesc, L"BLAS Scratch Buffer");
        
            // Create the BLAS buffer
            blasdesc.mElementSize = (UINT)ASPreBuildInfo.ResultDataMaxSizeInBytes;
            blasdesc.mState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        
            DXRSBuffer* blasBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, blasdesc, L"BLAS Buffer");
        
            // Describe and build the bottom level acceleration structure
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
            buildDesc.Inputs = ASInputs;
            buildDesc.ScratchAccelerationStructureData = blasScratchBuffer->GetResource()->GetGPUVirtualAddress();
            buildDesc.DestAccelerationStructureData = blasBuffer->GetResource()->GetGPUVirtualAddress();
            
            commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        
            // Wait for the BLAS build to complete
            D3D12_RESOURCE_BARRIER uavBarrier;
            uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarrier.UAV.pResource = blasBuffer->GetResource();
            uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            commandList->ResourceBarrier(1, &uavBarrier);
        
            mDragonModel->SetBlasBuffer(blasBuffer);
        
            //delete blasScratchBuffer;
        }
    }

    //Create TLAS
    {
        D3D12_RAYTRACING_INSTANCE_DESC* instanceDescriptions = new D3D12_RAYTRACING_INSTANCE_DESC[2]; //plane and dragon
        int noofInstances = 0;
        //plane
        {

            D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescriptions[noofInstances];
            // Describe the TLAS geometry instance(s)
            instanceDesc.InstanceID = noofInstances;// This value is exposed to shaders as SV_InstanceID
            instanceDesc.InstanceContributionToHitGroupIndex = 0;
            instanceDesc.InstanceMask = 0xFF;

            DirectX::XMMATRIX m = XMMatrixTranspose(mWorld * Matrix::CreateScale(8.0f, 8.0f, 8.0f) * Matrix::CreateTranslation(0, 0, 0.0f) * Matrix::CreateRotationX(-3.14f / 2.0f));
            memcpy(instanceDesc.Transform, &m, sizeof(instanceDesc.Transform));

            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            instanceDesc.AccelerationStructure = mPlaneModel->GetBlasBuffer()->GetResource()->GetGPUVirtualAddress();
            noofInstances++;
        }
        //dragon
        {

            D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescriptions[noofInstances];
            // Describe the TLAS geometry instance(s)
            instanceDesc.InstanceID = noofInstances;// This value is exposed to shaders as SV_InstanceID
            instanceDesc.InstanceContributionToHitGroupIndex = noofInstances;
            instanceDesc.InstanceMask = 0xFF;

            DirectX::XMMATRIX m = XMMatrixTranspose(mWorld * Matrix::CreateScale(0.3f, 0.3f, 0.3f) * Matrix::CreateTranslation(0, 0, 0.0f));
            memcpy(instanceDesc.Transform, &m, sizeof(instanceDesc.Transform));

            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            instanceDesc.AccelerationStructure = mDragonModel->GetBlasBuffer()->GetResource()->GetGPUVirtualAddress();
            noofInstances++;
        }

        DXRSBuffer::Description desc;
        desc.mSize = noofInstances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        desc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
        desc.mResourceFlags = D3D12_RESOURCE_FLAG_NONE;
        desc.mHeapType = D3D12_HEAP_TYPE_UPLOAD;
        desc.mDescriptorType = DXRSBuffer::DescriptorType::Raw;
        DXRSBuffer* instanceDescriptionBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, desc, L"Instance Description Buffer");

        // Copy the instance data to the buffer
        D3D12_RAYTRACING_INSTANCE_DESC* data;
        instanceDescriptionBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&data));
        memcpy(data, instanceDescriptions, noofInstances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        instanceDescriptionBuffer->GetResource()->Unmap(0, nullptr);

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

        // Get the size requirements for the TLAS buffers
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
        ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        ASInputs.InstanceDescs = instanceDescriptionBuffer->GetResource()->GetGPUVirtualAddress();
        ASInputs.NumDescs = noofInstances;
        ASInputs.Flags = buildFlags;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

        ASPreBuildInfo.ResultDataMaxSizeInBytes = Align(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);
        ASPreBuildInfo.ScratchDataSizeInBytes = Align(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);

        // Create TLAS scratch buffer
        DXRSBuffer::Description tlasDesc;
        tlasDesc.mAlignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        tlasDesc.mElementSize = (UINT)ASPreBuildInfo.ScratchDataSizeInBytes;
        tlasDesc.mState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        tlasDesc.mResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        tlasDesc.mDescriptorType = DXRSBuffer::DescriptorType::Raw;

        DXRSBuffer* tlasScratchBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, tlasDesc, L"TLAS Scratch Buffer");

        // Create the TLAS buffer
        tlasDesc.mElementSize = (UINT)ASPreBuildInfo.ResultDataMaxSizeInBytes;
        tlasDesc.mState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        DXRSBuffer* tlasBuffer = new DXRSBuffer(device, mSandboxFramework->GetDescriptorHeapManager(), commandList, tlasDesc, L"TLAS Buffer");

        // Describe and build the TLAS
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs = ASInputs;
        buildDesc.ScratchAccelerationStructureData = tlasScratchBuffer->GetResource()->GetGPUVirtualAddress();
        buildDesc.DestAccelerationStructureData = tlasBuffer->GetResource()->GetGPUVirtualAddress();

        commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        // Wait for the TLAS build to complete
        D3D12_RESOURCE_BARRIER uavBarrier;
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = tlasBuffer->GetResource();
        uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        commandList->ResourceBarrier(1, &uavBarrier);

        mTLASBuffer = tlasBuffer;
    }

}

void DXRSExampleScene::CreateRaytracingShaders()
{
    auto device = mSandboxFramework->GetDXRDevice();

    //raytracing shaders have to have local root signature
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    //compile raygen shader
    {
        mRaygenBlob = mSandboxFramework->CompileShaderLibrary(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\RayGen.hlsl");

        // create root signature
        mRaygenRS.Reset(1, 0);
        mRaygenRS[0].InitAsDescriptorTable(3);
        mRaygenRS[0].SetTableRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
        mRaygenRS[0].SetTableRange(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
        mRaygenRS[0].SetTableRange(2, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, 0);
        mRaygenRS.Finalize(device, L"Raygen RS", rootSignatureFlags);
    }

    //compile hit shader
    {
        mClosestHitBlob = mSandboxFramework->CompileShaderLibrary(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\Hit.hlsl");

        // create root signature
        mClosestHitRS.Reset(1, 0);
        mClosestHitRS[0].InitAsBufferSRV(0);
        mRaygenRS.Finalize(device, L"Closest Hit RS", rootSignatureFlags);
    }

    //compile miss shader
    {
        mMissBlob = mSandboxFramework->CompileShaderLibrary(L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\shaders\\Miss.hlsl");
        
        mMissRS.Reset(0, 0);
        mMissRS.Finalize(device, L"Miss RS", rootSignatureFlags);
    }
}

void DXRSExampleScene::CreateRaytracingShaderTable()
{
    auto device = mSandboxFramework->GetD3DDevice();

    // The SBT helper class collects calls to Add*Program.  If called several
    // times, the helper must be emptied before re-adding shaders.
    mRaytracingShaderBindingTableHelper.Reset();

    // The pointer to the beginning of the heap is the only parameter required by
    // shaders without root parameters
    D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = mRaytracingDescriptorHeap->GetGPUDescriptorHandleForHeapStart() /*GetHeapGPUStart()*/;
    
    // The helper treats both root parameter pointers and heap pointers as void*,
    // while DX12 uses the
    // D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this
    // struct is a UINT64, which then has to be reinterpreted as a pointer.
    auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);
    
    // The ray generation only uses heap data
    mRaytracingShaderBindingTableHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });
    
    // The miss and hit shaders do not access any external resources: instead they
    // communicate their results through the ray payload
    mRaytracingShaderBindingTableHelper.AddMissProgram(L"Miss", {});
    
    // Adding the mesh hit shader
    mRaytracingShaderBindingTableHelper.AddHitGroup(L"PlaneHitGroup", { (void*)(mPlaneModel->Meshes()[0]->GetVertexBuffer()) });
    mRaytracingShaderBindingTableHelper.AddHitGroup(L"HitGroup", { (void*)(mDragonModel->Meshes()[0]->GetVertexBuffer()) });
    
    // Compute the size of the SBT given the number of shaders and their
    // parameters
    uint32_t sbtSize = mRaytracingShaderBindingTableHelper.ComputeSBTSize();

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Alignment = 0;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.Height = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Width = sbtSize;
    
    ThrowIfFailed(device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mRaytracingShaderTableBuffer)));
    
    // Compile the SBT from the shader and parameters info
    mRaytracingShaderBindingTableHelper.Generate(mRaytracingShaderTableBuffer.Get(), mRaytracingPSOProperties.Get());
}

void DXRSExampleScene::CreateRaytracingResourceHeap()
{
    auto device = mSandboxFramework->GetD3DDevice();
    auto descriptorHeapManager = mSandboxFramework->GetDescriptorHeapManager();

    // Create a SRV/UAV/CBV descriptor heap. We need 3 entries
    // 1 - UAV for the RT output
    // 1 - SRV for the TLAS
    // 1 - for CBV 
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 3;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mRaytracingDescriptorHeap)));

    // Get a handle to the heap memory on the CPU side, to be able to write the
    // descriptors directly
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mRaytracingDescriptorHeap->GetCPUDescriptorHandleForHeapStart()/*GetHeapCPUStart()*/;

    // Create the UAV. Based on the root signature we created it is the first
    // entry. The Create*View methods write the view information directly into
    // srvHandle
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(mRaytracingOutputResource.Get(), nullptr, &uavDesc, srvHandle);

    // Add the Top Level AS SRV right after the raytracing output buffer
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = mTLASBuffer->GetResource()->GetGPUVirtualAddress();
    // Write the acceleration structure view in the heap
    device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

    // Add for CBV
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // Describe and create a constant buffer view for the camera
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = mCameraBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = 4 * sizeof(XMMATRIX);
    device->CreateConstantBufferView(&cbvDesc, srvHandle);
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

    //DXR dispatch
    {
        ID3D12GraphicsCommandList4* commandListDXR = (ID3D12GraphicsCommandList4*)mSandboxFramework->GetCommandList();
        ID3D12DescriptorHeap* heaps[] = { mRaytracingDescriptorHeap.Get() };
        commandListDXR->SetDescriptorHeaps(_countof(heaps), heaps);

        // On the last frame, the raytracing output was used as a copy source, to
        // copy its contents into the render target. Now we need to transition it to
        // a UAV so that the shaders can write in it.
        CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(mRaytracingOutputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandListDXR->ResourceBarrier(1, &transition);

        // RT dispatch
        D3D12_DISPATCH_RAYS_DESC desc = {};

        // The ray generation shaders are always at the beginning of the SBT.
        uint32_t rayGenerationSectionSizeInBytes = mRaytracingShaderBindingTableHelper.GetRayGenSectionSize();
        desc.RayGenerationShaderRecord.StartAddress = mRaytracingShaderTableBuffer->GetGPUVirtualAddress();
        desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

        uint32_t missSectionSizeInBytes = mRaytracingShaderBindingTableHelper.GetMissSectionSize();
        desc.MissShaderTable.StartAddress = mRaytracingShaderTableBuffer->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
        desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
        desc.MissShaderTable.StrideInBytes = mRaytracingShaderBindingTableHelper.GetMissEntrySize();

        uint32_t hitGroupsSectionSize = mRaytracingShaderBindingTableHelper.GetHitGroupSectionSize();
        desc.HitGroupTable.StartAddress = mRaytracingShaderTableBuffer->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes + missSectionSizeInBytes;
        desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
        desc.HitGroupTable.StrideInBytes = mRaytracingShaderBindingTableHelper.GetHitGroupEntrySize();

        // Dimensions of the image to render, identical to a kernel launch dimension
        auto size = mSandboxFramework->GetOutputSize();
        desc.Width = float(size.right);
        desc.Height = float(size.bottom);
        desc.Depth = 1;

        // Bind the raytracing pipeline
        commandListDXR->SetPipelineState1(mRaytracingPSO.Get());
        // Dispatch the rays and write to the raytracing output
        commandListDXR->DispatchRays(&desc);

        // The raytracing output needs to be copied to the actual render target used
        // for display. For this, we need to transition the raytracing output from a
        // UAV to a copy source, and the render target buffer to a copy destination.
        // We can then do the actual copy, before transitioning the render target
        // buffer into a render target, that will be then used to display the image
        transition = CD3DX12_RESOURCE_BARRIER::Transition(mRaytracingOutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandListDXR->ResourceBarrier(1, &transition);

        transition = CD3DX12_RESOURCE_BARRIER::Transition(mSandboxFramework->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &transition);

        commandList->CopyResource(mSandboxFramework->GetRenderTarget(), mRaytracingOutputResource.Get());
        transition = CD3DX12_RESOURCE_BARRIER::Transition(mSandboxFramework->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &transition);
    }


    /*
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
    commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &mSandboxFramework->GetDepthStencilView());

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float clearNormal[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(rtvHandles[0], clearColor, 0, nullptr);
    commandList->ClearRenderTargetView(rtvHandles[1], clearNormal, 0, nullptr);
    //commandList->ClearDepthStencilView(mDepthStencil->GetDSV().GetCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    DXRS::DescriptorHandle cbvHandle;
    DXRS::DescriptorHandle srvHandle;

    srvHandle = gpuDescriptorHeap->GetHandleBlock(0);

    //for (int i = 0; i < 1; i++)
    //{
    //   //if (i < textures.size())
    //   //{
    //   //    gpuDescriptorHeap->AddToHandle(srvHandle, textures[i]->GetSRV());
    //   //}
    //   //else
    //    {
            gpuDescriptorHeap->AddToHandle(device, srvHandle, mNullDescriptor);
     //   }
    //}


    //plane
    {
        cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
        gpuDescriptorHeap->AddToHandle(device, cbvHandle, mGbufferCB->GetCBV());
        gpuDescriptorHeap->AddToHandle(device, cbvHandle, mPlaneModel->GetCB()->GetCBV());

        commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
        commandList->SetGraphicsRootDescriptorTable(1, srvHandle.GetGPUHandle());

        mPlaneModel->Render(commandList);
    }
    //dragon
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
    commandList->DrawInstanced(4, 1, 0, 0);*/

    //mSandboxFramework->ResourceBarriersBegin(mBarriers);
    //mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);
    // Show the new frame.
    mSandboxFramework->Present();
    mGraphicsMemory->Commit(mSandboxFramework->GetCommandQueue());
}

// *** UPDATES *** //
void DXRSExampleScene::Update(DXRSTimer const& timer)
{
    UpdateControls();
    UpdateCamera();

    mWorld = XMMatrixIdentity();

    float width = mSandboxFramework->GetOutputSize().right;
    float height = mSandboxFramework->GetOutputSize().bottom;
    GBufferCBData gbufferPassData;
    gbufferPassData.ViewProjection = mCamreaView * mCameraProjection;
    gbufferPassData.InvViewProjection = XMMatrixInverse(nullptr, gbufferPassData.ViewProjection);
    gbufferPassData.MipBias = 0.0f;
    gbufferPassData.CameraPos = XMFLOAT4(mCameraEye.x, mCameraEye.y, mCameraEye.z, 1);
    gbufferPassData.RTSize = { width, height, 1.0f / width, 1.0f / height };
    memcpy(mGbufferCB->Map(), &gbufferPassData, sizeof(gbufferPassData));

    LightingCBData lightPassData = {};
    lightPassData.InvViewProjection = XMMatrixInverse(nullptr, gbufferPassData.ViewProjection);
    lightPassData.CameraPos = XMFLOAT4(mCameraEye.x, mCameraEye.y, mCameraEye.z, 1);
    lightPassData.RTSize = { width, height, 1.0f / width, 1.0f / height };
    memcpy(mLightingCB->Map(), &lightPassData, sizeof(lightPassData));

    UpdateLights();

    //TODO map model cb
    XMMATRIX local = mWorld * Matrix::CreateScale(0.3f, 0.3f, 0.3f) * Matrix::CreateTranslation(0, 0, 0.0f);
    mDragonModel->UpdateWorldMatrix(local);

    local = mWorld * Matrix::CreateScale(8.0f, 8.0f, 8.0f) * Matrix::CreateTranslation(0, 0, 0.0f) * Matrix::CreateRotationX(-3.14f / 2.0f);
    mPlaneModel->UpdateWorldMatrix(local);

}

void DXRSExampleScene::UpdateLights()
{
    XMVECTOR skyColour = XMVectorSet(0.9f, 0.8f, 1.0f, 0.0f);

    //TODO move from update
    LightsInfoCBData lightData = {};
    DirectionalLightData dirLight = {};
    dirLight.Colour = XMFLOAT4(mDirectionalLightColor[0], mDirectionalLightColor[1], mDirectionalLightColor[2], mDirectionalLightColor[3]);
    dirLight.Direction = XMFLOAT4(mDirectionalLightDir[0], mDirectionalLightDir[1], mDirectionalLightDir[2], mDirectionalLightDir[3]);
    dirLight.Intensity = mDirectionalLightIntensity;

    lightData.DirectionalLight.Colour = dirLight.Colour;
    lightData.DirectionalLight.Direction = dirLight.Direction;
    lightData.DirectionalLight.Intensity = dirLight.Intensity;

    memcpy(mLightsInfoCB->Map(), &lightData, sizeof(lightData));
}

void DXRSExampleScene::UpdateControls()
{
    auto mouse = mMouse->GetState();
    if (mouse.leftButton)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(mouse.x - mLastMousePosition.x));
        float dy = -XMConvertToRadians(0.25f * static_cast<float>(mouse.y - mLastMousePosition.y));

        mCameraTheta += dx;
        mCameraPhi += dy;

        mCameraPhi = std::clamp(mCameraPhi, 0.1f, 3.14f - 0.1f);
    }
    mLastMousePosition.x = mouse.x;
    mLastMousePosition.y = mouse.y;
    mouse;
}

void DXRSExampleScene::UpdateCamera()
{
    float x = mCameraRadius * sinf(mCameraPhi) * cosf(mCameraTheta);
    float y = mCameraRadius * cosf(mCameraPhi);
    float z = mCameraRadius * sinf(mCameraPhi) * sinf(mCameraTheta);

    mCameraEye.x = x;
    mCameraEye.y = y;
    mCameraEye.z = z;

    Vector3 at(0.0f, 0.0f, 0.0f);

    mCamreaView = Matrix::CreateLookAt(mCameraEye, at, Vector3::UnitY);

    std::vector<XMMATRIX> matrices(4); //TODO remove from update
    matrices[0] = mCamreaView;
    matrices[1] = mCameraProjection;
    matrices[2] = XMMatrixInverse(nullptr, mCamreaView);
    matrices[3] = XMMatrixInverse(nullptr, mCameraProjection);

    // Copy the matrix contents
    uint8_t* pData;
    ThrowIfFailed(mCameraBuffer->Map(0, nullptr, (void**)&pData));
    memcpy(pData, matrices.data(), 4 * sizeof(XMMATRIX));
    mCameraBuffer->Unmap(0, nullptr);
}

void DXRSExampleScene::SetProjectionMatrix()
{
    auto size = mSandboxFramework->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 60.0f * XM_PI / 180.0f;

    if (aspectRatio < 1.0f)
        fovAngleY *= 2.0f;

    mCameraProjection = Matrix::CreatePerspectiveFieldOfView(fovAngleY, aspectRatio, 0.01f, 100.0f);
}

void DXRSExampleScene::OnWindowSizeChanged(int width, int height)
{
    if (!mSandboxFramework->WindowSizeChanged(width, height))
        return;

    SetProjectionMatrix();
}

