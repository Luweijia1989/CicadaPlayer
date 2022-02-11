#include "glwidget.h"
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

OpenGLWidget::OpenGLWidget()
{


	player = std::shared_ptr<MediaPlayer>(new MediaPlayer());

    player->setVideoSurfaceSize(1, 1);
    player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); });

    player->SetDefaultBandWidth(1000 * 1000);
    player->SetDataSource("http://player.alicdn.com/video/aliyunmedia.mp4");
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
	player->setVideoSurfaceSize(-1, -1);
}

void OpenGLWidget::initializeGL()
{
	std::weak_ptr<MediaPlayer> wp = player;
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [=] {
		auto sp = wp.lock();
		if (sp)
			sp->setVideoSurfaceSize(-1, -1);
    }, Qt::DirectConnection);
}

void OpenGLWidget::resizeGL(int w, int h)
{
    player->setVideoSurfaceSize(w*devicePixelRatio(), h*devicePixelRatio());
}

void OpenGLWidget::paintGL()
{
    player->renderVideo();
}
