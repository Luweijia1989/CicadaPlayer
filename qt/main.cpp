#include "MediaPlayer.h"
#include "glwidget.h"
#include <QApplication>
#include <QPushButton>
#include <QTimer>

using namespace Cicada;
using namespace std;

class A {
public:
    A(int a) : m(a)
    {}

    int m;
};

//OpenGLWidget *gl = nullptr;
//int main(int argc, char *argv[])
//{
//	std::shared_ptr<A> aaa = std::make_shared<A>(2);
//	std::weak_ptr<A> aa = aaa;
//	auto ff = aa.lock();
//	aaa = std::make_shared<A>(3);
//
//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
//    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
//    QApplication a(argc, argv);
// //   QTimer t;
// //   QObject::connect(&t, &QTimer::timeout, []() {
// //       if (gl) {
//	//		gl->deleteLater();
//	//		//Sleep(1000);
// //       }
//
// //       gl = new OpenGLWidget;
// //       gl->resize(640, 480);
// //       gl->show();
// //   });
//	//t.start(200);
//	//t.setSingleShot(true);
//
//	OpenGLWidget gl;
//	gl.resize(640, 480);
//	gl.show();
//
//    return a.exec();
//}


int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication a(argc, argv);

    auto player = std::shared_ptr<MediaPlayer>(new MediaPlayer());

    player->setMaskMode(
            IVideoRender::Mask_Right,
            u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");
    player->SetRotateMode(ROTATE_MODE_0);
    player->SetScaleMode(SM_FIT);

    //player->SetSpeed(0.5);
    player->SetDefaultBandWidth(1000 * 1000);
    //player->SetDataSource("http://player.alicdn.com/video/aliyunmedia.mp4");
    //player->SetDataSource("E:\\vap1.mp4");
    player->SetDataSource("C:\\Users\\posat\\Desktop\\vap1.mp4");
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

    QPushButton btn("new one");
    QObject::connect(&btn, &QPushButton::clicked, [=]() {
        OpenGLWidget *w = new OpenGLWidget(player);
        w->resize(640, 480);
        w->show();
    });
    btn.show();

    //QTimer t;
    //QObject::connect(&t, &QTimer::timeout, [=]() {
    //    {
    //        OpenGLWidget *w = new OpenGLWidget(player);
    //        w->resize(640, 480);
    //        w->show();
    //    }
    //    {
    //        OpenGLWidget *w = new OpenGLWidget(player);
    //        w->resize(640, 480);
    //        w->show();
    //    }
    //    {
    //        OpenGLWidget *w = new OpenGLWidget(player);
    //        w->resize(640, 480);
    //        w->show();
    //    }
    //    {
    //        OpenGLWidget *w = new OpenGLWidget(player);
    //        w->resize(640, 480);
    //        w->show();
    //    }
    //    {
    //        OpenGLWidget *w = new OpenGLWidget(player);
    //        w->resize(640, 480);
    //        w->show();
    //    }
    //});
    //t.start(300);

    //QTimer::singleShot(5000, [=]() {
    //	qApp->quit();
    //});

    return a.exec();
}
