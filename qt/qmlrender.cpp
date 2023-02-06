#include "qmlrender.h"
#include <qquickwindow.h>

class VideoRendererInternal : public QQuickFramebufferObject::Renderer {
public:
    VideoRendererInternal(const std::shared_ptr<MediaPlayer> &p, void *vo) : player(p), m_vo(vo)
    {}

    ~VideoRendererInternal()
    {
        auto p = player.lock();
        if (p)
            p->clearGLResource(m_vo);
        else
            MediaPlayer::foreignGLContextDestroyed(m_vo);
    }

    void render() override
    {
        auto p = player.lock();
        if (p) p->renderVideo(m_vo);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        auto p = player.lock();
        if (p) p->setVideoSurfaceSize(size.width(), size.height(), m_vo);
		QOpenGLFramebufferObjectFormat format;
		format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
		format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    std::weak_ptr<MediaPlayer> player;
	void *m_vo;
};

#include <QTimer>
#include <qdatetime.h>
FILE *ff;
static void audioFrame(void *userData, uint8_t *data, int size)
{
	if (!ff)
		ff = fopen("D:\\ccc.pcm", "wb");
	fwrite(data, 1, size, ff);
	static qint64 ss = 0;
	auto c = QDateTime::currentMSecsSinceEpoch();
	qDebug()<< "======= " << c -ss;
	ss = c;
}

QMLPlayer::QMLPlayer(QQuickItem *parent) : QQuickFramebufferObject(parent), internal_player(std::make_shared<MediaPlayer>())
{
    setMirrorVertically(true);
    internal_player->setRenderCallback([this](void *p) { 
		QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); 
	}, this);

	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [=](){
		internal_player->SetDataSource("D:/cut.mp4");
		internal_player->SetAutoPlay(true);
		internal_player->SetLoop(false);
		internal_player->Prepare();
	});

	timer->start(500);
	//internal_player->EnableHardwareDecoder(true);
 //   internal_player->SetDataSource("D:/test.mkv");
	//internal_player->SetAudioRenderingCallback(audioFrame, this);
	///*internal_player->setMaskMode(
 //           IVideoRender::Mask_Right,
 //           u8"{\"[imgUser]\":\"E:/test.jpg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");*/
 //   internal_player->SetAutoPlay(true);
 //   internal_player->SetLoop(true);
 //   internal_player->Prepare();
}

QMLPlayer::~QMLPlayer()
{
	internal_player->Stop();
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *QMLPlayer::createRenderer() const
{
    return new VideoRendererInternal(internal_player, (void *)this);
}

#include <QThread>
void QMLPlayer::test()
{
	internal_player->Stop();
	internal_player->SetDataSource("https://video.hellobixin.com/video/dc37c4361114437ab84f1ebef1bbb7e5.mp4");
	/*internal_player->setMaskMode(
            IVideoRender::Mask_Right,
            u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");*/
    internal_player->SetAutoPlay(true);
    internal_player->SetLoop(false);
    internal_player->Prepare();
}

void QMLPlayer::stop()
{
	internal_player->Stop();
}