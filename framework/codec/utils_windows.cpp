#include "utils_windows.h"

namespace Cicada {
    typedef const char *(WINAPI *PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
    typedef const char *(WINAPI *PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);

    typedef unsigned int GLenum;
    typedef unsigned int GLuint;
    typedef int GLint;
    typedef BOOL(WINAPI *PFNWGLDXCLOSEDEVICENVPROC)(HANDLE hDevice);
    typedef BOOL(WINAPI *PFNWGLDXLOCKOBJECTSNVPROC)(HANDLE hDevice, GLint count, HANDLE *hObjects);
    typedef HANDLE(WINAPI *PFNWGLDXOPENDEVICENVPROC)(void *dxDevice);
    typedef HANDLE(WINAPI *PFNWGLDXREGISTEROBJECTNVPROC)(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
    typedef BOOL(WINAPI *PFNWGLDXSETRESOURCESHAREHANDLENVPROC)(void *dxObject, HANDLE shareHandle);
    typedef BOOL(WINAPI *PFNWGLDXUNLOCKOBJECTSNVPROC)(HANDLE hDevice, GLint count, HANDLE *hObjects);
    typedef BOOL(WINAPI *PFNWGLDXUNREGISTEROBJECTNVPROC)(HANDLE hDevice, HANDLE hObject);

    static const char *dummy_window_class = "GLDummyWindow";
    static bool registered_dummy_window_class = false;

    struct dummy_context {
        HWND hwnd;
        HGLRC hrc;
        HDC hdc;
    };

    static inline bool HasExtension(const char *apis, const char *api)
    {
        size_t apilen = strlen(api);
        while (apis) {
            while (*apis == ' ') apis++;
            if (!strncmp(apis, api, apilen) && memchr(" ", apis[apilen], 2)) return true;
            apis = strchr(apis, ' ');
        }
        return false;
    }

    static bool gl_register_dummy_window_class(void)
    {
        WNDCLASSA wc;
        if (registered_dummy_window_class) return true;

        memset(&wc, 0, sizeof(wc));
        wc.style = CS_OWNDC;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = dummy_window_class;

        if (!RegisterClassA(&wc)) {
            return false;
        }

        registered_dummy_window_class = true;
        return true;
    }

    static inline HWND gl_create_dummy_window(void)
    {
        HWND hwnd =
                CreateWindowExA(0, dummy_window_class, "Dummy GL Window", WS_POPUP, 0, 0, 2, 2, NULL, NULL, GetModuleHandle(NULL), NULL);

        return hwnd;
    }

    static inline void gl_dummy_context_free(struct dummy_context *dummy)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy->hrc);
        DestroyWindow(dummy->hwnd);
        memset(dummy, 0, sizeof(struct dummy_context));
    }

    static inline HGLRC gl_init_basic_context(HDC hdc)
    {
        HGLRC hglrc = wglCreateContext(hdc);
        if (!hglrc) {
            return NULL;
        }

        if (!wglMakeCurrent(hdc, hglrc)) {
            wglDeleteContext(hglrc);
            return NULL;
        }

        return hglrc;
    }

    /* would use designated initializers but Microsoft sort of sucks */
    static inline void init_dummy_pixel_format(PIXELFORMATDESCRIPTOR *pfd)
    {
        memset(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        pfd->nSize = sizeof(pfd);
        pfd->nVersion = 1;
        pfd->iPixelType = PFD_TYPE_RGBA;
        pfd->cColorBits = 32;
        pfd->cDepthBits = 24;
        pfd->cStencilBits = 8;
        pfd->iLayerType = PFD_MAIN_PLANE;
        pfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    }

    static bool gl_dummy_context_init(struct dummy_context *dummy)
    {
        PIXELFORMATDESCRIPTOR pfd;
        int format_index;

        if (!gl_register_dummy_window_class()) return false;

        dummy->hwnd = gl_create_dummy_window();
        if (!dummy->hwnd) return false;

        dummy->hdc = GetDC(dummy->hwnd);

        init_dummy_pixel_format(&pfd);
        format_index = ChoosePixelFormat(dummy->hdc, &pfd);
        if (!format_index) {
            return false;
        }

        if (!SetPixelFormat(dummy->hdc, format_index, &pfd)) {
            return false;
        }

        dummy->hrc = gl_init_basic_context(dummy->hdc);
        if (!dummy->hrc) {
            return false;
        }

        return true;
    }

    bool checkDxInteropAvailable()
    {
        bool ret = false;
        struct dummy_context dummy;
        memset(&dummy, 0, sizeof(struct dummy_context));

        if (gl_dummy_context_init(&dummy)) {
            PFNWGLGETEXTENSIONSSTRINGEXTPROC ext = NULL;
            PFNWGLGETEXTENSIONSSTRINGARBPROC arb = NULL;
            ext = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");
            if (!ext) {
                arb = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
            }

			if (ext || arb) {
				const char *ext_string = ext ? ext() : arb(dummy.hdc);
				if (ext_string) {
					if (HasExtension(ext_string, "WGL_NV_DX_interop")) {
						ret = wglGetProcAddress("wglDXSetResourceShareHandleNV") && wglGetProcAddress("wglDXOpenDeviceNV") &&
							  wglGetProcAddress("wglDXCloseDeviceNV") && wglGetProcAddress("wglDXRegisterObjectNV") &&
							  wglGetProcAddress("wglDXUnregisterObjectNV") && wglGetProcAddress("wglDXLockObjectsNV") &&
							  wglGetProcAddress("wglDXUnlockObjectsNV");
					}
				}
			}
        }

        gl_dummy_context_free(&dummy);
	
        return ret;
    }
}// namespace Cicada