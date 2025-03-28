﻿#include "SwapChain.h"

#include "Device.h"
#include "RenderTargetSurface.h"
#include "TypeConverter.h"
#include "RenderTargetTexture.h"

RenderTargetSurface* SwapChain::getRenderTargetSurface() const
{
    return renderTargetSurface.Get();
}

bool SwapChain::initialize(Device* device, D3DPRESENT_PARAMETERS* presentationParameters, DXGI_SCALING scaling)
{
    ComPtr<IDXGIFactory3> dxgiFactory;
    CreateDXGIFactory2(
#if _DEBUG
        DXGI_CREATE_FACTORY_DEBUG,
#else
        0,
#endif
        IID_PPV_ARGS(dxgiFactory.GetAddressOf()));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = presentationParameters->BackBufferWidth;
    swapChainDesc.Height = presentationParameters->BackBufferHeight;
    swapChainDesc.Format = TypeConverter::convert(presentationParameters->BackBufferFormat);
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 3;
    swapChainDesc.Scaling = scaling;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> swapChain1;
    dxgiFactory->CreateSwapChainForHwnd(
        device->get(),
        presentationParameters->hDeviceWindow,
        &swapChainDesc,
        nullptr,
        nullptr,
        swapChain1.GetAddressOf());

    swapChain1.As(&swapChain);

    swapChain->SetMaximumFrameLatency(2);
    waitHandle = swapChain->GetFrameLatencyWaitableObject();

    ComPtr<ID3D11Texture2D> backBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));

    ComPtr<RenderTargetTexture> renderTargetTexture;
    renderTargetTexture.Attach(new RenderTargetTexture(device, backBuffer.Get(), nullptr));
    renderTargetTexture->GetSurfaceLevel(0, reinterpret_cast<Surface**>(renderTargetSurface.GetAddressOf()));

    return true;
}

void SwapChain::wait()
{
    if (pendingWait)
    {
        WaitForSingleObject(waitHandle, INFINITE);
        pendingWait = false;
    }
}

void SwapChain::present(Device* device, UINT syncInterval)
{
    wait();
    swapChain->Present(syncInterval, (syncInterval == 0) ? DXGI_PRESENT_ALLOW_TEARING : 0);
    pendingWait = true;
}
