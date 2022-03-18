#pragma once

#include "VideoAcceleration.h"
#include <Unknwn.h>
#include <atomic>

#define MAX_SURFACE_COUNT 64

struct va_surface_t {
    va_surface_t() : refcount(1)
    {}
    virtual ~va_surface_t()
    {}
    virtual void setSurface(IUnknown *s) = 0;
    virtual IUnknown *getSurface() const = 0;

    std::atomic_uintptr_t refcount;
};

class VAD3D : public VideoAcceleration {
public:
    VAD3D(AVCodecContext *ctx, AVPixelFormat fmt) : VideoAcceleration(ctx, fmt)
    {}

    virtual std::string description() = 0;

    int openDXResource();
    void closeDXResource();
    int setupDecoder(const AVCodecContext *, unsigned count, int alignment);
    int setupSurfaces(unsigned count);
    int getVASurface(va_surface_t **surface);
    void surfaceAddRef(va_surface_t *);
    void surfaceRelease(va_surface_t *);

    virtual int createDevice() = 0;
    virtual void destroyDevice() = 0;

    virtual int createDeviceManager() = 0;
    virtual void destroyDeviceManager() = 0;

    virtual int createVideoService() = 0;
    virtual void destroyVideoService() = 0;

    virtual int createDecoderSurfaces(const AVCodecContext *, unsigned surface_count) = 0;
    virtual void destroySurfaces() = 0;

    virtual void setupAvcodecCtx() = 0;

    virtual va_surface_t *newSurfaceContext(int surface_index) = 0;

private:
    void destroyDecoder();
    va_surface_t *getSurface();

public:
    unsigned surface_count = 0;
    int surface_width = 0;
    int surface_height = 0;

    va_surface_t *surface[MAX_SURFACE_COUNT] = {0};
};