#include "DiyQmlRender.h"
#include <QOpenGLFramebufferObjectFormat>
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qtimer.h>

class DiyVideoRendererInternal : public QQuickFramebufferObject::Renderer {
public:
    DiyVideoRendererInternal(const std::shared_ptr<DiyEffectPlayer> &p, void *vo, QString videoTag)
        : player(p), m_vo(vo), m_videoTag(videoTag)
    {
        qInfo() << __FUNCTION__;
    }

    ~DiyVideoRendererInternal()
    {
        auto p = player.lock();
        if (p)
            p->clearGLResource(m_vo);
        else
            DiyEffectPlayer::foreignGLContextDestroyed(m_vo, m_videoTag.toStdString());
    }

    void render() override
    {
        auto p = player.lock();
        if (p) {
            p->renderVideo(m_vo, fbo_id);
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        qInfo() << __FUNCTION__;
        auto p = player.lock();
        if (p) p->setSurfaceSize(m_vo, size.width(), size.height());
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        auto fbo = new QOpenGLFramebufferObject(size, format);
        fbo_id = fbo->handle();
        return fbo;
    }

    uint fbo_id = -1;
    std::weak_ptr<DiyEffectPlayer> player;
    void *m_vo;
    QString m_videoTag;
};

DiyQMLPlayer::DiyQMLPlayer(QQuickItem *parent) :
QQuickFramebufferObject(parent), internal_player(std::make_shared<DiyEffectPlayer>())
{
    setMirrorVertically(true);

    connect(this, &DiyQMLPlayer::sourceTagChanged, this, [=]() {
        qInfo() << "sourceTagChanged" << m_sourceTag;
        if (internal_player) {
            internal_player->setRootTag(m_sourceTag.toStdString());

            playerListener pListener{nullptr};
            pListener.userData = this;
            pListener.VideoSizeChanged = onVideoSize;
            pListener.Completion = onEOS;
            pListener.FirstFrameShow = onFirstFrame;
            internal_player->SetListener(pListener);

			internal_player->setSmoothLoop(false);
            internal_player->EnableHardwareDecoder(false);
            internal_player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);

        }
    });

    connect(this, &DiyQMLPlayer::sourceUrlChanged, this, [=]() {
        qInfo() << "sourceUrlChanged" << m_sourceUrl;
        return;
        QTimer::singleShot(100, this, [=]() {
            qInfo() << "start render special videofile" << m_sourceUrl;
            if (internal_player && m_sourceUrl.length()) {
                internal_player->stop();
                internal_player->start(m_sourceUrl.toStdString());
            }
        });
    });

	connect(this, &DiyQMLPlayer::sourceListChanged, this, [=]() { 
		return;
		QTimer::singleShot(300,this,[=]{

            qInfo() << "sourceListChanged" << m_sourceList;
            std::list<std::string> urlArray;
            for (const auto &item : m_sourceList) {
                urlArray.push_back(item.toString().toStdString());
            }
            if (internal_player) {
                internal_player->start(urlArray);
            }
		});
	});
}

DiyQMLPlayer::~DiyQMLPlayer()
{
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *DiyQMLPlayer::createRenderer() const
{
    return new DiyVideoRendererInternal(internal_player, (void *) this, m_sourceTag);
}

void DiyQMLPlayer::play1()
{
    internal_player->stop();
    internal_player->setMaskMode(
            IVideoRender::Mask_Right,
            "{\"[textAnchor]\":\"aaaa\",\"[textUser]\":\"bbbb\",\"[imgAnchor]\":\"D:/test.jpg\",\"[imgUser]\":\"D:/ccc.jpg\"}");
    internal_player->start(u8R"(C:\Users\luweijia\AppData\Local\yuerlive\cache\image\stage\stage_level_3.mp4)");
}

void DiyQMLPlayer::play2()
{
    internal_player->stop();
    internal_player->start("F:\\workspace\\avsolution\\screen-recorder\\x64\\Debug_DLL\\hlqk\\output\\crf_compose.mp4");
}

void DiyQMLPlayer::play3(QString path)
{
    qInfo() << __FUNCTION__ << path;
    internal_player->stop();
    internal_player->start(path.toStdString());
}

void DiyQMLPlayer::play4(QVariantList urlList)
{
    qInfo() << __FUNCTION__ << urlList;

	std::list<std::string> urlArray;
    for (const auto &item : urlList) {
        urlArray.push_back(item.toString().toStdString());
    }
    if (internal_player) {
        internal_player->start(urlArray);
    }
}

QString DiyQMLPlayer::sourceTag()
{
    return m_sourceTag;
}

void DiyQMLPlayer::setSourceTag(const QString &tag)
{
    if (m_sourceTag != tag) {
        m_sourceTag = tag;
        sourceTagChanged();
    }
}

QString DiyQMLPlayer::sourceUrl()
{
    return m_sourceUrl;
}
void DiyQMLPlayer::setSourceUrl(const QString &url)
{
    qInfo() << __FUNCTION__ << url;
    if (m_sourceUrl != url) {
        m_sourceUrl = url;
        sourceUrlChanged();
    }
}

QVariantList DiyQMLPlayer::sourceList()
{
    return m_sourceList;
}
void DiyQMLPlayer::setSourceList(const QVariantList &urlList)
{
    if (m_sourceList != urlList) {
        m_sourceList = urlList;
        emit sourceListChanged();
	}
}

void DiyQMLPlayer::onVideoSize(int64_t width, int64_t height, void *userData)
{
    qInfo() << __FUNCTION__ << width << height << userData;
}
void DiyQMLPlayer::onEOS(void *userData)
{
    qInfo() << __FUNCTION__ << userData;
    if (userData) {
        DiyQMLPlayer *p = (DiyQMLPlayer *) userData;
        emit p->ended();
    }
}
void DiyQMLPlayer::onFirstFrame(void *userData)
{
    qInfo() << __FUNCTION__ << userData;
}