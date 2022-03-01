cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 11)
project(render)

include_directories(video/glRender/platform/glloader/include)
include_directories(video/glRender/platform/glloader/include/angle/include)
list(APPEND SRC_FILES
        video/vsync/timedVSync.cpp
        video/vsync/timedVSync.h
        video/vsync/VSyncFactory.cpp
        video/vsync/VSyncFactory.h
        video/AFActiveVideoRender.cpp
        video/AFActiveVideoRender.h
		video/IVideoRender.h
		video/IVideoRender.cpp
        renderFactory.cpp
        renderFactory.h
        audio/filterAudioRender.cpp
        audio/filterAudioRender.h
        audio/audioRenderPrototype.cpp
        audio/audioRenderPrototype.h
		video/glRender/platform/glloader/src/glloader_egl.c
		video/glRender/platform/glloader/src/glloader_gl.c
		video/glRender/platform/glloader/src/glloader_gles.c
		video/glRender/platform/glloader/src/glloader_glx.c
		video/glRender/platform/glloader/src/glloader_wgl.c
		video/glRender/platform/glloader/include/glloader/glloader.h
		video/glRender/platform/glloader/include/glloader/glloader_egl.h
		video/glRender/platform/glloader/include/glloader/glloader_gl.h
		video/glRender/platform/glloader/include/glloader/glloader_gles.h
		video/glRender/platform/glloader/include/glloader/glloader_glx.h
		video/glRender/platform/glloader/include/glloader/glloader_wgl.h
		video/glRender/platform/glloader/src/utils.cpp
		video/glRender/platform/glloader/src/utils.h)

#if (APPLE)
#    list(APPEND SRC_FILES
#            video/vsync/CADisplayLinkVSync.mm
#            video/vsync/CADisplayLinkVSync.h
#            video/vsync/CAApplicationCaller.mm
#            video/vsync/CAApplicationCaller.h)
#endif ()

if (ANDROID)
    list(APPEND SRC_FILES
            video/vsync/AndroidVSync.cpp
            video/vsync/AndroidVSync.h
            )
endif ()

if (ENABLE_SDL)
    list(APPEND SRC_FILES
            audio/SdlAFAudioRender.h
            audio/SdlAFAudioRender.cpp
            audio/SdlAFAudioRender2.h
            audio/SdlAFAudioRender2.cpp
            video/SdlAFVideoRender.cpp
            video/SdlAFVideoRender.h
            )
endif ()

if (ENABLE_CHEAT_RENDER)
    list(APPEND SRC_FILES
            audio/CheaterAudioRender.cpp
            )
endif ()

if (ANDROID)
    list(APPEND SRC_FILES
            audio/filterAudioRender.cpp
            audio/filterAudioRender.h
            audio/Android/AudioTrackRender.cpp
            audio/Android/AudioTrackRender.h
            )
endif ()


if (APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fobjc-arc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc")
    list(APPEND SRC_FILES
            audio/Apple/AudioRenderType.h
            audio/Apple/AFAudioQueueRender.cpp
            audio/Apple/AFAudioQueueRender.h
            video/AVFoundation/AVFoundationVideoRender.cpp
            video/AVFoundation/AVFoundationVideoRender.h
            video/AVFoundation/SampleDisplayLayerRender.mm
            video/AVFoundation/SampleDisplayLayerRender.h
            video/AVFoundation/DisplayLayerImpl-interface.h
            )
endif ()

if (IOS)
    list(APPEND SRC_FILES
            audio/Apple/AFAudioSession.mm
            audio/Apple/AFAudioSession.h
            audio/Apple/AFAudioSessionWrapper.mm
            audio/Apple/AFAudioSessionWrapper.h
            audio/Apple/CicadaAudioSessionDelegate.h
            )
endif ()

if (WIN32)
	add_definitions(-DSTBI_WINDOWS_UTF8)
	add_definitions(-DGLLOADER_SOURCE)
    list(APPEND SRC_FILES
                video/glRender/XXQGLRender.cpp
                video/glRender/XXQGLRender.h
				video/glRender/IProgramContext.h
				video/glRender/XXQProgramContext.cpp
				video/glRender/XXQProgramContext.h
				video/glRender/VAP.h
				video/glRender/stb_image.h
        )
endif ()

if (ENABLE_GLRENDER)
    add_definitions(-DGLRENDER)

    file(GLOB FilesPlatformBase video/glRender/platform/*.*)
    # TODO
    if (ANDROID)
        file(GLOB FilesPlatform video/glRender/platform/android/*.*)
    endif ()

    if (IOS)
        file(GLOB FilesPlatform video/glRender/platform/ios/*.*)
    endif ()

    #

    if (APPLE AND NOT IOS)
        file(GLOB FilesPlatform video/glRender/platform/mac/*.*)
    endif ()
    #
    #    if (${TARGET_PLATFORM} STREQUAL "windows")
    #        file(GLOB FilesPlatform      glRender/platform/window/*.*)
    #        ADD_DEFINITIONS(-DGLEW_STATIC)
    #    endif()

    file(GLOB FilesRenderSystem video/glRender/render_system/*.*)
    file(GLOB FilesEGL video/glRender/render_system/EGL/*.*)
    file(GLOB FilesBase video/glRender/base/*.*)

    list(APPEND SRC_FILES
            video/glRender/GLRender.cpp
            video/glRender/GLRender.h
            video/glRender/IProgramContext.h
            video/glRender/YUVProgramContext.cpp
            video/glRender/YUVProgramContext.h

            ${FilesPlatformBase}
            ${FilesPlatform}

            ${FilesRenderSystem}
            ${FilesEGL}

            ${FilesBase}

            )

    if (ANDROID)
        set(SRC_FILES ${SRC_FILES}
                video/glRender/OESProgramContext.cpp
                video/glRender/OESProgramContext.h
                video/glRender/platform/android/decoder_surface.cpp
                video/glRender/platform/android/decoder_surface.h
                )
    endif ()

    if (IOS)
        set(SRC_FILES ${SRC_FILES}
                video/glRender/CV420PProgramContext.cpp
                video/glRender/CV420PProgramContext.h)
    endif ()

endif ()

add_library(render STATIC ${SRC_FILES})

target_include_directories(render PUBLIC ${COMMON_INC_DIR})
if (ENABLE_SDL)
    target_compile_definitions(render PUBLIC ENABLE_SDL)
if (NOT MSVC)
    target_include_directories(render PUBLIC /usr/local/include/)
    target_link_directories(render PRIVATE ${COMMON_LIB_DIR})
    target_link_directories(render PUBLIC /usr/local/lib)
    target_link_libraries(render PUBLIC SDL2)
endif()
endif ()

if (ENABLE_CHEAT_RENDER)
    target_compile_definitions(render PUBLIC ENABLE_CHEAT_RENDER)
endif ()

if (ENABLE_GLRENDER)

    target_include_directories(render PUBLIC
            video/glRender/
            )

endif ()


if (HAVE_COVERAGE_CONFIG)
    target_link_libraries(render PUBLIC coverage_config)
endif ()
