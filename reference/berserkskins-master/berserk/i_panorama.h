#pragma once
#include <cstdint>
#include <d3d11.h>
#define D3D10_IGNORE_SDK_LAYERS
#define D3D10_IGNORE_DEPRECATED
#include <d3dcompiler.h>
#include "imgui/imgui.h"
#include "memory.h"
#include <stdio.h>
enum EImageFormat : std::uint32_t
{
    NONE = 0,
    DXT1 = 1,
    DXT5 = 2,
    I8 = 3,
    RGBA8888 = 4,
    R16 = 5,
    RG1616 = 6,
    RGBA16161616 = 7,
    R16F = 8,
    RG1616F = 9,
    RGBA16161616F = 10,
    R32F = 11,
    RG3232F = 12,
    RGB323232F = 13,
    RGBA32323232F = 14,
    JPEG_RGBA8888 = 15,
    PNG_RGBA8888 = 16,
    JPEG_DXT5 = 17,
    PNG_DXT5 = 18,
    BC6H = 19,
    BC7 = 20,
    ATI2N = 21,
    IA88 = 22,
    ETC2 = 23,
    ETC2_EAC = 24,
    R11_EAC = 25,
    RG11_EAC = 26,
    ATI1N = 27,
    BGRA8888 = 28,
};
struct ImageData_t
{
    ImageData_t()
    {
        ZeroMemory(this, sizeof(*this));

        m_iWidth = -1;
        m_iHeight = -1;
        m_iUnk1 = -1;
        m_iUnk2 = -1;
        m_flScale = 1.333f;

        m_iUnk3 = 1;
        m_iUnk4 = 1;
    }

    const char* m_szImagePath; // 0x0
    int m_iWidth; // 0x8
    int m_iHeight; // 0xC
    int m_iUnk1; // 0x10
    int m_iUnk2; // 0x14
    float m_flScale; // 0x18
    char padd[0x30]; // 0x1C
    int m_iUnk3; // 0x1C
    char pading[0x18]; // 0x20
    int m_iUnk4; // 0x38
    char padi[0x2C]; // 0x3C
};

class CTextureDx11
{
public:
    char pad1[0x10]; // 0x0
    ID3D11ShaderResourceView* m_pShaderResourceViewSRGB; // 0x10
    ID3D11ShaderResourceView* m_pShaderResourceViewUNORM; // 0x18
};

class CSource2UITexture
{
public:
    class CData
    {
    public:
        CTextureDx11* m_pDx11Texture; // 0x0
    };

    char pad1[0x28]; // 0x0
    CData* m_pData; // 0x28
    int m_iWidth; // 0x30
    int m_iHeight; // 0x43
};

class CImageData
{
public:
    char padd[0x30]; // 0x0
    CSource2UITexture* m_pUITexture; // 0x30
};

class CImageProxySource
{
public:
    char padx[0x10]; // 0x0
    CImageData* m_pImageData; // 0x10

    CSource2UITexture* GetTextureID()
    {
        return  M::call_virtual<CSource2UITexture*>(this,4);
    }

    void AddRef()
    {
        return M::CallVFunc<void, 10U>(this);
    }

    void Release()
    {
        return M::CallVFunc<void, 11U>(this);
    }

    ID3D11ShaderResourceView* GetNativeTexture()
    {
        if (!this)
            return nullptr;

        CSource2UITexture* pUITexture = this->GetTextureID();
        if (!pUITexture)
            return nullptr;

        CSource2UITexture::CData* pUITextureData = pUITexture->m_pData;
        if (!pUITextureData)
            return nullptr;

        CTextureDx11* pDX11Texture = pUITextureData->m_pDx11Texture;
        if (!pDX11Texture)
            return nullptr;

        return pDX11Texture->m_pShaderResourceViewSRGB;
    }

    ImVec2 GetImageSize()
    {
        if (!this)
            return ImVec2(0, 0);

        CSource2UITexture* pUITexture = this->GetTextureID();
        if (!pUITexture)
            return ImVec2(0, 0);

        return ImVec2(static_cast<float>(pUITexture->m_iWidth), static_cast<float>(pUITexture->m_iHeight));
    }
};

class CImagePanel
{
public:
    char padad1[0xD0]; // 0x0
    CImageProxySource* m_pImageProxy; // 0xD0
};

using LoadImageInternal_t = CImageProxySource * (__fastcall*)(
    void* pThis,
    void* unk1,
    void* unk2,
    const char* szImagePath,
    EImageFormat format,
    ImageData_t* pImageData
    );

class CImageResourceManager
{
public:

    CImageProxySource* LoadImageInternal(const char* szImagePath, EImageFormat eImageFormat)
    {
        ImageData_t imageData{};
        imageData.m_szImagePath = szImagePath;

        return LoadImageFromURL(nullptr, nullptr, szImagePath, eImageFormat, &imageData);
    }

    // file://
    // http://
    // https://
    // s2r://
    // raw://
    // panel://
    // panel-background://

    CImageProxySource* LoadImageFromURL(void* pPanel, void* pDefaultResourceURL, const char* szResourceURL, EImageFormat eImageFormat, ImageData_t* pImageFormatOut)
    {
        return M::call_virtual<CImageProxySource*>(this, 0, pPanel, pDefaultResourceURL, szResourceURL, eImageFormat, pImageFormatOut);
    }


};

class CUIEngineSource2
{
public:
    CImageResourceManager* GetResourceManager()
    {
        return M::CallVFunc<CImageResourceManager*, 23U>(this);
    }
};

// panorama.dll @ PanoramaUIEngine001
class i_panorama
{
public:
    CUIEngineSource2* AccessUIEngine()
    {
        return M::CallVFunc<CUIEngineSource2*, 13U>(this);
    }
};