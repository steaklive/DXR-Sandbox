#pragma once

#include "DXRSGraphics.h"
#include "DXRSTimer.h"
#include "DXRSModel.h"
#include "DXRSRenderTarget.h"
#include "DXRSDepthBuffer.h"
#include "DXRSBuffer.h"

#include "RootSignature.h"
#include "PipelineStateObject.h"

#define NUM_POINT_LIGHTS 15

class DXRSExampleScene /*: public SandboxScene*/
{
public:
	DXRSExampleScene();
	~DXRSExampleScene();

	void Init(HWND window, int width, int height) /*override*/;
    void Clear()                                  /*override*/;
    void Run()                                    /*override*/;

    void OnWindowSizeChanged(int width, int height);
private:
    void Update(DXRSTimer const& timer)  /*override*/;
    void Render()                           /*override*/;
    void CreateDragonMeshResources()            /*override*/;
    void CreateWindowResources()            /*override*/;
    void UpdateLights();
    
    DXRSGraphics*   	                                 mSandboxFramework;
                                                         
	U_PTR<GamePad>	                                     mGamePad;
	U_PTR<Keyboard>                                      mKeyboard;
	U_PTR<Mouse>                                         mMouse;
	DirectX::GamePad::ButtonStateTracker                 mGamePadButtons;
	DirectX::Keyboard::KeyboardStateTracker              mKeyboardButtons;

    DXRSTimer                                            mTimer;

    std::vector<CD3DX12_RESOURCE_BARRIER>                mBarriers;

    // DirectXTK objects.
    U_PTR<GraphicsMemory>                                mGraphicsMemory;
    //U_PTR<DescriptorHeap>                                mResourceDescriptors;
    U_PTR<CommonStates>                                  mStates;

    // Dragon
    enum DragonDescriptors
    {
        SeaFloor, //texture
        Count = 256
    };
    U_PTR<BasicEffect>                                   mDragonBasicEffectXTK;
    ComPtr<ID3D12Resource>                               mDragonTextureAlbedo;
    U_PTR<DXRSModel>                                     mDragonModel;

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
       // UINT	PointAmount;
       // UINT	pad1;
      //  UINT	pad2;
      //  UINT	pad3;
       // XMFLOAT4 SkyLight;
        DirectionalLightData DirectionalLight;
        //PointLightData PointLights[NUM_POINT_LIGHTS];
    };

    float mDirectionalLightColor[4]{ 0.9, 0.9, 0.9, 1.0 };
    float mDirectionalLightDir[4]{ 0.0, 0.0, 1.0, 1.0 };
    float mDirectionalLightIntensity = 1.0f;

    //extra constants
    Matrix                                               mWorld;
    Matrix                                               mView;
    Matrix                                               mProjection;
};
