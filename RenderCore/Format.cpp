// Copyright 2015 XLGAMES Inc.
//
// Distributed under the MIT License (See
// accompanying file "LICENSE" or the website
// http://www.opensource.org/licenses/mit-license.php)

#include "Format.h"
#include "ResourceDesc.h"
#include "../../../Utility/ParameterBox.h"
#include "../../../Utility/StringUtils.h"

namespace RenderCore
{

    FormatCompressionType GetCompressionType(Format format)
    {
        switch (format) {
        #define _EXP(X, Y, Z, U)    case Format::X##_##Y: return FormatCompressionType::Z;
            #include "Metal/Detail/DXGICompatibleFormats.h"
        #undef _EXP
        default:
            return FormatCompressionType::None;
        }
    }

    /// Container for FormatPrefix::Enum
    namespace FormatPrefix
    {
        enum Enum 
        { 
            R32G32B32A32, R32G32B32, R16G16B16A16, R32G32, 
            R10G10B10A2, R11G11B10,
            R8G8B8A8, R16G16, R32, D32,
            R8G8, R16, D16, 
            R8, A8, A1, R1,
            R9G9B9E5, R8G8_B8G8, G8R8_G8B8,
            BC1, BC2, BC3, BC4, BC5, BC6H, BC7,
            B5G6R5, B5G5R5A1, B8G8R8A8, B8G8R8X8,
            Unknown
        };
    }

    static FormatPrefix::Enum   GetPrefix(Format format)
    {
        switch (format) {
        #define _EXP(X, Y, Z, U)    case Format::X##_##Y: return FormatPrefix::X;
            #include "Metal/Detail/DXGICompatibleFormats.h"
        #undef _EXP
        default: return FormatPrefix::Unknown;
        }
    }

    FormatComponents GetComponents(Format format)
    {
        FormatPrefix::Enum prefix = GetPrefix(format);
        using namespace FormatPrefix;
        switch (prefix) {
        case A8:
        case A1:                return FormatComponents::Alpha;

        case D32:
        case D16:               return FormatComponents::Depth; 

        case R32:
        case R16: 
        case R8:
        case R1:                return FormatComponents::Luminance;

        case B5G5R5A1:
        case B8G8R8A8:
        case R8G8B8A8:
        case R10G10B10A2:
        case R16G16B16A16:
        case R32G32B32A32:      return FormatComponents::RGBAlpha;
        case B5G6R5:
        case B8G8R8X8:
        case R11G11B10:
        case R32G32B32:         return FormatComponents::RGB;

        case R9G9B9E5:          return FormatComponents::RGBE;
            
        case R32G32:
        case R16G16:
        case R8G8:              return FormatComponents::RG;
            
        
        case BC1:               
        case BC6H:              return FormatComponents::RGB;

        case BC2:
        case BC3:
        case BC4: 
        case BC5:               
        case BC7:               return FormatComponents::RGBAlpha;

        case R8G8_B8G8: 
        case G8R8_G8B8:         return FormatComponents::RGB;
        }

        switch (format) {
        case Format::R24G8_TYPELESS:
        case Format::D24_UNORM_S8_UINT:
        case Format::R24_UNORM_X8_TYPELESS:
        case Format::X24_TYPELESS_G8_UINT:
        case Format::D32_SFLOAT_S8_UINT:
            return FormatComponents::DepthStencil;

        case Format::S8_UINT:
            return FormatComponents::Stencil;
        }

        return FormatComponents::Unknown;
    }

    FormatComponentType GetComponentType(Format format)
    {
        enum InputComponentType
        {
            TYPELESS, FLOAT, UINT, SINT, UNORM, SNORM, UNORM_SRGB, SHAREDEXP, UF16, SF16
        };
        InputComponentType input;
        switch (format) {
            #define _EXP(X, Y, Z, U)    case Format::X##_##Y: input = Y; break;
                #include "Metal/Detail/DXGICompatibleFormats.h"
            #undef _EXP
            case Format::S8_UINT: input = UINT; break;
            case Format::Matrix4x4: input = FLOAT; break;
            case Format::Matrix3x4: input = FLOAT; break;
            default: input = TYPELESS; break;
        }
        switch (input) {
        default:
        case TYPELESS:      return FormatComponentType::Typeless;
        case FLOAT:         return FormatComponentType::Float;
        case UINT:          return FormatComponentType::UInt;
        case SINT:          return FormatComponentType::SInt;
        case UNORM:         return FormatComponentType::UNorm;
        case SNORM:         return FormatComponentType::SNorm;
        case UNORM_SRGB:    return FormatComponentType::UNorm_SRGB;
        case SHAREDEXP:     return FormatComponentType::Exponential;
        case UF16:          return FormatComponentType::UnsignedFloat16;
        case SF16:          return FormatComponentType::SignedFloat16;
        }
    }

    unsigned                    BitsPerPixel(Format format)
    {
        switch (format) {
        #define _EXP(X, Y, Z, U)    case Format::X##_##Y: return U;
            #include "Metal/Detail/DXGICompatibleFormats.h"
        #undef _EXP
        case Format::D32_SFLOAT_S8_UINT: return 32 + 8;
        case Format::S8_UINT: return 8;
        case Format::Matrix4x4: return 16 * sizeof(float) * 8;
        case Format::Matrix3x4: return 12 * sizeof(float) * 8;
        default: return 0;
        }
    }

    unsigned    GetComponentPrecision(Format format)
    {
        return BitsPerPixel(format) / GetComponentCount(GetComponents(format));
    }

    unsigned    GetDecompressedComponentPrecision(Format format)
    {
        FormatPrefix::Enum prefix = GetPrefix(format);
        using namespace FormatPrefix;
        switch (prefix) {
        case BC1:
        case BC2:
        case BC3:
        case BC4:
        case BC5:   return 8;
        case BC6H:  return 16;
        case BC7:   return 8;   // (can be used for higher precision data)
        default:
            return GetComponentPrecision(format);
        }
    }

    unsigned    GetComponentCount(FormatComponents components)
    {
        switch (components) 
        {
        case FormatComponents::Alpha:
        case FormatComponents::Luminance: 
        case FormatComponents::Depth: 
        case FormatComponents::Stencil: return 1;

		case FormatComponents::LuminanceAlpha:
		case FormatComponents::RG: 
        case FormatComponents::DepthStencil: return 2;
        
		case FormatComponents::RGB: return 3;

        case FormatComponents::RGBAlpha:
        case FormatComponents::RGBE: return 4;

        default: return 0;
        }
    }

    Format FindFormat(
        FormatCompressionType compression, 
        FormatComponents components,
        FormatComponentType componentType,
        unsigned precision)
    {
        #define _EXP(X, Y, Z, U)                                                    \
            if (    compression == FormatCompressionType::Z                         \
                &&  components == GetComponents(Format::X##_##Y)              \
                &&  componentType == GetComponentType(Format::X##_##Y)        \
                &&  precision == GetComponentPrecision(Format::X##_##Y)) {    \
                return Format::X##_##Y;                                       \
            }                                                                       \
            /**/
            #include "Metal/Detail/DXGICompatibleFormats.h"
        #undef _EXP

        if (components == FormatComponents::RGB)
            return FindFormat(compression, FormatComponents::RGBAlpha, componentType, precision);

        return Format::Unknown;
    }


    Format AsSRGBFormat(Format inputFormat)
    {
        switch (inputFormat) {
        case Format::R8G8B8A8_TYPELESS:
        case Format::R8G8B8A8_UNORM: return Format::R8G8B8A8_UNORM_SRGB;
        case Format::BC1_TYPELESS:
        case Format::BC1_UNORM: return Format::BC1_UNORM_SRGB;
        case Format::BC2_TYPELESS:
        case Format::BC2_UNORM: return Format::BC2_UNORM_SRGB;
        case Format::BC3_TYPELESS:
        case Format::BC3_UNORM: return Format::BC3_UNORM_SRGB;
        case Format::BC7_TYPELESS:
        case Format::BC7_UNORM: return Format::BC7_UNORM_SRGB;

        case Format::B8G8R8A8_TYPELESS:
        case Format::B8G8R8A8_UNORM: return Format::B8G8R8A8_UNORM_SRGB;
        case Format::B8G8R8X8_TYPELESS:
        case Format::B8G8R8X8_UNORM: return Format::B8G8R8X8_UNORM_SRGB;
        }
        return inputFormat; // no linear/srgb version of this format exists
    }

    Format AsLinearFormat(Format inputFormat)
    {
        switch (inputFormat) {
        case Format::R8G8B8A8_TYPELESS:
        case Format::R8G8B8A8_UNORM_SRGB: return Format::R8G8B8A8_UNORM;
        case Format::BC1_TYPELESS:
        case Format::BC1_UNORM_SRGB: return Format::BC1_UNORM;
        case Format::BC2_TYPELESS:
        case Format::BC2_UNORM_SRGB: return Format::BC2_UNORM;
        case Format::BC3_TYPELESS:
        case Format::BC3_UNORM_SRGB: return Format::BC3_UNORM;
        case Format::BC7_TYPELESS:
        case Format::BC7_UNORM_SRGB: return Format::BC7_UNORM;

        case Format::B8G8R8A8_TYPELESS:
        case Format::B8G8R8A8_UNORM_SRGB: return Format::B8G8R8A8_UNORM;
        case Format::B8G8R8X8_TYPELESS:
        case Format::B8G8R8X8_UNORM_SRGB: return Format::B8G8R8X8_UNORM;
        }
        return inputFormat; // no linear/srgb version of this format exists
    }

    Format      AsTypelessFormat(Format inputFormat)
    {
            // note -- currently this only modifies formats that are also
            //          modified by AsSRGBFormat and AsLinearFormat. This is
            //          important, because this function is used to convert
            //          a pixel format for a texture that might be used by
            //          either a linear or srgb shader resource view.
            //          If this function changes formats aren't also changed
            //          by AsSRGBFormat and AsLinearFormat, it will cause some
            //          sources to fail to load correctly.
        switch (inputFormat) {
        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_UNORM_SRGB: return Format::R8G8B8A8_TYPELESS;
        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB: return Format::BC1_TYPELESS;
        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB: return Format::BC2_TYPELESS;
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB: return Format::BC3_TYPELESS;
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB: return Format::BC7_TYPELESS;

        case Format::B8G8R8A8_UNORM:
        case Format::B8G8R8A8_UNORM_SRGB: return Format::B8G8R8A8_TYPELESS;
        case Format::B8G8R8X8_UNORM:
        case Format::B8G8R8X8_UNORM_SRGB: return Format::B8G8R8X8_TYPELESS;

        case Format::D24_UNORM_S8_UINT:
        case Format::R24_UNORM_X8_TYPELESS:
        case Format::X24_TYPELESS_G8_UINT: return Format::R24G8_TYPELESS;

        case Format::R32_TYPELESS:
        case Format::D32_FLOAT:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT: return Format::R32_TYPELESS;

        case Format::S8_UINT: return Format::R8_TYPELESS;
        }
        return inputFormat; // no linear/srgb version of this format exists
    }

    Format AsDepthStencilFormat(Format inputFormat)
    {
        switch (inputFormat) {
        case Format::R24_UNORM_X8_TYPELESS:
        case Format::X24_TYPELESS_G8_UINT: 
        case Format::R24G8_TYPELESS:
            return Format::D24_UNORM_S8_UINT;

        case Format::R32_TYPELESS:
        case Format::R32_FLOAT:
            return Format::D32_FLOAT;

        case Format::R8_TYPELESS:
        case Format::R8_UINT:
            return Format::S8_UINT;
        }
        return inputFormat;
    }

    Format      AsDepthAspectSRVFormat(Format inputFormat)
    {
        switch (inputFormat) {
        case Format::D24_UNORM_S8_UINT:
        case Format::X24_TYPELESS_G8_UINT: 
        case Format::R24G8_TYPELESS:
            return Format::R24_UNORM_X8_TYPELESS;

        case Format::R32_TYPELESS:
        case Format::D32_FLOAT:
            return Format::R32_FLOAT;
        }
        return inputFormat;
    }

    Format      AsStencilAspectSRVFormat(Format inputFormat)
    {
        switch (inputFormat) {
        case Format::R24_UNORM_X8_TYPELESS:
        case Format::D24_UNORM_S8_UINT: 
        case Format::R24G8_TYPELESS:
            return Format::X24_TYPELESS_G8_UINT;

        case Format::R8_TYPELESS:
        case Format::S8_UINT:
            return Format::R8_UINT;
        }
        return inputFormat;
    }

    bool HasLinearAndSRGBFormats(Format inputFormat)
    {
        switch (inputFormat) {
        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_UNORM_SRGB:
        case Format::R8G8B8A8_TYPELESS:
        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB: 
        case Format::BC1_TYPELESS:
        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB: 
        case Format::BC2_TYPELESS:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB: 
        case Format::BC3_TYPELESS:
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB: 
        case Format::BC7_TYPELESS:
        case Format::B8G8R8A8_UNORM:
        case Format::B8G8R8A8_UNORM_SRGB: 
        case Format::B8G8R8A8_TYPELESS:
        case Format::B8G8R8X8_UNORM:
        case Format::B8G8R8X8_UNORM_SRGB: 
        case Format::B8G8R8X8_TYPELESS:
            return true;

        default: return false;
        }
    }

    Format AsFormat(
        const ImpliedTyping::TypeDesc& type,
        ShaderNormalizationMode norm)
    {
        if (type._type == ImpliedTyping::TypeCat::Float) {
            if (type._arrayCount == 1) return Format::R32_FLOAT;
            if (type._arrayCount == 2) return Format::R32G32_FLOAT;
            if (type._arrayCount == 3) return Format::R32G32B32_FLOAT;
            if (type._arrayCount == 4) return Format::R32G32B32A32_FLOAT;
            return Format::Unknown;
        }
        
        if (norm == ShaderNormalizationMode::Integer) {
            switch (type._type) {
            case ImpliedTyping::TypeCat::Int8:
                if (type._arrayCount == 1) return Format::R8_SINT;
                if (type._arrayCount == 2) return Format::R8G8_SINT;
                if (type._arrayCount == 4) return Format::R8G8B8A8_SINT;
                break;

            case ImpliedTyping::TypeCat::UInt8:
                if (type._arrayCount == 1) return Format::R8_UINT;
                if (type._arrayCount == 2) return Format::R8G8_UINT;
                if (type._arrayCount == 4) return Format::R8G8B8A8_UINT;
                break;

            case ImpliedTyping::TypeCat::Int16:
                if (type._arrayCount == 1) return Format::R16_SINT;
                if (type._arrayCount == 2) return Format::R16G16_SINT;
                if (type._arrayCount == 4) return Format::R16G16B16A16_SINT;
                break;

            case ImpliedTyping::TypeCat::UInt16:
                if (type._arrayCount == 1) return Format::R16_UINT;
                if (type._arrayCount == 2) return Format::R16G16_UINT;
                if (type._arrayCount == 4) return Format::R16G16B16A16_UINT;
                break;

            case ImpliedTyping::TypeCat::Int32:
                if (type._arrayCount == 1) return Format::R32_SINT;
                if (type._arrayCount == 2) return Format::R32G32_SINT;
                if (type._arrayCount == 3) return Format::R32G32B32_SINT;
                if (type._arrayCount == 4) return Format::R32G32B32A32_SINT;
                break;

            case ImpliedTyping::TypeCat::UInt32:
                if (type._arrayCount == 1) return Format::R32_UINT;
                if (type._arrayCount == 2) return Format::R32G32_UINT;
                if (type._arrayCount == 3) return Format::R32G32B32_UINT;
                if (type._arrayCount == 4) return Format::R32G32B32A32_UINT;
                break;
            }
        } else if (norm == ShaderNormalizationMode::Normalized) {
            switch (type._type) {
            case ImpliedTyping::TypeCat::Int8:
                if (type._arrayCount == 1) return Format::R8_SNORM;
                if (type._arrayCount == 2) return Format::R8G8_SNORM;
                if (type._arrayCount == 4) return Format::R8G8B8A8_SNORM;
                break;

            case ImpliedTyping::TypeCat::UInt8:
                if (type._arrayCount == 1) return Format::R8_UNORM;
                if (type._arrayCount == 2) return Format::R8G8_UNORM;
                if (type._arrayCount == 4) return Format::R8G8B8A8_UNORM;
                break;

            case ImpliedTyping::TypeCat::Int16:
                if (type._arrayCount == 1) return Format::R16_SNORM;
                if (type._arrayCount == 2) return Format::R16G16_SNORM;
                if (type._arrayCount == 4) return Format::R16G16B16A16_SNORM;
                break;

            case ImpliedTyping::TypeCat::UInt16:
                if (type._arrayCount == 1) return Format::R16_UNORM;
                if (type._arrayCount == 2) return Format::R16G16_UNORM;
                if (type._arrayCount == 4) return Format::R16G16B16A16_UNORM;
                break;
            }
        } else if (norm == ShaderNormalizationMode::Float) {
            switch (type._type) {
            case ImpliedTyping::TypeCat::Int16:
            case ImpliedTyping::TypeCat::UInt16:
                if (type._arrayCount == 1) return Format::R16_FLOAT;
                if (type._arrayCount == 2) return Format::R16G16_FLOAT;
                if (type._arrayCount == 4) return Format::R16G16B16A16_FLOAT;
                break;
            }
        }

        return Format::Unknown;
    }

    const char* AsString(Format format)
    {
        switch (format) {
            #define _EXP(X, Y, Z, U)    case Format::X##_##Y: return #X;
                #include "Metal/Detail/DXGICompatibleFormats.h"
            #undef _EXP
        case Format::D32_SFLOAT_S8_UINT: return "D32_FLOAT_S8_UINT";
        case Format::S8_UINT: return "S8_UINT";
        case Format::Matrix4x4: return "Matrix4x4";
        case Format::Matrix3x4: return "Matrix3x4";
        default: return "Unknown";
        }
    }

    #define STRINGIZE(X) #X

    Format AsFormat(const char name[])
    {
        #define _EXP(X, Y, Z, U)    if (XlEqStringI(name, STRINGIZE(X##_##Y))) return Format::X##_##Y;
            #include "Metal/Detail/DXGICompatibleFormats.h"
        #undef _EXP

        if (!XlEqStringI(name, "D32_FLOAT_S8_UINT")) return Format::D32_SFLOAT_S8_UINT;
        if (!XlEqStringI(name, "S8_UINT")) return Format::S8_UINT;
        if (!XlEqStringI(name, "Matrix4x4")) return Format::Matrix4x4;
        if (!XlEqStringI(name, "Matrix3x4")) return Format::Matrix3x4;
        return Format::Unknown;
    }

    Format ResolveFormat(Format baseFormat, TextureViewWindow::FormatFilter filter, FormatUsage usage)
    {
        // We need to filter the format by aspect and color space.
        // First, check if there are linear & SRGB versions of the format. If there are, we can ignore the "aspect" filter,
        // because these formats only have color aspects
        
        switch (filter._aspect) {
        default:
            return baseFormat;

        case TextureViewWindow::DepthStencil:
        case TextureViewWindow::Depth:
            if (usage == FormatUsage::DSV) return AsDepthStencilFormat(baseFormat);
            return AsDepthAspectSRVFormat(baseFormat);

        case TextureViewWindow::Stencil:
            if (usage == FormatUsage::DSV) return AsDepthStencilFormat(baseFormat);
            return AsStencilAspectSRVFormat(baseFormat);

        case TextureViewWindow::ColorLinear:
            return AsLinearFormat(baseFormat);

        case TextureViewWindow::ColorSRGB:
            return AsSRGBFormat(baseFormat);
        }
    }


}