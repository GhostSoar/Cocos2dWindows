#include "pch.h"
#include "HelloXAMLComponent.h"
#include "Direct3DContentProvider.h"
#include "cocos2d.h"
#include "CCEGLView.h"
#include "classes\AppDelegate.h"

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Microsoft::WRL;
using namespace Windows::Phone::Graphics::Interop;
using namespace Windows::Phone::Input::Interop;

namespace PhoneDirect3DXamlAppComponent
{

AppDelegate app;	
Direct3DBackground::Direct3DBackground() :
	m_timer(ref new BasicTimer())
{
}

IDrawingSurfaceContentProvider^ Direct3DBackground::CreateContentProvider()
{
	ComPtr<Direct3DContentProvider> provider = Make<Direct3DContentProvider>(this);
	return reinterpret_cast<IDrawingSurfaceContentProvider^>(provider.Detach());
}

// IDrawingSurfaceManipulationHandler
void Direct3DBackground::SetManipulationHost(DrawingSurfaceManipulationHost^ manipulationHost)
{
	manipulationHost->PointerPressed +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerPressed);

	manipulationHost->PointerMoved +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerMoved);

	manipulationHost->PointerReleased +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerReleased);
}

// 事件处理程序
void Direct3DBackground::OnPointerPressed(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

void Direct3DBackground::OnPointerMoved(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

void Direct3DBackground::OnPointerReleased(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// 在此处插入代码。
}

// 与 Direct3DContentProvider 交互
HRESULT Direct3DBackground::Connect(_In_ IDrawingSurfaceRuntimeHostNative* host)
{
	m_renderer = ref new cocos2d::DirectXRender();
	m_renderer->Initialize();
	m_renderer->UpdateForWindowSizeChange(WindowBounds.Width, WindowBounds.Height);
	m_renderer->UpdateForRenderResolutionChange(m_renderResolution.Width, m_renderResolution.Height);

		cocos2d::CCEGLView* mainView = cocos2d::CCEGLView::sharedOpenGLView();
		mainView->setDesignResolution(480, 320);

		cocos2d::CCApplication::sharedApplication()->initInstance();
		cocos2d::CCApplication::sharedApplication()->applicationDidFinishLaunching();
	// 在呈现器完成初始化后重新启动计时器。
	m_timer->Reset();

	return S_OK;
}

void Direct3DBackground::RenderResolution::set(Windows::Foundation::Size renderResolution)
{
	if (renderResolution.Width  != m_renderResolution.Width ||
		renderResolution.Height != m_renderResolution.Height)
	{
		m_renderResolution = renderResolution;

		if (m_renderer)
		{
			m_renderer->UpdateForRenderResolutionChange(m_renderResolution.Width, m_renderResolution.Height);
//			RecreateSynchronizedTexture();
		}
	}
}

void Direct3DBackground::Disconnect()
{
	m_renderer = nullptr;
}

HRESULT Direct3DBackground::PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Out_ BOOL* contentDirty)
{
	*contentDirty = true;

	return S_OK;
}

HRESULT Direct3DBackground::GetTexture(_In_ const DrawingSurfaceSizeF* size, _Out_ IDrawingSurfaceSynchronizedTextureNative** synchronizedTexture, _Out_ DrawingSurfaceRectF* textureSubRectangle)
{
	m_timer->Update();
//	m_renderer->Update(m_timer->Total, m_timer->Delta);
	cocos2d::CCDirector::sharedDirector()->mainLoop();
	m_renderer->Render();

	RequestAdditionalFrame();

	return S_OK;
}
}