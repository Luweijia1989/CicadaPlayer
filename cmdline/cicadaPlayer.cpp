#include <MediaPlayer.h>
#include <memory>
#include <utils/timer.h>

#ifdef ENABLE_SDL
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#endif
#include <utils/CicadaJSON.h>
using namespace Cicada;
using namespace std;

#include "NetWorkEventReceiver.h"
#include "SDLEventReceiver.h"
#include "cicadaEventListener.h"
#include <utils/frame_work_log.h>

#include <utils/property.h>

#include <media_player_error_def.h>

using IEvent = IEventReceiver::IEvent;
struct cicadaCont {
    MediaPlayer *player;
    IEventReceiver *receiver;
    bool error;
};

static void onVideoSize(int64_t width, int64_t height, void *userData)
{
    AF_TRACE;
    using IEvent = IEventReceiver::IEvent;
    auto *cont = static_cast<cicadaCont *>(userData);

    if (cont->receiver) {
        cont->receiver->push(std::unique_ptr<IEvent>(new IEvent(IEvent::TYPE_SET_VIEW)));
    }
}

static void onAudioRendered(int64_t time, int64_t pts, void *userData)
{
    //    AF_LOGD("audio pts %lld rendered\n",pts);
}

static void onEOS(void *userData)
{
    auto *cont = static_cast<cicadaCont *>(userData);

    if (cont->receiver) {
        cont->receiver->push(std::unique_ptr<IEvent>(new IEvent(IEvent::TYPE_EXIT)));
    }
}
static void onPrepared(void *userData)
{
    auto *cont = static_cast<cicadaCont *>(userData);
    //   af_msleep(10000);
    cont->player->Start();
}

static void currentDownLoadSpeed(int64_t speed, void *userData)
{
    AF_LOGD("current speed is %f kbps\n", (float) speed / 1024);
}

static void onEvent(int64_t errorCode, const void *errorMsg, void *userData)
{
    auto *cont = static_cast<cicadaCont *>(userData);

    if (errorMsg == nullptr) {
        return;
    }

    AF_LOGD("%s\n", errorMsg);

    switch (errorCode) {
        case MediaPlayerEventType::MEDIA_PLAYER_EVENT_NETWORK_RETRY:
            cont->player->Reload();
            break;

        case MediaPlayerEventType::MEDIA_PLAYER_EVENT_DIRECT_COMPONENT_MSG: {
            AF_LOGI("get a dca message %s\n", errorMsg);
            CicadaJSONItem msg((char *) errorMsg);
            if (msg.getString("content", "") == "hello") {
                msg.deleteItem("content");
                msg.addValue("content", "hi");
                msg.addValue("cmd", 0);
                cont->player->InvokeComponent(msg.printJSON().c_str());
            }
            break;
        }

        default:
            break;
    }
}

static void onError(int64_t errorCode, const void *errorMsg, void *userData)
{
    auto *cont = static_cast<cicadaCont *>(userData);

    if (errorMsg) {
        AF_LOGE("%s\n", errorMsg);
    }

    if (cont->receiver) {
        auto *event = new IEvent(IEvent::TYPE_EXIT);
        cont->receiver->push(std::unique_ptr<IEvent>(event));
    } else {
        cont->error = true;
    }
}

static bool CicadaOnRenderFrame(void *userData, IAFFrame *frame)
{
    if (frame == nullptr) return false;
    if (frame->getType() != IAFFrame::FrameTypeVideo) {
        return false;
    }
    //    AF_LOGD("render a video frame %lld\n", frame->getInfo().pts);
    return false;
}
static void onSeekEnd(int64_t position, void *userData)
{
    //  AF_LOGD("seek end\n");
}

static void changeAudioFormat()
{
    setProperty("protected.audio.render.change_format", "ON");
    setProperty("protected.audio.render.change_format.fmt", "s16");
    setProperty("protected.audio.render.change_format.channels", "2");
    setProperty("protected.audio.render.change_format.sample_rate", "44100");
}

int main(int argc, char *argv[])
{
    string url;
    setProperty("protected.network.http.http2", "ON");

    if (argc > 1) {
        url = argv[1];
    } else {
        url = "https://player.alicdn.com/video/aliyunmedia.mp4";
    }

    log_enable_color(1);
    log_set_level(AF_LOG_LEVEL_TRACE, 1);
    setProperty("protected.audio.render.hw.tempo", "OFF");
    //
    //    changeAudioFormat();

    cicadaCont cicada{};
    unique_ptr<MediaPlayer> player = unique_ptr<MediaPlayer>(new MediaPlayer());
    cicada.player = player.get();
    playerListener pListener{nullptr};
    pListener.userData = &cicada;
    pListener.VideoSizeChanged = onVideoSize;
    pListener.AudioRendered = onAudioRendered;
    pListener.Completion = onEOS;
    pListener.EventCallback = onEvent;
    pListener.ErrorCallback = onError;
    pListener.Prepared = onPrepared;
    pListener.CurrentDownLoadSpeed = currentDownLoadSpeed;
    pListener.SeekEnd = onSeekEnd;
    cicadaEventListener eListener(player.get());
#ifdef ENABLE_SDL
    SDLEventReceiver receiver(eListener);
    cicada.receiver = &receiver;
#else
    int view = 0;
    player->SetView(&view);
#endif
    NetWorkEventReceiver netWorkEventReceiver(eListener);
    player->SetListener(pListener);
    player->SetDefaultBandWidth(1000 * 1000);
    player->SetDataSource(url.c_str());
    player->SetAutoPlay(true);
    player->SetLoop(true);
    player->SetIPResolveType(IpResolveWhatEver);
    player->SetFastStart(true);
    MediaPlayerConfig config = *(player->GetConfig());
    //   config.mMaxBackwardBufferDuration = 20000;
    config.liveStartIndex = -3;
    player->SetConfig(&config);
    player->Prepare();
    player->SelectTrack(-1);
    player->SetOnRenderFrameCallback(CicadaOnRenderFrame, nullptr);
    //    player->SetStreamDelayTime(-1, -200);
    bool quite = false;

    while (!quite && !cicada.error) {
#ifdef ENABLE_SDL
        receiver.poll(quite);
#endif

        if (!quite) {
            netWorkEventReceiver.poll(quite);

            if (quite) {
                auto *event = new IEvent(IEvent::TYPE_EXIT);
#ifdef ENABLE_SDL
                receiver.push(std::unique_ptr<IEvent>(event));
#endif
            }
        }

        if (!quite) {
            af_msleep(10);
        }
    }

    return 0;
}