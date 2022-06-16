#pragma once

#include "gl_base.h"

class WGL : public GLBase {
public:
    struct {
        PFNWGLGETEXTENSIONSSTRINGEXTPROC GetExtensionsStringEXT;
        PFNWGLGETEXTENSIONSSTRINGARBPROC GetExtensionsStringARB;
    } exts;

    static const char *GetExtensionsString(GLBase *base)
    {
        auto wgl = dynamic_cast<WGL *>(base);
        return wgl->exts.GetExtensionsStringEXT 
			? wgl->exts.GetExtensionsStringEXT() 
			: (wgl->exts.GetExtensionsStringARB ? wgl->exts.GetExtensionsStringARB(wgl->hGLDC) : NULL);
    }

    WGL()
    {
        hGLDC = wglGetCurrentDC();
        hGLRC = wglGetCurrentContext();

#ifdef WGL_EXT_swap_control
        const char *extensions = (const char *) glGetString(GL_EXTENSIONS);
        if (HasExtension(extensions, "WGL_EXT_swap_control")) {
            PFNWGLSWAPINTERVALEXTPROC SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
            if (SwapIntervalEXT) SwapIntervalEXT(1);
        }
#endif

#define LOAD_EXT(name, type) exts.name = (type) wglGetProcAddress("wgl" #name)

        LOAD_EXT(GetExtensionsStringEXT, PFNWGLGETEXTENSIONSSTRINGEXTPROC);
        if (!exts.GetExtensionsStringEXT) LOAD_EXT(GetExtensionsStringARB, PFNWGLGETEXTENSIONSSTRINGARBPROC);

        ext = GLBase::VLC_GL_EXT_WGL;
        wgl.getExtensionsString = GetExtensionsString;
    }
    ~WGL()
    {}

    void *getProcAddress(const char *name) override
    {
        return wglGetProcAddress(name);
    }

    bool checkCurrent() override
    {
        auto cur = wglGetCurrentContext();
        if (!cur) return wglMakeCurrent(hGLDC, hGLRC);

        return true;
    }

private:
    HDC hGLDC;
    HGLRC hGLRC;
};