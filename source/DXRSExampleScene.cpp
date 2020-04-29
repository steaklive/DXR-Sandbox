#include "DXRSExampleScene.h"

DXRSExampleScene::DXRSExampleScene()
{
	mSandboxFramework = std::make_unique<DXRSGraphics>();
}

DXRSExampleScene::~DXRSExampleScene()
{
	if (mSandboxFramework)
		mSandboxFramework->WaitForGpu();
}

void DXRSExampleScene::Init(HWND window, int width, int height)
{
    mGamePad = std::make_unique<DirectX::GamePad>();
    mKeyboard = std::make_unique<DirectX::Keyboard>();
    mMouse = std::make_unique<DirectX::Mouse>();
    mMouse->SetWindow(window);

    mSandboxFramework->SetWindow(window, width, height);

    mSandboxFramework->CreateResources();
    CreateDeviceResources();

    mSandboxFramework->CreateWindowResources();
    CreateWindowResources();
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
    m_timer.Run([&]()
    {
        Update(m_timer);
    });

    Render();
}

void DXRSExampleScene::Update(DXRSTimer const& timer)
{
    Vector3 eye(0.0f, 0.0f, 1.5f);
    Vector3 at(0.0f, 0.0f, 0.0f);

    m_view = Matrix::CreateLookAt(eye, at, Vector3::UnitY);

    m_world = Matrix::CreateRotationY(float(timer.GetTotalSeconds() * DirectX::XM_PIDIV4));

    m_lineEffect->SetView(m_view);
    m_lineEffect->SetWorld(Matrix::Identity);

    m_shapeEffect->SetView(m_view);


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

void DXRSExampleScene::Render()
{
    if (m_timer.GetFrameCount() == 0)
        return;

    // Prepare the command list to render a new frame.
    mSandboxFramework->Prepare();
    Clear();

    auto commandList = mSandboxFramework->GetCommandList();

    // Set the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);


    XMMATRIX local = m_world *Matrix::CreateTranslation(0, 0.0f, -5.0f);
    m_shapeEffect->SetWorld(local);
    m_shapeEffect->Apply(commandList);
    m_shape->Draw(commandList);

   //// Draw model
   //const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f };
   //const XMVECTORF32 translate = { 3.f, -2.f, -4.f };
   //XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XM_PI / 2.f, 0.f, -XM_PI / 2.f);
   //XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);
   //Model::UpdateEffectMatrices(m_modelEffects, local, m_view, m_projection);
   //heaps[0] = m_modelResources->Heap();
   //commandList->SetDescriptorHeaps(_countof(heaps), heaps);
   //m_model->Draw(commandList, m_modelEffects.begin());

    // Show the new frame.
    mSandboxFramework->Present();
    m_graphicsMemory->Commit(mSandboxFramework->GetCommandQueue());
}

void DXRSExampleScene::CreateDeviceResources()
{
    auto device = mSandboxFramework->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_states = std::make_unique<CommonStates>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

    m_shape = GeometricPrimitive::CreateTeapot(4.f, 8);

    // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    wchar_t strFilePath[MAX_PATH] = { L"Tiny\\tiny.sdkmesh" };
    //strFilePath = L"Tiny\\tiny.sdkmesh";

    wchar_t txtPath[MAX_PATH] = {};
    {
        wchar_t drive[_MAX_DRIVE];
        wchar_t path[_MAX_PATH];

        if (_wsplitpath_s(strFilePath, drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0))
            throw std::exception("_wsplitpath_s");

        if (_wmakepath_s(txtPath, _MAX_PATH, drive, path, nullptr, nullptr))
            throw std::exception("_wmakepath_s");
    }

    //m_model = Model::CreateFromSDKMESH(device, strFilePath);

    {
        ResourceUploadBatch resourceUpload(device);

        resourceUpload.Begin();

        //m_model->LoadStaticBuffers(device, resourceUpload);

        ThrowIfFailed(
            CreateDDSTextureFromFile(device, resourceUpload, L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\textures\\seafloor.dds", m_texture1.ReleaseAndGetAddressOf())
        );

        CreateShaderResourceView(device, m_texture1.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::SeaFloor));

        RenderTargetState rtState(mSandboxFramework->GetBackBufferFormat(), mSandboxFramework->GetDepthBufferFormat());

        {
            EffectPipelineStateDescription pd(
                &VertexPositionColor::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthNone,
                CommonStates::CullNone,
                rtState,
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

            m_lineEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
        }

        {
            EffectPipelineStateDescription pd(
                &GeometricPrimitive::VertexType::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullNone,
                rtState);

            m_shapeEffect = std::make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
            m_shapeEffect->EnableDefaultLighting();
            m_shapeEffect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::SeaFloor), m_states->AnisotropicWrap());
        }

        //m_modelResources = m_model->LoadTextures(device, resourceUpload, txtPath);

        {
            EffectPipelineStateDescription psd(
                nullptr,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullClockwise,    // Using RH coordinates, and SDKMESH is in LH coordiantes
                rtState);

            EffectPipelineStateDescription alphapsd(
                nullptr,
                CommonStates::NonPremultiplied, // Using straight alpha
                CommonStates::DepthRead,
                CommonStates::CullClockwise,    // Using RH coordinates, and SDKMESH is in LH coordiantes
                rtState);

            //m_modelEffects = m_model->CreateEffects(psd, alphapsd, m_modelResources->Heap(), m_states->Heap());
        }

        // Upload the resources to the GPU.
        auto uploadResourcesFinished = resourceUpload.End(mSandboxFramework->GetCommandQueue());

        // Wait for the upload thread to terminate
        uploadResourcesFinished.wait();
    }
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
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );

    m_lineEffect->SetProjection(m_projection);
    m_shapeEffect->SetProjection(m_projection);
}

void DXRSExampleScene::OnWindowSizeChanged(int width, int height)
{
    if (!mSandboxFramework->WindowSizeChanged(width, height))
        return;

    CreateWindowResources();
}

