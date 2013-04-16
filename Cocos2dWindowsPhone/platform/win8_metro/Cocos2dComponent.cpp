#include "pch.h"
#include "Cocos2dComponent.h"
#include "Direct3DContentProvider.h"

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Microsoft::WRL;
using namespace Windows::Phone::Graphics::Interop;
using namespace Windows::Phone::Input::Interop;

namespace cocos2d
{


Cocos2dComponent::Cocos2dComponent() :
	m_timer(ref new BasicTimer())
{
}

IDrawingSurfaceBackgroundContentProvider^ Cocos2dComponent::CreateContentProvider()
{
	ComPtr<Direct3DContentProvider> provider = Make<Direct3DContentProvider>(this);
	return reinterpret_cast<IDrawingSurfaceBackgroundContentProvider^>(provider.Detach());
}

// IDrawingSurfaceManipulationHandler
void Cocos2dComponent::SetManipulationHost(DrawingSurfaceManipulationHost^ manipulationHost)
{
	manipulationHost->PointerPressed +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Cocos2dComponent::OnPointerPressed);

	manipulationHost->PointerMoved +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Cocos2dComponent::OnPointerMoved);

	manipulationHost->PointerReleased +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Cocos2dComponent::OnPointerReleased);
}

// 事件处理程序
void Cocos2dComponent::OnPointerPressed(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

void Cocos2dComponent::OnPointerMoved(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

void Cocos2dComponent::OnPointerReleased(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

// 与 Direct3DContentProvider 交互
HRESULT Cocos2dComponent::Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device)
{
	m_renderer = ref new cocos2d::DirectXRender();
	m_renderer->UpdateForWindowSizeChange(WindowBounds.Width, WindowBounds.Height);
	m_renderer->Initialize(device);



	// 在呈现器完成初始化后重新启动计时器。
	m_timer->Reset();

	return S_OK;
}

void Cocos2dComponent::Disconnect()
{
	m_renderer = nullptr;
}

HRESULT Cocos2dComponent::PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize)
{
	m_timer->Update();
	//m_renderer->Update(m_timer->Total, m_timer->Delta);


	desiredRenderTargetSize->width = RenderResolution.Width;
	desiredRenderTargetSize->height = RenderResolution.Height;

	return S_OK;
}

static bool bLoop = false;

HRESULT Cocos2dComponent::Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView)
{
	m_renderer->UpdateDevice(device, context, renderTargetView);
	m_renderer->Render();

	if(!bLoop)
	{
		bLoop = true;
		CCApplication::sharedApplication()->initInstance();
		CCApplication::sharedApplication()->applicationDidFinishLaunching();
	}

	CCDirector::sharedDirector()->mainLoop();


	RequestAdditionalFrame();

	return S_OK;
}

}