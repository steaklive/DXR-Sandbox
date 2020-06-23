#pragma once

#include "DXRSGraphics.h"
#include "DXRSTimer.h"
#include "DXRSModel.h"
#include "DXRSRenderTarget.h"
#include "DXRSDepthBuffer.h"
#include "DXRSBuffer.h"

#include "RootSignature.h"
#include "PipelineStateObject.h"

#include "RaytracingPipelineGenerator.h"
#include "ShaderBindingTableGenerator.h"

class DXRSExampleScene 
{
public:
	DXRSExampleScene();
	~DXRSExampleScene();

	void Init(HWND window, int width, int height);
    void Clear();
    void Run();
    void OnWindowSizeChanged(int width, int height);

private:
    void Update(DXRSTimer const& timer);
    void Render();
    void SetProjectionMatrix();
    void UpdateLights();
    void UpdateControls();
    void UpdateCamera();

    //raytracing methods
    void CreateRaytracingPSO();
    void CreateRaytracingAccelerationStructures();
    void CreateRaytracingShaders();
    void CreateRaytracingShaderTable();
    void CreateRaytracingResourceHeap();

    DXRSGraphics*   	                                 mSandboxFramework;
                                                         
	U_PTR<GamePad>	                                     mGamePad;
	U_PTR<Keyboard>                                      mKeyboard;
	U_PTR<Mouse>                                         mMouse;
	DirectX::GamePad::ButtonStateTracker                 mGamePadButtons;
	DirectX::Keyboard::KeyboardStateTracker              mKeyboardButtons;

    DXRSTimer                                            mTimer;

    std::vector<CD3DX12_RESOURCE_BARRIER>                mBarriers;

    U_PTR<GraphicsMemory>                                mGraphicsMemory;
    U_PTR<CommonStates>                                  mStates;

    U_PTR<DXRSModel>                                     mDragonModel;
    U_PTR<DXRSModel>                                     mPlaneModel;

    // Gbuffer
    RootSignature                                        mGbufferRS;
    std::vector< DXRSRenderTarget*>                      mGbufferRTs;
    GraphicsPSO                                          mGbufferPSO;
    DXRSBuffer*                                          mGbufferCB;
    DXRSDepthBuffer*                                     mDepthStencil;
    DXRS::DescriptorHandle                               mNullDescriptor;

    __declspec(align(16)) struct GBufferCBData
    {
        XMMATRIX ViewProjection;
        XMMATRIX InvViewProjection;
        XMFLOAT4 CameraPos;
        XMFLOAT4 RTSize;
        float MipBias;
    };

    // Composite
    RootSignature                                        mCompositeRS;
    GraphicsPSO                                          mCompositePSO;

    // Lighting
    RootSignature                                        mLightingRS;
    std::vector< DXRSRenderTarget*>                      mLightingRTs;
    GraphicsPSO                                          mLightingPSO;
    DXRSBuffer*                                          mLightingCB;
    DXRSBuffer*                                          mLightsInfoCB;

    __declspec(align(16)) struct LightingCBData
    {
        XMMATRIX InvViewProjection;
        XMFLOAT4 CameraPos;
        XMFLOAT4 RTSize;
    };

    __declspec(align(16)) struct DirectionalLightData
    {
        XMFLOAT4	Direction;
        XMFLOAT4	Colour;
        float		Intensity;
    };
    __declspec(align(16)) struct PointLightData
    {
        XMFLOAT4	Position;
        XMFLOAT4	Colour;
        float		Intensity;
        float		Attenuation;
        float		Radius;
    };
    __declspec(align(16)) struct LightsInfoCBData
    {
        DirectionalLightData DirectionalLight;
        //PointLightData PointLights[NUM_POINT_LIGHTS];
    };

    // Directional light
    float mDirectionalLightColor[4]{ 0.9, 0.9, 0.9, 1.0 };
    float mDirectionalLightDir[4]{ 0.0, 0.707f, 0.707f, 1.0 };
    float mDirectionalLightIntensity = 1.0f;

    // Raytracing
    IDxcBlob* mRaygenBlob;
    IDxcBlob* mClosestHitBlob;
    IDxcBlob* mMissBlob;

    RootSignature mRaygenRS;
    RootSignature mClosestHitRS;
    RootSignature mMissRS;

    ComPtr<ID3D12DescriptorHeap>        mRaytracingDescriptorHeap;
    ComPtr<ID3D12StateObject>           mRaytracingPSO;
    ComPtr<ID3D12StateObjectProperties> mRaytracingPSOProperties;
    ComPtr<ID3D12Resource>              mRaytracingOutputResource;
    ComPtr<ID3D12Resource>              mRaytracingShaderTableBuffer;
    ShaderBindingTableGenerator         mRaytracingShaderBindingTableHelper;

    DXRSBuffer* mTLASBuffer; // top level acceleration structure of the scene

    // Camera
    ComPtr<ID3D12Resource> mCameraBuffer;
    XMFLOAT2 mLastMousePosition;
    float mCameraTheta = -1.5f * XM_PI;
    float mCameraPhi = XM_PI / 3;
    float mCameraRadius = 25.0f;
    Vector3 mCameraEye{ 0.0f, 0.0f, 0.0f };
    Matrix mCamreaView;
    Matrix mCameraProjection;

    Matrix mWorld;
};
