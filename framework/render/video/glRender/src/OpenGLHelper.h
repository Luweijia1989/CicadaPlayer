#pragma once

#include "base/media/VideoFormat.h"
#include "platform/platform_gl.h"
#include "qmatrix4x4.h"

namespace OpenGLHelper {
    enum ShaderType {
        Vertex,
        Fragment,
    };
    std::string removeComments(const std::string &code);
    std::string compatibleShaderHeader(ShaderType type);
    int GLSLVersion();
    int GLMajorVersion();
    bool isEGL();
    bool isOpenGLES();
    /*!
 * \brief hasExtensionEGL
 * Test if any of the given extensions is supported
 * \param exts Ends with NULL
 * \return true if one of extension is supported
 */
    bool hasExtensionEGL(const char *exts[]);
    bool hasRG();
    bool has16BitTexture();
    // set by user (environment var "QTAV_TEXTURE16_DEPTH=8 or 16", default now is 8)
    int depth16BitTexture();
    // set by user (environment var "QTAV_GL_DEPRECATED=1")
    bool useDeprecatedFormats();
    /*!
 * \brief hasExtension
 * Test if any of the given extensions is supported. Current OpenGL context must be valid.
 * \param exts Ends with NULL
 * \return true if one of extension is supported
 */
    bool hasExtension(const char *exts[]);
    bool isPBOSupported();
    /*!
 * \brief videoFormatToGL
 * \param fmt
 * \param internal_format an array with size fmt.planeCount()
 * \param data_format an array with size fmt.planeCount()
 * \param data_type an array with size fmt.planeCount()
 * \param mat channel reorder matrix used in glsl
 * \return false if fmt is not supported
 */
    bool videoFormatToGL(const VideoFormat &fmt, GLint *internal_format, GLenum *data_format, GLenum *data_type, QMatrix4x4 *mat = NULL);
    int bytesOfGLFormat(GLenum format, GLenum dataType = GL_UNSIGNED_BYTE);
}//namespace OpenGLHelper
