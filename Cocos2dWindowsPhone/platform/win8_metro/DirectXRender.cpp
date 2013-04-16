/*
* cocos2d-x   http://www.cocos2d-x.org
*
* Copyright (c) 2010-2011 - cocos2d-x community
* 
* Portions Copyright (c) Microsoft Open Technologies, Inc.
* All Rights Reserved
* 
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0 
* 
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an 
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and limitations under the License.
*/

#include "pch.h"
#include "DirectXRender.h" 
#include "DXGI.h"
#include "exception\CCException.h"
#include "CCEGLView.h"
#include "CCApplication.h"

#include "CCDrawingPrimitives.h"

//#include "Classes\HelloWorldScene.h"
#include "d3d10.h"

using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;


#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
typedef FTTextPainter TextPainter;
#else
typedef DXTextPainter TextPainter;
using namespace D2D1;
#endif


USING_NS_CC;

static CCPoint getCCPointFromScreen(Point point)
{
	CCSize viewSize = cocos2d::CCEGLView::sharedOpenGLView()->getSize();

	CCPoint ccPoint;
	ccPoint.x = ceilf(point.X);
	ccPoint.y = ceilf(point.Y);

	return ccPoint;
}

static DirectXRender^ s_pDXRender;

// Constructor.
DirectXRender::DirectXRender()
	: m_dpi(-1.0f)
	, m_windowClosed(true)
{
	s_pDXRender = this;
}

// Initialize the Direct3D resources required to run.
void DirectXRender::Initialize(_In_ ID3D11Device1* device)
{
	//m_window = window;
	//m_windowClosed = false;
	m_textPainter = ref new TextPainter();

	//window->Closed += 
	//	ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &DirectXRender::OnWindowClosed);

	//window->VisibilityChanged +=
	//	ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXRender::OnWindowVisibilityChanged);

	//window->SizeChanged += 
	//	ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &DirectXRender::OnWindowSizeChanged);

	//window->PointerPressed += 
	//	ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXRender::OnPointerPressed);

	//window->PointerReleased +=
	//	ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXRender::OnPointerReleased);

	//window->PointerMoved +=
	//	ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXRender::OnPointerMoved);

	//window->CharacterReceived += 
	//	ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &DirectXRender::OnCharacterReceived);
	 
	//CreateDeviceIndependentResources();
	//CreateDeviceResources();
	//SetDpi(dpi);
	//SetRasterState();
	//Render();
	//Present();

	m_d3dDevice = device;
	CreateDeviceResources(); 
}

// These are the resources required independent of hardware.

void DirectXRender::CreateDeviceIndependentResources()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#else
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	DX::ThrowIfFailed(
		D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory1),
		&options,
		&m_d2dFactory
		)
		);

	DX::ThrowIfFailed(
		DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		&m_dwriteFactory
		)
		);

	DX::ThrowIfFailed(
		CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_wicFactory)
		)
		);
#endif
}

// These are the resources that depend on the device.
void DirectXRender::CreateDeviceResources()
{
		

}

void DirectXRender::UpdateDevice(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView)
{
	m_d3dContext = context; 
	m_renderTargetView = renderTargetView; 

	if (m_d3dDevice.Get() != device) 
	{
		m_d3dDevice = device; 
		CreateDeviceResources(); 

		m_renderTargetSize.Width  = -1; 
		m_renderTargetSize.Height = -1;
	}

	ComPtr<ID3D11Resource> renderTargetViewResource; 
	m_renderTargetView->GetResource(&renderTargetViewResource); 

	ComPtr<ID3D11Texture2D> backBuffer; 
	DX::ThrowIfFailed( 
		renderTargetViewResource.As(&backBuffer) 
		);

	D3D11_TEXTURE2D_DESC backBufferDesc; 
	backBuffer->GetDesc(&backBufferDesc); 

	if (m_renderTargetSize.Width  != static_cast<float>(backBufferDesc.Width) || 
		 m_renderTargetSize.Height != static_cast<float>(backBufferDesc.Height)) 
	{
		m_renderTargetSize.Width  = static_cast<float>(backBufferDesc.Width); 
		m_renderTargetSize.Height = static_cast<float>(backBufferDesc.Height); 
		CreateWindowSizeDependentResources(); 
	}

	CD3D11_VIEWPORT viewport( 
		 0.0f, 
		 0.0f,
		 m_renderTargetSize.Width, 
		 m_renderTargetSize.Height
		 );

	m_d3dContext->RSSetViewports(1, &viewport); 
}


// Helps track the DPI in the helper class.
// This is called in the dpiChanged event handler in the view class.
void DirectXRender::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		// Save the DPI of this display in our class.
		m_dpi = dpi;

		// Update Direct2D's stored DPI.
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#else
		m_d2dContext->SetDpi(m_dpi, m_dpi);
#endif
		// Often a DPI change implies a window size change. In some cases Windows will issues
		// both a size changed event and a DPI changed event. In this case, the resulting bounds 
		// will not change, and the window resize code will only be executed once.
		//UpdateForWindowSizeChange();
	}
}

using namespace Windows::Graphics::Display;
// This routine is called in the event handler for the view SizeChanged event.
void DirectXRender::UpdateForWindowSizeChange(float width, float height)
{
	m_windowBounds.Width  = width; 
	m_windowBounds.Height = height; 
}

// Allocate all memory resources that change on a window SizeChanged event.
void DirectXRender::CreateWindowSizeDependentResources()
{
	// Create a descriptor for the depth/stencil buffer.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		static_cast<UINT>(m_renderTargetSize.Width),
		static_cast<UINT>(m_renderTargetSize.Height),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL
		);

	// Allocate a 2-D surface as the depth/stencil buffer.
	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);


	// Create a DepthStencil view on this surface to use on bind.
	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_depthStencilView
			)
		);
}

// Method to call cocos2d's main loop for game update and draw.
void DirectXRender::Render()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#else
	//int r = 255;
	//int g = 255;
	//int b = 255;
	////int r = rand() % 255;
	////int g = rand() % 255;
	////int b = rand() % 255;
	//const float color[] = { r/255.0f, g/255.0f, b/255.0f, 1.000f };
	//   m_d3dContext->ClearRenderTargetView(
	//       m_renderTargetView.Get(),
	//       color
	//       );
	//b2Body* DXRBox = HelloWorld::getBox();
	//b2Vec2 position = DXRBox->GetPosition();
	//float width = 20.0/160.0;
	//float height = 20.0/160.0;
	//CCPoint box[] = { ccp((position.x - width)*160.0, (position.y - height)*160.0), ccp((position.x - width)*160.0, (position.y + height)*160.0),
	//				  ccp((position.x + width)*160.0, (position.y + height)*160.0), ccp((position.x + width)*160.0, (position.y - height)*160.0)};
	//ccDrawPoly(box,4,true,true);
	//ccDrawLine(ccp(0,100),ccp(480,100));
	////CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	////CCSprite *player = CCSprite::create("Player.png", CCRectMake(0, 0, 27, 40) );
	////player->setPosition( ccp(150, 150) );
	////player->draw();
	////HelloWorld::schedule(schedule_selector(HelloWorld::gameLogic), 1.0));
	////CCLabelTTF* pLabel = CCLabelTTF::create("Hello World", "Arial", 24);
	////pLabel->setPosition(ccp(200,200));
	////b2Body* temp = HelloWorld::getBody();
	////Direct3DBase::DrawRect(100.0f, 100.0f, 1.0f, 1.0f,  DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f));
#endif 
}

// Method to deliver the final image to the display.
void DirectXRender::Present()
{
	//int r = rand() % 255;
	//int g = rand() % 255;
	//int b = rand() % 255;


	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	// We 
	//HRESULT hr = m_swapChain->Present(1, 0);

	// If the device was removed either by a disconnect or a driver upgrade, we 
	// must completely reinitialize the renderer.
	//if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	//{
	//	Initialize(m_window.Get(), m_dpi);
	//}
	//else
	//{
	//	DX::ThrowIfFailed(hr);
	//}
}

void DirectXRender::SetBackBufferRenderTarget()
{
	m_d3dContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView.Get());
}

void DirectXRender::CloseWindow()
{
	if (nullptr != m_window.Get())
	{
		m_window->Close();
		m_window = nullptr;
	}
	m_windowClosed = true;
}

bool DirectXRender::GetWindowsClosedState()
{
	return m_windowClosed;
}

DirectXRender^ DirectXRender::SharedDXRender()
{
	return s_pDXRender;
}

bool DirectXRender::SetRasterState()
{
	ID3D11RasterizerState* rasterState;
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc,sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.AntialiasedLineEnable = TRUE;
	rasterDesc.FrontCounterClockwise = TRUE;
	rasterDesc.MultisampleEnable = FALSE;
	rasterDesc.ScissorEnable = FALSE;

	// Create the rasterizer state from the description we just filled out.
	if(FAILED(m_d3dDevice->CreateRasterizerState(&rasterDesc, &rasterState)))
	{
		return FALSE;
	}
	m_d3dContext->RSSetState(rasterState);
	if(rasterState)
	{
		rasterState->Release();
		rasterState = 0;
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Event callback functions
////////////////////////////////////////////////////////////////////////////////

void DirectXRender::OnWindowClosed(
	_In_ CoreWindow^ sender,
	_In_ CoreWindowEventArgs^ args
	)
{
	m_window = nullptr;
	m_windowClosed = true;
}

void DirectXRender::OnWindowVisibilityChanged(
	_In_ Windows::UI::Core::CoreWindow^ sender,
	_In_ Windows::UI::Core::VisibilityChangedEventArgs^ args
	)
{
	if (args->Visible)
	{
		CCApplication::sharedApplication()->applicationWillEnterForeground();
	} 
	else
	{
		CCApplication::sharedApplication()->applicationDidEnterBackground();
	}
}

void DirectXRender::OnWindowSizeChanged(
	_In_ CoreWindow^ sender,
	_In_ WindowSizeChangedEventArgs^ args
	)
{
	//UpdateForWindowSizeChange();
	cocos2d::CCEGLView::sharedOpenGLView()->OnWindowSizeChanged();
}

void DirectXRender::OnPointerPressed(
	_In_ Windows::UI::Core::CoreWindow^ sender,
	_In_ Windows::UI::Core::PointerEventArgs^ args
	)
{
	CCPoint point = getCCPointFromScreen(args->CurrentPoint->Position);
	cocos2d::CCEGLView::sharedOpenGLView()->OnPointerPressed(args->CurrentPoint->PointerId, point);
}

void DirectXRender::OnPointerReleased(
	_In_ Windows::UI::Core::CoreWindow^ sender,
	_In_ Windows::UI::Core::PointerEventArgs^ args
	)
{
	CCPoint point = getCCPointFromScreen(args->CurrentPoint->Position);
	cocos2d::CCEGLView::sharedOpenGLView()->OnPointerReleased(args->CurrentPoint->PointerId, point);
}

void DirectXRender::OnPointerMoved(
	_In_ Windows::UI::Core::CoreWindow^ sender,
	_In_ Windows::UI::Core::PointerEventArgs^ args
	)
{
	CCPoint point = getCCPointFromScreen(args->CurrentPoint->Position);
	cocos2d::CCEGLView::sharedOpenGLView()->OnPointerMoved(args->CurrentPoint->PointerId, point);
}

void DirectXRender::OnCharacterReceived(
	_In_ Windows::UI::Core::CoreWindow^ sender,
	_In_ Windows::UI::Core::CharacterReceivedEventArgs^ args
	)
{
	cocos2d::CCEGLView::sharedOpenGLView()->OnCharacterReceived(args->KeyCode);
}
