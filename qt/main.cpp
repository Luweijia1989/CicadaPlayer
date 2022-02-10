#include <MediaPlayer.h>
#include <QApplication>
#include "glwidget.h"

using namespace Cicada;
using namespace std;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

	OpenGLWidget gl;
	gl.resize(640, 480);
	gl.show();

	return a.exec();

    /*unique_ptr<MediaPlayer> player = unique_ptr<MediaPlayer>(new MediaPlayer());

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

	getchar();
	return 0;*/
}
