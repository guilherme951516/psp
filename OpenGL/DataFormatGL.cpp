#include "Common/GPU/OpenGL/DataFormatGL.h"
#include "Common/GPU/OpenGL/GLFeatures.h"
#include "Common/Log.h"

namespace Draw {

bool Thin3DFormatToGLFormatAndType(DataFormat fmt, GLuint &internalFormat, GLuint &format, GLuint &type, int &alignment) {
    alignment = 4;
    switch (fmt) {
    case DataFormat::R16_UNORM:
        internalFormat = GL_RGBA;
        format = GL_RED;
        type = GL_UNSIGNED_SHORT;
        alignment = 2;
        break;

    case DataFormat::R8_UNORM:
#ifdef __EMSCRIPTEN__
        // WebGL 2 suporta RED
        internalFormat = GL_R8;
        format = GL_RED;
#else
        if (gl_extensions.IsGLES) {
            internalFormat = GL_LUMINANCE;
            format = GL_LUMINANCE;
        } else if (gl_extensions.VersionGEThan(3, 0)) {
            internalFormat = GL_RED;
            format = GL_RED;
        } else {
            internalFormat = GL_RGBA;
            format = GL_RED;
        }
#endif
        type = GL_UNSIGNED_BYTE;
        alignment = 1;
        break;

    case DataFormat::R8G8B8A8_UNORM:
        internalFormat = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;

    case DataFormat::D32F:
        internalFormat = GL_DEPTH_COMPONENT;
        format = GL_DEPTH_COMPONENT;
        type = GL_FLOAT;
        break;

#if !defined(USING_GLES2) || defined(__EMSCRIPTEN__)
    case DataFormat::S8:
#ifdef __EMSCRIPTEN__
        // WebGL 2 não suporta GL_STENCIL_INDEX sozinho
        internalFormat = GL_DEPTH24_STENCIL8;
        format = GL_DEPTH_STENCIL;
        type = GL_UNSIGNED_INT_24_8;
#else
        internalFormat = GL_STENCIL_INDEX;
        format = GL_STENCIL_INDEX;
        type = GL_UNSIGNED_BYTE;
#endif
        alignment = 1;
        break;
#endif

    case DataFormat::R8G8B8_UNORM:
        internalFormat = GL_RGB;
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        alignment = 3;
        break;

    case DataFormat::R4G4B4A4_UNORM_PACK16:
        internalFormat = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_SHORT_4_4_4_4;
        alignment = 2;
        break;

    case DataFormat::R5G6B5_UNORM_PACK16:
        internalFormat = GL_RGB;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_6_5;
        alignment = 2;
        break;

    case DataFormat::R5G5B5A1_UNORM_PACK16:
        internalFormat = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_SHORT_5_5_5_1;
        alignment = 2;
        break;

    case DataFormat::R32G32B32A32_FLOAT:
        internalFormat = GL_RGBA32F;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;

#if !defined(USING_GLES2) && !defined(__EMSCRIPTEN__)
    // WebGL não suporta compressão S3TC/BC nativamente
    case DataFormat::BC1_RGBA_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        format = GL_RGB;
        type = GL_FLOAT;
        alignment = 8;
        break;
    case DataFormat::BC2_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;
    case DataFormat::BC3_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;
    case DataFormat::BC4_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RED_RGTC1;
        format = GL_R;
        type = GL_FLOAT;
        alignment = 16;
        break;
    case DataFormat::BC5_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RG_RGTC2;
        format = GL_RG;
        type = GL_FLOAT;
        alignment = 16;
        break;
    case DataFormat::BC7_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;
#endif
    case DataFormat::ETC2_R8G8B8_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGB8_ETC2;
        format = GL_RGB;
        type = GL_FLOAT;
        alignment = 8;
        break;

    case DataFormat::ETC2_R8G8B8A1_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;

    case DataFormat::ETC2_R8G8B8A8_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;

#ifdef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
    case DataFormat::ASTC_4x4_UNORM_BLOCK:
        internalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        format = GL_RGBA;
        type = GL_FLOAT;
        alignment = 16;
        break;
#endif

    default:
        return false;
    }
    return true;
}

}