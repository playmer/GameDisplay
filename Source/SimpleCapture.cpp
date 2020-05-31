//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

//#include <winrt/Windows.Graphics.Capture.h>
//#include <windows.graphics.capture.interop.h>
//#include <windows.graphics.capture.h>

#include "SimpleCapture.h"

using namespace winrt;
using namespace Windows;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;


struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
    IDirect3DDxgiInterfaceAccess : ::IUnknown
{
    virtual HRESULT __stdcall GetInterface(GUID const& id, void** object) = 0;
};

template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
    auto access = object.as<IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

inline auto CreateCaptureItemForWindow(HWND hwnd)
{
	auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
	auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
	interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item)));
	return item;
}

extern "C"
{
    HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(::IDXGIDevice* dxgiDevice,
        ::IInspectable** graphicsDevice);

    HRESULT __stdcall CreateDirect3D11SurfaceFromDXGISurface(::IDXGISurface* dgxiSurface,
        ::IInspectable** graphicsSurface);
}


inline auto CreateDirect3DDevice(IDXGIDevice* dxgi_device)
{
    winrt::com_ptr<::IInspectable> d3d_device;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, d3d_device.put()));
    return d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

SizeInt32 SimpleCapture::GetClientSize()
{
    //RECT rect;
    //SizeInt32 size;
    //
    //GetWindowRect(aWindowHandle, &rect);
    ////GetClientRect(aWindowHandle, &rect);
    //
    //size.Width = rect.right - rect.left;
    //size.Height = rect.bottom - rect.top;
    //return size;

    
    return m_item.Size();
}

SimpleCapture::SimpleCapture(
    winrt::com_ptr<ID3D11Device> aD3DDevice,
    HWND aWindowHandle
    //IDirect3DDevice const& device,
    /*GraphicsCaptureItem const& item*/)
{
    mWindowHandle = aWindowHandle;
    mD3DDevice = aD3DDevice;
    m_item = CreateCaptureItemForWindow(aWindowHandle);
    
    auto dxgiDevice = mD3DDevice.as<IDXGIDevice>();

    m_device = CreateDirect3DDevice(dxgiDevice.get());

    // Set up 
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    d3dDevice->GetImmediateContext(m_d3dContext.put());

    SizeInt32 size = m_item.Size();

    ResetTexture(size);

    // Create framepool, define pixel format (DXGI_FORMAT_B8G8R8A8_UNORM), and frame size. 
    m_framePool = Direct3D11CaptureFramePool::Create(
        m_device,
        DirectXPixelFormat::B8G8R8A8UIntNormalized,
        2,
        size);
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_session.IsCursorCaptureEnabled(false);
    mLastSize = size;
    m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &SimpleCapture::OnFrameArrived });
}

// Start sending capture frames
void SimpleCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture();
}


void SimpleCapture::ResetTexture(SizeInt32 aSize)
{
    std::vector<unsigned char> blankData(aSize.Width * aSize.Height * 4, 0);
    mTexture.detach();
    mTextureView.detach();

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = static_cast<UINT>(aSize.Width);
    desc.Height = static_cast<UINT>(aSize.Height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    // Create texture
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = blankData.data();
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    mD3DDevice->CreateTexture2D(&desc, &subResource, mTexture.put());

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    mD3DDevice->CreateShaderResourceView(mTexture.get(), &srvDesc, mTextureView.put());
}

// Process captured frames
void SimpleCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_frameArrived.revoke();
        m_framePool.Close();
        m_session.Close();

        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void SimpleCapture::OnFrameArrived(
    Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&)
{
    mNewSize = false;

    auto frame = sender.TryGetNextFrame();
    auto frameContentSize = frame.ContentSize();

    if (frameContentSize.Width != mLastSize.Width ||
        frameContentSize.Height != mLastSize.Height)
    {
        // The thing we have been capturing has changed size.
        // We need to resize our swap chain first, then blit the pixels.
        // After we do that, retire the frame and then recreate our frame pool.
        mNewSize = true;
        mLastSize = frameContentSize;
    }

    auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
            
    m_d3dContext->CopyResource(mTexture.get(), frameSurface.get());
}

void SimpleCapture::ResizeIfRequired()
{
    if (mNewSize)
    {
        m_framePool.Recreate(
            m_device,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            mLastSize);
    
        ResetTexture(mLastSize);
    }
}


