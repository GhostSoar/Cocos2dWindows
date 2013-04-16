#pragma once

#include "DirectXHelper.h"

// 为 3D 呈现初始化 DirectX API 的帮助器类。
ref class Direct3DBase abstract
{
internal:
	Direct3DBase();

	virtual void Initialize(_In_ ID3D11Device1* device);
	virtual void CreateDeviceResources();
	virtual void UpdateDevice(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView);
	virtual void CreateWindowSizeDependentResources();
	virtual void UpdateForWindowSizeChange(float width, float height);
	virtual void Render() = 0;

protected private:
	// Direct3D 对象。
	Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;

	// 缓存的呈现器属性。
	Windows::Foundation::Size m_renderTargetSize;
	Windows::Foundation::Rect m_windowBounds;
};