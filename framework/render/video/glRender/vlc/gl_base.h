#pragma once

#include "gl_common.h"
#include <cstdint>

enum {
    VLC_OPENGL,
    VLC_OPENGL_ES2,
};

class GLBase {
public:
    virtual int makeCurrent()
    {
        return 0;
    }
    virtual void releaseCurrent()
    {}
    virtual void resize(unsigned, unsigned)
    {}
    virtual void swap()
    {}
    virtual void *getProcAddress(const char *)
    {
        return nullptr;
    }

	virtual bool checkCurrent()
    {
		return true;
	}

    enum {
        VLC_GL_EXT_DEFAULT,
        VLC_GL_EXT_EGL,
        VLC_GL_EXT_WGL,
    } ext;

    union {
        /* if ext == VLC_GL_EXT_EGL */
        struct {
            /* call eglQueryString() with current display */
            const char *(*queryString)(int32_t name);
            /* call eglCreateImageKHR() with current display and context, can
             * be NULL */
            void *(*createImageKHR)(unsigned target, void *buffer, const int32_t *attrib_list);
            /* call eglDestroyImageKHR() with current display, can be NULL */
            bool (*destroyImageKHR)(void *image);
        } egl;
        /* if ext == VLC_GL_EXT_WGL */
        struct {
            const char *(*getExtensionsString)(GLBase *);
        } wgl;
    };
};