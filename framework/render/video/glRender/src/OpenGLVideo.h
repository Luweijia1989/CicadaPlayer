#pragma once

#include "base/media/VideoFormat.h"
#include "base/media/IAFPacket.h"
#include "qmatrix4x4.h"

class VideoShader;
class OpenGLVideoPrivate;
/*!
 * \brief The OpenGLVideo class
 * high level api for renderering a video frame. use VideoShader, VideoMaterial and ShaderManager internally.
 * By default, VBO is used. Set environment var QTAV_NO_VBO=1 or 0 to disable/enable VBO.
 * VAO will be enabled if supported. Disabling VAO is the same as VBO.
 */
class OpenGLVideo {
public:
    enum MeshType { RectMesh, SphereMesh };
    static bool isSupported(VideoFormat::PixelFormat pixfmt);
    OpenGLVideo();
    ~OpenGLVideo();
    /*!
     * \brief setOpenGLContext
     * a context must be set before renderering.
     * \param ctx
     * 0: current context in OpenGL is done. shaders will be released.
     * QOpenGLContext is QObject in Qt5, and gl resources here will be released automatically if context is destroyed.
     * But you have to call setOpenGLContext(0) for Qt4 explicitly in the old context.
     * Viewport is also set here using context surface/paintDevice size and devicePixelRatio.
     * devicePixelRatio may be wrong for multi-screen with 5.0<qt<5.5, so you should call setProjectionMatrixToRect later in this case
     */

    void fill(const int &color);
    /*!
     * \brief render
     * all are in Qt's coordinate
     * \param target: the rect renderering to. in Qt's coordinate. not normalized here but in shader. // TODO: normalized check?
     * invalid value (default) means renderering to the whole viewport
     * \param roi: normalized rect of texture to renderer.
     * \param transform: additinal transformation.
     */
    void render(std::unique_ptr<IAFFrame> &frame, const QRectF &target = QRectF(), const QRectF &roi = QRectF(),
                const QMatrix4x4 &transform = QMatrix4x4());

    void setViewport(const QRectF &r);

    void setBrightness(double value);
    void setContrast(double value);
    void setHue(double value);
    void setSaturation(double value);

    void setUserShader(VideoShader *shader);
    VideoShader *userShader() const;

    void setMeshType(MeshType value);
    MeshType meshType() const;

private:
    OpenGLVideoPrivate *d;
};
