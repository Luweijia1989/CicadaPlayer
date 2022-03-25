#include "glwidget.h"
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

OpenGLWidget::OpenGLWidget()
{
    //   setAttribute(Qt::WA_TranslucentBackground);
    //setWindowFlag(Qt::FramelessWindowHint);

    player = std::shared_ptr<MediaPlayer>(new MediaPlayer());

    player->setVideoSurfaceSize(1, 1);
    player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); });
    //player->setMaskMode(IVideoRender::Mask_Right, u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");
    player->SetRotateMode(ROTATE_MODE_0);
    player->SetScaleMode(SM_FIT);

    //player->SetSpeed(0.5);
    player->SetDefaultBandWidth(1000 * 1000);
    //player->SetDataSource("http://player.alicdn.com/video/aliyunmedia.mp4");
    //player->SetDataSource("E:\\vap1.mp4");
    player->SetDataSource("C:\\Users\\posat\\Desktop\\7p.mp4");
    player->SetAutoPlay(true);
    player->SetLoop(true);
    player->SetIPResolveType(IpResolveWhatEver);
    player->SetFastStart(true);
    MediaPlayerConfig config = *(player->GetConfig());
    config.mMaxBackwardBufferDuration = 20000;
    config.liveStartIndex = -3;
    player->SetConfig(&config);
    player->Prepare();
    player->SelectTrack(-1);
}

OpenGLWidget::~OpenGLWidget()
{
    player->setRenderCallback(nullptr);
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
                    sp->foreignGLContextDestroyed();
                    return;
                }
                sp->clearGLResource();
            },
            Qt::DirectConnection);
}

void OpenGLWidget::resizeGL(int w, int h)
{
    player->setVideoSurfaceSize(w * devicePixelRatio(), h * devicePixelRatio());
}

void OpenGLWidget::paintGL()
{
    player->renderVideo();
}
