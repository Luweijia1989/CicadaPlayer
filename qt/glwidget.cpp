#include "glwidget.h"
#include "MediaPlayer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <qtimer.h>

using namespace Cicada;
OpenGLWidget::OpenGLWidget(const std::shared_ptr<MediaPlayer> &p) : player(p)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);
    player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);

	//QTimer::singleShot(300, [=]() {deleteLater();});
}

OpenGLWidget::~OpenGLWidget()
{
    player->setRenderCallback(nullptr, this);
    //player->clearGLResource();
}

void OpenGLWidget::initializeGL()
{
    std::weak_ptr<MediaPlayer> wp = player;
    connect(
            context(), &QOpenGLContext::aboutToBeDestroyed, this,
            [=] {
                auto cc = context()->currentContext();
                auto sp = wp.lock();
                if (!sp) {
                    sp->foreignGLContextDestroyed(this);
                    return;
                }
                sp->clearGLResource(this);
            },
            Qt::DirectConnection);
}

void OpenGLWidget::resizeGL(int w, int h)
{
    player->setVideoSurfaceSize(w * devicePixelRatio(), h * devicePixelRatio(), this);
}

void OpenGLWidget::paintGL()
{
    player->renderVideo(this);
}
