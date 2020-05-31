#pragma once

#include <Unknwn.h>
#include <inspectable.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

// STL
#include <atomic>
#include <memory>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// Helpers
#include "composition.interop.h"
#include "d3dHelpers.h"
#include "direct3d11.interop.h"
#include "capture.interop.h"

class SimpleCapture
{
public:
    SimpleCapture(
        winrt::com_ptr<ID3D11Device> aD3DDevice,
        HWND aWindowHandle
        //winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        /*winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item*/);
    ~SimpleCapture() { Close(); }

    void StartCapture();
    winrt::Windows::UI::Composition::ICompositionSurface CreateSurface(
        winrt::Windows::UI::Composition::Compositor const& compositor);

    void ResizeIfRequired();

    void Close();

    struct DrawData
    {
        ID3D11ShaderResourceView* mTextureView;
        unsigned int mWidth;
        unsigned int mHeight;
    };

    DrawData GetDrawData()
    {
        return { mTextureView.get(),(unsigned int)mLastSize.Width, (unsigned int)mLastSize.Height };
    }

    HWND GetWindowHandle()
    {
        return mWindowHandle;
    }

private:
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    void CheckClosed()
    {
        if (m_closed.load() == true)
        {
            throw winrt::hresult_error(RO_E_CLOSED);
        }
    }

    void ResetTexture(winrt::Windows::Graphics::SizeInt32 aSize);
    winrt::Windows::Graphics::SizeInt32 GetClientSize();


private:
    HWND mWindowHandle = NULL;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 mLastSize;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
    winrt::com_ptr<ID3D11Device> mD3DDevice;
    winrt::com_ptr<ID3D11Texture2D> mTexture;
    winrt::com_ptr<ID3D11ShaderResourceView> mTextureView;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived;
    std::atomic<bool> m_closed = false;
    bool mNewSize = false;
};