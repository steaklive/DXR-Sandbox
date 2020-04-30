#pragma once

#include "DXRSGraphics.h"
#include "DXRSTimer.h"
#include "DXRSModel.h"

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
    void CreateDeviceResources()            /*override*/;
    void CreateWindowResources()            /*override*/;
    
    

    DXRSGraphics*   	        mSandboxFramework;

	U_PTR<GamePad>	            mGamePad;
	U_PTR<Keyboard>             mKeyboard;
	U_PTR<Mouse>                mMouse;

    U_PTR<DXRSModel>            mDragonModel;

	DirectX::GamePad::ButtonStateTracker        mGamePadButtons;
	DirectX::Keyboard::KeyboardStateTracker     mKeyboardButtons;

    DXRSTimer m_timer;

    // DirectXTK objects.
    U_PTR<GraphicsMemory>                                mGraphicsMemory;
    U_PTR<DescriptorHeap>                                mResourceDescriptors;
    U_PTR<CommonStates>                                  mStates;
    U_PTR<BasicEffect>                                   mBasicEffect;
    U_PTR<EffectTextureFactory>                          mModelResources;

    ComPtr<ID3D12Resource>                               mTexture1;

    Matrix                                               mWorld;
    Matrix                                               mView;
    Matrix                                               mProjection;

    // Descriptors
    enum Descriptors
    {
        WindowsLogo,
        SeaFloor,
        SegoeFont,
        Count = 256
    };
};
