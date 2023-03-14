#include "qmlrender.h"
#include <QOpenGLFramebufferObjectFormat>

class VideoRendererInternal : public QQuickFramebufferObject::Renderer
{
public:
    VideoRendererInternal(const std::shared_ptr< MediaPlayer > &p, void *vo)
        : player(p)
        , m_vo(vo)
    {
    }

    ~VideoRendererInternal()
    {
        auto p = player.lock();
        if(p)
            p->clearGLResource(m_vo);
        else
            MediaPlayer::foreignGLContextDestroyed(m_vo);
    }

    void render() override
    {
        auto p = player.lock();
        if(p)
            p->renderVideo(m_vo);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        auto p = player.lock();
        if(p)
            p->setVideoSurfaceSize(size.width(), size.height(), m_vo);
        QOpenGLFramebufferObjectFormat format;
		format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
		format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    std::weak_ptr< MediaPlayer > player;
    void *                       m_vo;
};

void QMLPlayer::onVideoSize(int64_t width, int64_t height, void *userData)
{
    QMLPlayer *p = (QMLPlayer *)userData;
    if(p)
        emit p->videoRatioChanged((qreal)width / (qreal)height);
}

void QMLPlayer::onEOS(void *userData)
{
    QMLPlayer *p = (QMLPlayer *)userData;
    if(p)
        emit p->ended();

	//p->seektobegin();
}

void QMLPlayer::onFirstFrame(void *userData)
{
	QMLPlayer *p = (QMLPlayer *)userData;
    if(p)
        emit p->firstFrame();
}

QMLPlayer::QMLPlayer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , internal_player(std::make_shared< MediaPlayer >())
{
    playerListener pListener{ nullptr };
    pListener.userData         = this;
    pListener.VideoSizeChanged = onVideoSize;
    pListener.Completion       = onEOS;
	pListener.FirstFrameShow   = onFirstFrame;
    internal_player->SetListener(pListener);

    setMirrorVertically(true);
	internal_player->EnableHardwareDecoder(true);
    internal_player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);
    internal_player->setMaskMode(IVideoRender::Mask_Right);
	internal_player->setSmoothLoop(true);

}

QMLPlayer::~QMLPlayer()
{
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *QMLPlayer::createRenderer() const
{
    return new VideoRendererInternal(internal_player, (void *)this);
}

void QMLPlayer::setMixInfo(QString info)
{
    //internal_player->setMaskMode(IVideoRender::Mask_Right, info.toStdString());
}

void QMLPlayer::setSource(QString path)
{
    std::string p = path.toStdString();
    internal_player->SetDataSource(p.c_str());
    internal_player->SetAutoPlay(true);
    internal_player->SetLoop(false);
    internal_player->Prepare();
}

void QMLPlayer::stop()
{
    internal_player->Stop();
}

void QMLPlayer::testplay()
{
	stop();
    setMixInfo("");
    setSource("D:\\fenceng 1.mp4");
}

void QMLPlayer::seektobegin()
{
	static bool paused = false;
	if (!paused) {
	internal_player->Pause();
	paused = true;
	} else {
		paused = false;
		internal_player->Start();
	}
	
}