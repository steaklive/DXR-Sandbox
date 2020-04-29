#pragma once

#include "DXRSGraphics.h"
#include "DXRSTimer.h"

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
    
    

	U_PTR<DXRSGraphics>	        mSandboxFramework;
	U_PTR<GamePad>	            mGamePad;
	U_PTR<Keyboard>             mKeyboard;
	U_PTR<Mouse>                mMouse;

	DirectX::GamePad::ButtonStateTracker        mGamePadButtons;
	DirectX::Keyboard::KeyboardStateTracker     mKeyboardButtons;

    DXRSTimer m_timer;

    // DirectXTK objects.
    U_PTR<GraphicsMemory>                                m_graphicsMemory;
    U_PTR<DescriptorHeap>                                m_resourceDescriptors;
    U_PTR<CommonStates>                                  m_states;
    U_PTR<BasicEffect>                                   m_lineEffect;
    U_PTR<PrimitiveBatch<VertexPositionColor>>           m_batch;
    U_PTR<BasicEffect>                                   m_shapeEffect;
    //U_PTR<Model>                                         m_model;
    std::vector<std::shared_ptr<IEffect>>                m_modelEffects;
    U_PTR<EffectTextureFactory>                          m_modelResources;
    U_PTR<GeometricPrimitive>                            m_shape;


    ComPtr<ID3D12Resource>                                  m_texture1;

    Matrix                                             m_world;
    Matrix                                             m_view;
    Matrix                                             m_projection;

    // Descriptors
    enum Descriptors
    {
        WindowsLogo,
        SeaFloor,
        SegoeFont,
        Count = 256
    };
};
