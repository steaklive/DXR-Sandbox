#include "DXRSExampleScene.h"

DXRSExampleScene::DXRSExampleScene()
{
    mSandboxFramework = new DXRSGraphics();
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

    //create dragon model
    mDragonModel = U_PTR<DXRSModel>(new DXRSModel(*mSandboxFramework, "C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\models\\dragon.fbx", true));

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

    mView = Matrix::CreateLookAt(eye, at, Vector3::UnitY);

    mWorld = Matrix::CreateRotationY(float(timer.GetTotalSeconds() * DirectX::XM_PIDIV4));

    mBasicEffect->SetView(mView);


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
    ID3D12DescriptorHeap* heaps[] = { mResourceDescriptors->Heap(), mStates->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);


    XMMATRIX local = mWorld * Matrix::CreateScale(0.2f, 0.2f, 0.2f) *  Matrix::CreateTranslation(0, -1.5f, -5.0f);
    mBasicEffect->SetWorld(local);
    mBasicEffect->Apply(commandList);

    mDragonModel->Render(commandList);

    // Show the new frame.
    mSandboxFramework->Present();
    mGraphicsMemory->Commit(mSandboxFramework->GetCommandQueue());
}

void DXRSExampleScene::CreateDeviceResources()
{
    auto device = mSandboxFramework->GetD3DDevice();

    mGraphicsMemory = std::make_unique<GraphicsMemory>(device);
    mStates = std::make_unique<CommonStates>(device);

    mResourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count);

    {
        ResourceUploadBatch resourceUpload(device);

        resourceUpload.Begin();

        //m_model->LoadStaticBuffers(device, resourceUpload);

        ThrowIfFailed(
            CreateDDSTextureFromFile(device, resourceUpload, L"C:\\Users\\Zhenya\\Documents\\GraphicsProgramming\\DXR-Sandbox\\content\\textures\\seafloor.dds", mTexture1.ReleaseAndGetAddressOf())
        );

        CreateShaderResourceView(device, mTexture1.Get(), mResourceDescriptors->GetCpuHandle(Descriptors::SeaFloor));

        RenderTargetState rtState(mSandboxFramework->GetBackBufferFormat(), mSandboxFramework->GetDepthBufferFormat());

        {
            EffectPipelineStateDescription pd(
                &GeometricPrimitive::VertexType::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullNone,
                rtState);

            mBasicEffect = std::make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
            mBasicEffect->EnableDefaultLighting();
            mBasicEffect->SetTexture(mResourceDescriptors->GetGpuHandle(Descriptors::SeaFloor), mStates->AnisotropicWrap());
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
    mProjection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );

    mBasicEffect->SetProjection(mProjection);
}

void DXRSExampleScene::OnWindowSizeChanged(int width, int height)
{
    if (!mSandboxFramework->WindowSizeChanged(width, height))
        return;

    CreateWindowResources();
}

