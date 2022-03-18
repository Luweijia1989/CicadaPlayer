#define LOG_TAG "VAD3D"
#include "VAD3D.h"
#include <cassert>
#include <utils/frame_work_log.h>

void VAD3D::destroyDecoder()
{
    for (unsigned i = 0; i < surface_count; i++) {
        surfaceRelease(surface[i]);
        delete surface[i];
    }
    destroySurfaces();
    surface_count = 0;
}

int VAD3D::openDXResource()
{
    /* */
    if (createDevice()) {
        AF_LOGE("Failed to create device");
        goto error;
    }
    AF_LOGD("CreateDevice succeed");

    if (createDeviceManager()) {
        AF_LOGE("CreateDeviceManager failed");
        goto error;
    }

    if (createVideoService()) {
        AF_LOGE("CreateVideoService failed");
        goto error;
    }

    return 0;

error:
    return -1;
}
void VAD3D::closeDXResource()
{
    destroyDecoder();
    destroyVideoService();
    destroyDeviceManager();
    destroyDevice();
}
int VAD3D::setupDecoder(const AVCodecContext *avctx, unsigned count, int alignment)
{
    int err = -2;
    unsigned i = surface_count;

    if (avctx->coded_width <= 0 || avctx->coded_height <= 0) return -1;

    assert((alignment & (alignment - 1)) == 0); /* power of 2 */
#define ALIGN(x, y) (((x) + ((y) -1)) & ~((y) -1))
    int surface_width = ALIGN(avctx->coded_width, alignment);
    int surface_height = ALIGN(avctx->coded_height, alignment);

    if (avctx->coded_width != surface_width || avctx->coded_height != surface_height)
        AF_LOGW("surface dimensions (%dx%d) differ from avcodec dimensions (%dx%d)", surface_width, surface_height, avctx->coded_width,
                avctx->coded_height);

    if (surface_count >= count && this->surface_width == surface_width && this->surface_height == surface_height) {
        AF_LOGD("reusing surface pool");
        err = 0;
        goto done;
    }

    /* */
    destroyDecoder();

    /* */
    AF_LOGD("va_pool_SetupDecoder id %d %dx%d count: %d", avctx->codec_id, avctx->coded_width, avctx->coded_height, count);

    if (count > MAX_SURFACE_COUNT) return -1;

    err = createDecoderSurfaces(avctx, count);
    if (err == 0) {
        this->surface_width = surface_width;
        this->surface_height = surface_height;
    }

done:
    this->surface_count = i;
    if (err == 0) setupAvcodecCtx();

    return err;
}
int VAD3D::setupSurfaces(unsigned count)
{
    int err = -1;
    unsigned i = surface_count;

    for (i = 0; i < count; i++) {
        surface[i] = newSurfaceContext(i);
    }
    err = 0;

done:
    surface_count = i;
    if (err == 0) setupAvcodecCtx();

    return err;
}

va_surface_t *VAD3D::getSurface()
{
    for (unsigned i = 0; i < surface_count; i++) {
        auto s = surface[i];
        uintptr_t expected = 1;

        if (std::atomic_compare_exchange_strong(&s->refcount, &expected, 2)) {
            return s;
        }
    }
    return NULL;
}

#define CLOCK_FREQ 1000000
#define VOUT_OUTMEM_SLEEP CLOCK_FREQ / 50
int VAD3D::getVASurface(va_surface_t **surface)
{
    unsigned tries = (CLOCK_FREQ + VOUT_OUTMEM_SLEEP) / VOUT_OUTMEM_SLEEP;

    if (surface_count == 0) return -1;

    va_surface_t *ret = NULL;
    while ((ret = getSurface()) == NULL) {
        if (--tries == 0) return -1;
            /* Pool empty. Wait for some time as in src/input/decoder.c.
         * XXX: Both this and the core should use a semaphore or a CV. */
#ifdef WIN32
        Sleep(VOUT_OUTMEM_SLEEP);
#endif
    }
    *surface = ret;
    return 0;
}

void VAD3D::surfaceAddRef(va_surface_t *surface)
{
    std::atomic_fetch_add(&surface->refcount, 1);
}
void VAD3D::surfaceRelease(va_surface_t *surface)
{
    std::atomic_fetch_sub(&surface->refcount, 1);
}