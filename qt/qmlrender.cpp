#include "qmlrender.h"

class VideoRendererInternal : public QQuickFramebufferObject::Renderer {
public:
    VideoRendererInternal(const std::shared_ptr<MediaPlayer> &p) : player(p)
    {}

    ~VideoRendererInternal()
    {
        auto p = player.lock();
        if (p)
            p->clearGLResource(this);
        else
            MediaPlayer::foreignGLContextDestroyed(this);
    }

    void render() override
    {
        auto p = player.lock();
        if (p) p->renderVideo(this);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        auto p = player.lock();
        if (p) p->setVideoSurfaceSize(size.width(), size.height(), this);
        return new QOpenGLFramebufferObject(size);
    }

    std::weak_ptr<MediaPlayer> player;
};


QMLPlayer::QMLPlayer(QQuickItem *parent) : QQuickFramebufferObject(parent), internal_player(std::make_shared<MediaPlayer>())
{
    setMirrorVertically(true);
    internal_player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);

    internal_player->SetDataSource("C:\\Users\\posat\\Desktop\\7p.mp4");
    internal_player->SetAutoPlay(true);
    internal_player->SetLoop(true);
    internal_player->Prepare();
}

QMLPlayer::~QMLPlayer()
{
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *QMLPlayer::createRenderer() const
{
    return new VideoRendererInternal(internal_player);
}