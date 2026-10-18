#pragma once

#include <cstdint>
#include <format>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#pragma push_macro("WINSOCK_API_LINKAGE")
#undef WINSOCK_API_LINKAGE
#define WINSOCK_API_LINKAGE
#include <winsock.h>
#pragma pop_macro("WINSOCK_API_LINKAGE")
#include <windows.h>

#include <d3d.h>
#include <ddraw.h>

namespace gems::impl
{

auto format_dword_flags(::DWORD flags, const std::unordered_map<::DWORD, std::string_view> &lookup) -> std::string
{
    auto strm = std::stringstream{};
    auto value = flags;
    auto first = true;

    for (const auto &[flag, name] : lookup)
    {
        if (value == 0)
        {
            break;
        }

        if (value & flag)
        {
            if (!first)
            {
                strm << " | ";
            }

            strm << name;
            first = false;

            value &= ~flag;
        }
    }

    if (value != 0)
    {
        strm << " | <unknown>";
    }

    return std::format("{:x} [{}]", flags, strm.str());
}

}

template <>
struct std::formatter<::GUID>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const ::GUID &obj, std::format_context &ctx) const
    {
        if (::IsEqualGUID(obj, IID_IUnknown))
        {
            return std::format_to(ctx.out(), "IID_IUnknown");
        }
        else if (::IsEqualGUID(obj, IID_IDirectDraw))
        {
            return std::format_to(ctx.out(), "IID_IDirectDraw");
        }
        else if (::IsEqualGUID(obj, IID_IDirectDraw2))
        {
            return std::format_to(ctx.out(), "IID_IDirectDraw2");
        }
        else if (::IsEqualGUID(obj, IID_IDirectDraw3))
        {
            return std::format_to(ctx.out(), "IID_IDirectDraw3");
        }
        else if (::IsEqualGUID(obj, IID_IDirectDraw4))
        {
            return std::format_to(ctx.out(), "IID_IDirectDraw4");
        }
        else if (::IsEqualGUID(obj, IID_IDirectDraw7))
        {
            return std::format_to(ctx.out(), "IID_IDirectDraw7");
        }
        else if (::IsEqualGUID(obj, IID_IDirect3D3))
        {
            return std::format_to(ctx.out(), "IID_IDirect3D3");
        }
        else if (::IsEqualGUID(obj, IID_IDirect3DHALDevice))
        {
            return std::format_to(ctx.out(), "IID_IDirect3DHALDevice");
        }

        return std::format_to(
            ctx.out(),
            "{{{:08x}-{:04x}-{:04x}-{:02x}{:02x}-"
            "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}}}",
            obj.Data1,
            obj.Data2,
            obj.Data3,
            obj.Data4[0],
            obj.Data4[1],
            obj.Data4[2],
            obj.Data4[3],
            obj.Data4[4],
            obj.Data4[5],
            obj.Data4[6],
            obj.Data4[7]);
    }
};

template <>
struct std::formatter<::HWND>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const ::HWND &obj, std::format_context &ctx) const
    {
        return std::format_to(ctx.out(), "{}", reinterpret_cast<void *>(obj));
    }
};

template <>
struct std::formatter<::in_addr>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const ::in_addr &obj, std::format_context &ctx) const
    {
        const auto *bytes = reinterpret_cast<const unsigned char *>(&obj.s_addr);
        return std::format_to(ctx.out(), "{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
    }
};

template <>
struct std::formatter<::sockaddr>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const ::sockaddr &obj, std::format_context &ctx) const
    {
        const auto *bytes = reinterpret_cast<const unsigned char *>(obj.sa_data);
        if (obj.sa_family == AF_INET)
        {
            const auto port = (static_cast<unsigned int>(bytes[0]) << 8) | bytes[1];
            return std::format_to(
                ctx.out(),
                "sockaddr{{family=AF_INET address={}.{}.{}.{} port={}}}",
                bytes[2],
                bytes[3],
                bytes[4],
                bytes[5],
                port);
        }

        auto data = std::string{};
        for (const auto byte : obj.sa_data)
        {
            std::format_to(std::back_inserter(data), "{:02x}", static_cast<unsigned char>(byte));
        }

        return std::format_to(ctx.out(), "sockaddr{{family={} data=0x{}}}", obj.sa_family, data);
    }
};

template <>
struct std::formatter<::hostent>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const ::hostent &obj, std::format_context &ctx) const
    {
        auto fields = std::format("name={} aliases=[", obj.h_name ? obj.h_name : "<null>");

        if (obj.h_aliases)
        {
            for (auto alias = obj.h_aliases; *alias; ++alias)
            {
                std::format_to(std::back_inserter(fields), "{}{}", alias == obj.h_aliases ? "" : ", ", *alias);
            }
        }

        std::format_to(std::back_inserter(fields), "] addrtype={} length={} addresses=[", obj.h_addrtype, obj.h_length);

        if (obj.h_addr_list)
        {
            for (auto address = obj.h_addr_list; *address; ++address)
            {
                if (address != obj.h_addr_list)
                {
                    fields += ", ";
                }

                const auto bytes = reinterpret_cast<const unsigned char *>(*address);
                if (obj.h_addrtype == AF_INET)
                {
                    if (obj.h_length != 4)
                    {
                        fields += "<invalid length>";
                        continue;
                    }

                    std::format_to(std::back_inserter(fields), "{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
                }
#ifdef AF_INET6
                else if (obj.h_addrtype == AF_INET6)
                {
                    if (obj.h_length != 16)
                    {
                        fields += "<invalid length>";
                        continue;
                    }

                    for (auto i = 0; i < 16; i += 2)
                    {
                        std::format_to(
                            std::back_inserter(fields), "{}{:02x}{:02x}", i == 0 ? "" : ":", bytes[i], bytes[i + 1]);
                    }
                }
#endif
                else
                {
                    if (obj.h_length <= 0)
                    {
                        fields += "<invalid length>";
                        continue;
                    }

                    fields += "0x";
                    for (auto i = 0; i < obj.h_length; ++i)
                    {
                        std::format_to(std::back_inserter(fields), "{:02x}", bytes[i]);
                    }
                }
            }
        }

        fields += "]";
        return std::format_to(ctx.out(), "hostent{{{}}}", fields);
    }
};

struct SetCooperativeLevelFlags
{
    ::DWORD flags;
};

template <>
struct std::formatter<SetCooperativeLevelFlags>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const SetCooperativeLevelFlags &obj, std::format_context &ctx) const
    {
        static auto lookup = std::unordered_map<::DWORD, std::string_view>{
            {DDSCL_ALLOWMODEX, "DDSCL_ALLOWMODEX"},
            {DDSCL_ALLOWREBOOT, "DDSCL_ALLOWREBOOT"},
            {DDSCL_CREATEDEVICEWINDOW, "DDSCL_CREATEDEVICEWINDOW"},
            {DDSCL_EXCLUSIVE, "DDSCL_EXCLUSIVE"},
            {DDSCL_FPUPRESERVE, "DDSCL_FPUPRESERVE"},
            {DDSCL_FPUSETUP, "DDSCL_FPUSETUP"},
            {DDSCL_FULLSCREEN, "DDSCL_FULLSCREEN"},
            {DDSCL_MULTITHREADED, "DDSCL_MULTITHREADED"},
            {DDSCL_NORMAL, "DDSCL_NORMAL"},
            {DDSCL_NOWINDOWCHANGES, "DDSCL_NOWINDOWCHANGES"},
            {DDSCL_SETDEVICEWINDOW, "DDSCL_SETDEVICEWINDOW"},
            {DDSCL_SETFOCUSWINDOW, "DDSCL_SETFOCUSWINDOW"},
        };

        return std::format_to(ctx.out(), "{}", gems::impl::format_dword_flags(obj.flags, lookup));
    }
};

template <>
struct std::formatter<::DDSURFACEDESC>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const ::DDSURFACEDESC &obj, std::format_context &ctx) const
    {
        static const auto desc_flag_names = std::unordered_map<::DWORD, std::string_view>{
            {DDSD_CAPS, "DDSD_CAPS"},
            {DDSD_HEIGHT, "DDSD_HEIGHT"},
            {DDSD_WIDTH, "DDSD_WIDTH"},
            {DDSD_PITCH, "DDSD_PITCH"},
            {DDSD_BACKBUFFERCOUNT, "DDSD_BACKBUFFERCOUNT"},
            {DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH"},
            {DDSD_ALPHABITDEPTH, "DDSD_ALPHABITDEPTH"},
            {DDSD_LPSURFACE, "DDSD_LPSURFACE"},
            {DDSD_PIXELFORMAT, "DDSD_PIXELFORMAT"},
            {DDSD_CKDESTOVERLAY, "DDSD_CKDESTOVERLAY"},
            {DDSD_CKDESTBLT, "DDSD_CKDESTBLT"},
            {DDSD_CKSRCOVERLAY, "DDSD_CKSRCOVERLAY"},
            {DDSD_CKSRCBLT, "DDSD_CKSRCBLT"},
            {DDSD_MIPMAPCOUNT, "DDSD_MIPMAPCOUNT"},
            {DDSD_REFRESHRATE, "DDSD_REFRESHRATE"},
            {DDSD_LINEARSIZE, "DDSD_LINEARSIZE"},
        };
        static const auto caps_flag_names = std::unordered_map<::DWORD, std::string_view>{
            {DDSCAPS_RESERVED1, "DDSCAPS_RESERVED1"},
            {DDSCAPS_ALPHA, "DDSCAPS_ALPHA"},
            {DDSCAPS_BACKBUFFER, "DDSCAPS_BACKBUFFER"},
            {DDSCAPS_COMPLEX, "DDSCAPS_COMPLEX"},
            {DDSCAPS_FLIP, "DDSCAPS_FLIP"},
            {DDSCAPS_FRONTBUFFER, "DDSCAPS_FRONTBUFFER"},
            {DDSCAPS_OFFSCREENPLAIN, "DDSCAPS_OFFSCREENPLAIN"},
            {DDSCAPS_OVERLAY, "DDSCAPS_OVERLAY"},
            {DDSCAPS_PALETTE, "DDSCAPS_PALETTE"},
            {DDSCAPS_PRIMARYSURFACE, "DDSCAPS_PRIMARYSURFACE"},
            {DDSCAPS_PRIMARYSURFACELEFT, "DDSCAPS_PRIMARYSURFACELEFT"},
            {DDSCAPS_SYSTEMMEMORY, "DDSCAPS_SYSTEMMEMORY"},
            {DDSCAPS_TEXTURE, "DDSCAPS_TEXTURE"},
            {DDSCAPS_3DDEVICE, "DDSCAPS_3DDEVICE"},
            {DDSCAPS_VIDEOMEMORY, "DDSCAPS_VIDEOMEMORY"},
            {DDSCAPS_VISIBLE, "DDSCAPS_VISIBLE"},
            {DDSCAPS_WRITEONLY, "DDSCAPS_WRITEONLY"},
            {DDSCAPS_ZBUFFER, "DDSCAPS_ZBUFFER"},
            {DDSCAPS_OWNDC, "DDSCAPS_OWNDC"},
            {DDSCAPS_LIVEVIDEO, "DDSCAPS_LIVEVIDEO"},
            {DDSCAPS_HWCODEC, "DDSCAPS_HWCODEC"},
            {DDSCAPS_MODEX, "DDSCAPS_MODEX"},
            {DDSCAPS_MIPMAP, "DDSCAPS_MIPMAP"},
            {DDSCAPS_RESERVED2, "DDSCAPS_RESERVED2"},
            {DDSCAPS_ALLOCONLOAD, "DDSCAPS_ALLOCONLOAD"},
            {DDSCAPS_VIDEOPORT, "DDSCAPS_VIDEOPORT"},
            {DDSCAPS_LOCALVIDMEM, "DDSCAPS_LOCALVIDMEM"},
            {DDSCAPS_NONLOCALVIDMEM, "DDSCAPS_NONLOCALVIDMEM"},
            {DDSCAPS_STANDARDVGAMODE, "DDSCAPS_STANDARDVGAMODE"},
            {DDSCAPS_OPTIMIZED, "DDSCAPS_OPTIMIZED"},
        };
        static const auto pixel_flag_names = std::unordered_map<::DWORD, std::string_view>{
            {DDPF_ALPHAPIXELS, "DDPF_ALPHAPIXELS"},
            {DDPF_ALPHA, "DDPF_ALPHA"},
            {DDPF_FOURCC, "DDPF_FOURCC"},
            {DDPF_PALETTEINDEXED4, "DDPF_PALETTEINDEXED4"},
            {DDPF_PALETTEINDEXEDTO8, "DDPF_PALETTEINDEXEDTO8"},
            {DDPF_PALETTEINDEXED8, "DDPF_PALETTEINDEXED8"},
            {DDPF_RGB, "DDPF_RGB"},
            {DDPF_COMPRESSED, "DDPF_COMPRESSED"},
            {DDPF_RGBTOYUV, "DDPF_RGBTOYUV"},
            {DDPF_YUV, "DDPF_YUV"},
            {DDPF_ZBUFFER, "DDPF_ZBUFFER"},
            {DDPF_PALETTEINDEXED1, "DDPF_PALETTEINDEXED1"},
            {DDPF_PALETTEINDEXED2, "DDPF_PALETTEINDEXED2"},
            {DDPF_ZPIXELS, "DDPF_ZPIXELS"},
            {DDPF_STENCILBUFFER, "DDPF_STENCILBUFFER"},
            {DDPF_ALPHAPREMULT, "DDPF_ALPHAPREMULT"},
            {DDPF_LUMINANCE, "DDPF_LUMINANCE"},
            {DDPF_BUMPLUMINANCE, "DDPF_BUMPLUMINANCE"},
            {DDPF_BUMPDUDV, "DDPF_BUMPDUDV"},
        };

        auto fields =
            std::format("size={} flags={}", obj.dwSize, gems::impl::format_dword_flags(obj.dwFlags, desc_flag_names));

        const auto append = [&fields](std::string_view name, auto value)
        { std::format_to(std::back_inserter(fields), " {}={}", name, value); };
        const auto append_hex = [&fields](std::string_view name, ::DWORD value)
        { std::format_to(std::back_inserter(fields), " {}=0x{:08x}", name, value); };
        const auto append_color_key = [&fields](std::string_view name, const ::DDCOLORKEY &key)
        {
            std::format_to(
                std::back_inserter(fields),
                " {}=[0x{:08x}-0x{:08x}]",
                name,
                key.dwColorSpaceLowValue,
                key.dwColorSpaceHighValue);
        };

        if (obj.dwFlags & DDSD_WIDTH)
        {
            append("width", obj.dwWidth);
        }
        if (obj.dwFlags & DDSD_HEIGHT)
        {
            append("height", obj.dwHeight);
        }
        if (obj.dwFlags & DDSD_PITCH)
        {
            append("pitch", obj.lPitch);
        }
        if (obj.dwFlags & DDSD_LINEARSIZE)
        {
            append("linearSize", obj.dwLinearSize);
        }
        if (obj.dwFlags & DDSD_BACKBUFFERCOUNT)
        {
            append("backBuffers", obj.dwBackBufferCount);
        }
        if (obj.dwFlags & DDSD_MIPMAPCOUNT)
        {
            append("mipMaps", obj.dwMipMapCount);
        }
        if (obj.dwFlags & DDSD_ZBUFFERBITDEPTH)
        {
            append("zBufferBits", obj.dwZBufferBitDepth);
        }
        if (obj.dwFlags & DDSD_REFRESHRATE)
        {
            append("refreshRate", obj.dwRefreshRate);
        }
        if (obj.dwFlags & DDSD_ALPHABITDEPTH)
        {
            append("alphaBits", obj.dwAlphaBitDepth);
        }
        if (obj.dwFlags & DDSD_LPSURFACE)
        {
            append("surface", obj.lpSurface);
        }
        if (obj.dwFlags & DDSD_CKDESTOVERLAY)
        {
            append_color_key("destOverlayKey", obj.ddckCKDestOverlay);
        }
        if (obj.dwFlags & DDSD_CKDESTBLT)
        {
            append_color_key("destBltKey", obj.ddckCKDestBlt);
        }
        if (obj.dwFlags & DDSD_CKSRCOVERLAY)
        {
            append_color_key("srcOverlayKey", obj.ddckCKSrcOverlay);
        }
        if (obj.dwFlags & DDSD_CKSRCBLT)
        {
            append_color_key("srcBltKey", obj.ddckCKSrcBlt);
        }
        if (obj.dwFlags & DDSD_CAPS)
        {
            append("caps", gems::impl::format_dword_flags(obj.ddsCaps.dwCaps, caps_flag_names));
        }
        if (obj.dwFlags & DDSD_PIXELFORMAT)
        {
            const auto &pixel = obj.ddpfPixelFormat;
            append("pixel.size", pixel.dwSize);
            append("pixel.flags", gems::impl::format_dword_flags(pixel.dwFlags, pixel_flag_names));

            if (pixel.dwFlags & DDPF_FOURCC)
            {
                append_hex("pixel.fourCC", pixel.dwFourCC);
            }
            if (pixel.dwFlags & DDPF_RGB)
            {
                append("pixel.rgbBits", pixel.dwRGBBitCount);
                append_hex("pixel.rMask", pixel.dwRBitMask);
                append_hex("pixel.gMask", pixel.dwGBitMask);
                append_hex("pixel.bMask", pixel.dwBBitMask);
                append_hex("pixel.aMask", pixel.dwRGBAlphaBitMask);
            }
            if (pixel.dwFlags & (DDPF_YUV | DDPF_RGBTOYUV))
            {
                append("pixel.yuvBits", pixel.dwYUVBitCount);
                append_hex("pixel.yMask", pixel.dwYBitMask);
                append_hex("pixel.uMask", pixel.dwUBitMask);
                append_hex("pixel.vMask", pixel.dwVBitMask);
                append_hex("pixel.aMask", pixel.dwYUVAlphaBitMask);
            }
            if (pixel.dwFlags & DDPF_ZBUFFER)
            {
                append("pixel.zBits", pixel.dwZBufferBitDepth);
                append_hex("pixel.zMask", pixel.dwZBitMask);
                append_hex("pixel.zAlphaMask", pixel.dwRGBZBitMask);
            }
            if (pixel.dwFlags & DDPF_STENCILBUFFER)
            {
                append("pixel.stencilBits", pixel.dwStencilBitDepth);
                append_hex("pixel.stencilMask", pixel.dwStencilBitMask);
            }
            if (pixel.dwFlags & DDPF_ALPHA)
            {
                append("pixel.alphaBits", pixel.dwAlphaBitDepth);
                append_hex("pixel.alphaMask", pixel.dwRGBAlphaBitMask);
            }
            if (pixel.dwFlags & DDPF_LUMINANCE)
            {
                append("pixel.luminanceBits", pixel.dwLuminanceBitCount);
                append_hex("pixel.luminanceMask", pixel.dwLuminanceBitMask);
                append_hex("pixel.luminanceAlphaMask", pixel.dwLuminanceAlphaBitMask);
            }
            if (pixel.dwFlags & (DDPF_BUMPLUMINANCE | DDPF_BUMPDUDV))
            {
                append("pixel.bumpBits", pixel.dwBumpBitCount);
                append_hex("pixel.bumpDuMask", pixel.dwBumpDuBitMask);
                append_hex("pixel.bumpDvMask", pixel.dwBumpDvBitMask);
                append_hex("pixel.bumpLuminanceMask", pixel.dwBumpLuminanceBitMask);
            }
        }

        return std::format_to(ctx.out(), "DDSURFACEDESC{{{}}}", fields);
    }
};
