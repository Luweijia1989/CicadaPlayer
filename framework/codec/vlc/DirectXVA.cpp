#include "DirectXVA.h"
#include "VAD3D.h"
#include "VADxva.h"
#include <iostream>

static const int PROF_MPEG2_MAIN[] = {FF_PROFILE_MPEG2_SIMPLE, FF_PROFILE_MPEG2_MAIN, FF_PROFILE_UNKNOWN};
static const int PROF_H264_HIGH[] = {FF_PROFILE_H264_BASELINE, FF_PROFILE_H264_CONSTRAINED_BASELINE, FF_PROFILE_H264_MAIN,
                                     FF_PROFILE_H264_HIGH, FF_PROFILE_UNKNOWN};
static const int PROF_HEVC_MAIN[] = {FF_PROFILE_HEVC_MAIN, FF_PROFILE_UNKNOWN};
static const int PROF_HEVC_MAIN10[] = {FF_PROFILE_HEVC_MAIN, FF_PROFILE_HEVC_MAIN_10, FF_PROFILE_UNKNOWN};

static const int PROF_VP9_MAIN[] = {FF_PROFILE_VP9_0, FF_PROFILE_UNKNOWN};
static const int PROF_VP9_10[] = {FF_PROFILE_VP9_2, FF_PROFILE_UNKNOWN};

#include <winapifamily.h>
#if defined(WINAPI_FAMILY)
#undef WINAPI_FAMILY
#endif
//#define WINAPI_FAMILY WINAPI_PARTITION_DESKTOP
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP

#include <initguid.h> /* must be last included to not redefine existing GUIDs */

/* dxva2api.h GUIDs: http://msdn.microsoft.com/en-us/library/windows/desktop/ms697067(v=vs100).aspx
 * assume that they are declared in dxva2api.h */
//#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define MS_GUID DEFINE_GUID

/* Codec capabilities GUID, sorted by codec */
MS_GUID(DXVA2_ModeMPEG2_MoComp, 0xe6a9f44b, 0x61b0, 0x4563, 0x9e, 0xa4, 0x63, 0xd2, 0xa3, 0xc6, 0xfe, 0x66);
MS_GUID(DXVA2_ModeMPEG2_IDCT, 0xbf22ad00, 0x03ea, 0x4690, 0x80, 0x77, 0x47, 0x33, 0x46, 0x20, 0x9b, 0x7e);
MS_GUID(DXVA2_ModeMPEG2_VLD, 0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA_ModeMPEG1_A, 0x1b81be09, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_A, 0x1b81be0A, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_B, 0x1b81be0B, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_C, 0x1b81be0C, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_D, 0x1b81be0D, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD, 0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID(DXVA2_ModeMPEG1_VLD, 0x6f3ec719, 0x3735, 0x42cc, 0x80, 0x63, 0x65, 0xcc, 0x3c, 0xb3, 0x66, 0x16);

MS_GUID(DXVA2_ModeH264_A, 0x1b81be64, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeH264_B, 0x1b81be65, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeH264_C, 0x1b81be66, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeH264_D, 0x1b81be67, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeH264_E, 0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeH264_F, 0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH264_VLD_Multiview, 0x9901CCD3, 0xca12, 0x4b7e, 0x86, 0x7a, 0xe2, 0x22, 0x3d, 0x92, 0x55, 0xc3);// MVC
DEFINE_GUID(DXVA_ModeH264_VLD_WithFMOASO_NoFGT, 0xd5f04ff9, 0x3418, 0x45d8, 0x95, 0x61, 0x32, 0xa7, 0x6a, 0xae, 0x2d, 0xdd);
DEFINE_GUID(DXVADDI_Intel_ModeH264_A, 0x604F8E64, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C, 0x604F8E66, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVA_Intel_H264_NoFGT_ClearVideo, 0x604F8E68, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVA_ModeH264_VLD_NoFGT_Flash, 0x4245F676, 0x2BBC, 0x4166, 0xa0, 0xBB, 0x54, 0xE7, 0xB8, 0x49, 0xC3, 0x80);

MS_GUID(DXVA2_ModeWMV8_A, 0x1b81be80, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeWMV8_B, 0x1b81be81, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

MS_GUID(DXVA2_ModeWMV9_A, 0x1b81be90, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeWMV9_B, 0x1b81be91, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeWMV9_C, 0x1b81be94, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

MS_GUID(DXVA2_ModeVC1_A, 0x1b81beA0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeVC1_B, 0x1b81beA1, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeVC1_C, 0x1b81beA2, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
MS_GUID(DXVA2_ModeVC1_D, 0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010, 0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);// August 2010 update
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo, 0xBCC5DB6D, 0xA2B6, 0x4AF0, 0xAC, 0xE4, 0xAD, 0xB1, 0xF7, 0x87, 0xBC, 0x89);
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo_2, 0xE07EC519, 0xE651, 0x4CD6, 0xAC, 0x84, 0x13, 0x70, 0xCC, 0xEE, 0xC8, 0x51);

DEFINE_GUID(DXVA_nVidia_MPEG4_ASP, 0x9947EC6F, 0x689B, 0x11DC, 0xA3, 0x20, 0x00, 0x19, 0xDB, 0xBC, 0x41, 0x84);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_Simple, 0xefd64d74, 0xc9e8, 0x41d7, 0xa5, 0xe9, 0xe9, 0xb0, 0xe3, 0x9f, 0xa3, 0x19);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC, 0xed418a9f, 0x010d, 0x4eda, 0x9a, 0xe3, 0x9a, 0x65, 0x35, 0x8d, 0x8d, 0x2e);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC, 0xab998b5b, 0x4258, 0x44a9, 0x9f, 0xeb, 0x94, 0xe5, 0x97, 0xa6, 0xba, 0xae);
DEFINE_GUID(DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo, 0x7C74ADC6, 0xe2ba, 0x4ade, 0x86, 0xde, 0x30, 0xbe, 0xab, 0xb4, 0x0c, 0xc1);

DEFINE_GUID(DXVA_ModeHEVC_VLD_Main, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
DEFINE_GUID(DXVA_ModeHEVC_VLD_Main10, 0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13);

DEFINE_GUID(DXVA_ModeH264_VLD_Stereo_Progressive_NoFGT, 0xd79be8da, 0x0cf1, 0x4c81, 0xb8, 0x2a, 0x69, 0xa4, 0xe2, 0x36, 0xf4, 0x3d);
DEFINE_GUID(DXVA_ModeH264_VLD_Stereo_NoFGT, 0xf9aaccbb, 0xc2b6, 0x4cfc, 0x87, 0x79, 0x57, 0x07, 0xb1, 0x76, 0x05, 0x52);
DEFINE_GUID(DXVA_ModeH264_VLD_Multiview_NoFGT, 0x705b9d82, 0x76cf, 0x49d6, 0xb7, 0xe6, 0xac, 0x88, 0x72, 0xdb, 0x01, 0x3c);

DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Scalable_Baseline, 0xc30700c4, 0xe384, 0x43e0, 0xb9, 0x82, 0x2d, 0x89, 0xee, 0x7f, 0x77, 0xc4);
DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Restricted_Scalable_Baseline, 0x9b8175d4, 0xd670, 0x4cf2, 0xa9, 0xf0, 0xfa, 0x56, 0xdf, 0x71, 0xa1, 0xae);
DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Scalable_High, 0x728012c9, 0x66a8, 0x422f, 0x97, 0xe9, 0xb5, 0xe3, 0x9b, 0x51, 0xc0, 0x53);
DEFINE_GUID(DXVA_ModeH264_VLD_SVC_Restricted_Scalable_High_Progressive, 0x8efa5926, 0xbd9e, 0x4b04, 0x8b, 0x72, 0x8f, 0x97, 0x7d, 0xc4,
            0x4c, 0x36);

DEFINE_GUID(DXVA_ModeH261_A, 0x1b81be01, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH261_B, 0x1b81be02, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

DEFINE_GUID(DXVA_ModeH263_A, 0x1b81be03, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_B, 0x1b81be04, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_C, 0x1b81be05, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_D, 0x1b81be06, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_E, 0x1b81be07, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA_ModeH263_F, 0x1b81be08, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

DEFINE_GUID(DXVA_ModeVP8_VLD, 0x90b899ea, 0x3a62, 0x4705, 0x88, 0xb3, 0x8d, 0xf0, 0x4b, 0x27, 0x44, 0xe7);
DEFINE_GUID(DXVA_ModeVP9_VLD_Profile0, 0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);
DEFINE_GUID(DXVA_ModeVP9_VLD_10bit_Profile2, 0xa4c749ef, 0x6ecf, 0x48aa, 0x84, 0x48, 0x50, 0xa7, 0xa1, 0x16, 0x5f, 0xf7);
DEFINE_GUID(DXVA_ModeVP9_VLD_Intel, 0x76988a52, 0xdf13, 0x419a, 0x8e, 0x64, 0xff, 0xcf, 0x4a, 0x33, 0x6c, 0xf5);

/* XXX Prefered modes must come first */
static const directx_va_mode_t DXVA_MODES[] = {
        /* MPEG-1/2 */
        {"MPEG-1 decoder, restricted profile A", &DXVA_ModeMPEG1_A, AV_CODEC_ID_NONE, NULL},
        {"MPEG-2 decoder, restricted profile A", &DXVA_ModeMPEG2_A, AV_CODEC_ID_NONE, NULL},
        {"MPEG-2 decoder, restricted profile B", &DXVA_ModeMPEG2_B, AV_CODEC_ID_NONE, NULL},
        {"MPEG-2 decoder, restricted profile C", &DXVA_ModeMPEG2_C, AV_CODEC_ID_NONE, NULL},
        {"MPEG-2 decoder, restricted profile D", &DXVA_ModeMPEG2_D, AV_CODEC_ID_NONE, NULL},

        {"MPEG-2 variable-length decoder", &DXVA2_ModeMPEG2_VLD, AV_CODEC_ID_MPEG2VIDEO, PROF_MPEG2_MAIN},
        {"MPEG-2 & MPEG-1 variable-length decoder", &DXVA2_ModeMPEG2and1_VLD, AV_CODEC_ID_MPEG2VIDEO, PROF_MPEG2_MAIN},
        {"MPEG-2 & MPEG-1 variable-length decoder", &DXVA2_ModeMPEG2and1_VLD, AV_CODEC_ID_MPEG1VIDEO, NULL},
        {"MPEG-2 motion compensation", &DXVA2_ModeMPEG2_MoComp, AV_CODEC_ID_NONE, NULL},
        {"MPEG-2 inverse discrete cosine transform", &DXVA2_ModeMPEG2_IDCT, AV_CODEC_ID_NONE, NULL},

        /* MPEG-1 http://download.microsoft.com/download/B/1/7/B172A3C8-56F2-4210-80F1-A97BEA9182ED/DXVA_MPEG1_VLD.pdf */
        {"MPEG-1 variable-length decoder, no D pictures", &DXVA2_ModeMPEG1_VLD, AV_CODEC_ID_NONE, NULL},

        /* H.264 http://www.microsoft.com/downloads/details.aspx?displaylang=en&FamilyID=3d1c290b-310b-4ea2-bf76-714063a6d7a6 */
        {"H.264 variable-length decoder, film grain technology", &DXVA2_ModeH264_F, AV_CODEC_ID_H264, PROF_H264_HIGH},
        {"H.264 variable-length decoder, no film grain technology", &DXVA2_ModeH264_E, AV_CODEC_ID_H264, PROF_H264_HIGH},
        {"H.264 variable-length decoder, no film grain technology (Intel ClearVideo)", &DXVA_Intel_H264_NoFGT_ClearVideo, AV_CODEC_ID_H264,
         PROF_H264_HIGH},
        {"H.264 variable-length decoder, no film grain technology, FMO/ASO", &DXVA_ModeH264_VLD_WithFMOASO_NoFGT, AV_CODEC_ID_H264,
         PROF_H264_HIGH},
        {"H.264 variable-length decoder, no film grain technology, Flash", &DXVA_ModeH264_VLD_NoFGT_Flash, AV_CODEC_ID_H264,
         PROF_H264_HIGH},

        {"H.264 inverse discrete cosine transform, film grain technology", &DXVA2_ModeH264_D, AV_CODEC_ID_NONE, NULL},
        {"H.264 inverse discrete cosine transform, no film grain technology", &DXVA2_ModeH264_C, AV_CODEC_ID_NONE, NULL},
        {"H.264 inverse discrete cosine transform, no film grain technology (Intel)", &DXVADDI_Intel_ModeH264_C, AV_CODEC_ID_NONE, NULL},

        {"H.264 motion compensation, film grain technology", &DXVA2_ModeH264_B, AV_CODEC_ID_NONE, NULL},
        {"H.264 motion compensation, no film grain technology", &DXVA2_ModeH264_A, AV_CODEC_ID_NONE, NULL},
        {"H.264 motion compensation, no film grain technology (Intel)", &DXVADDI_Intel_ModeH264_A, AV_CODEC_ID_NONE, NULL},

        /* http://download.microsoft.com/download/2/D/AV_CODEC_ID_NONE/2D02E72E-7890-430F-BA91-4A363F72F8C8/DXVA_H264_MVC.pdf */
        {"H.264 stereo high profile, mbs flag set", &DXVA_ModeH264_VLD_Stereo_Progressive_NoFGT, AV_CODEC_ID_NONE, NULL},
        {"H.264 stereo high profile", &DXVA_ModeH264_VLD_Stereo_NoFGT, AV_CODEC_ID_NONE, NULL},
        {"H.264 multiview high profile", &DXVA_ModeH264_VLD_Multiview_NoFGT, AV_CODEC_ID_NONE, NULL},

        /* SVC http://download.microsoft.com/download/C/8/A/C8AD9F1B-57D1-4C10-85A0-09E3EAC50322/DXVA_SVC_2012_06.pdf */
        {"H.264 scalable video coding, Scalable Baseline Profile", &DXVA_ModeH264_VLD_SVC_Scalable_Baseline, AV_CODEC_ID_NONE, NULL},
        {"H.264 scalable video coding, Scalable Constrained Baseline Profile", &DXVA_ModeH264_VLD_SVC_Restricted_Scalable_Baseline,
         AV_CODEC_ID_NONE, NULL},
        {"H.264 scalable video coding, Scalable High Profile", &DXVA_ModeH264_VLD_SVC_Scalable_High, AV_CODEC_ID_NONE, NULL},
        {"H.264 scalable video coding, Scalable Constrained High Profile", &DXVA_ModeH264_VLD_SVC_Restricted_Scalable_High_Progressive,
         AV_CODEC_ID_NONE, NULL},

        /* WMV */
        {"Windows Media Video 8 motion compensation", &DXVA2_ModeWMV8_B, AV_CODEC_ID_NONE, NULL},
        {"Windows Media Video 8 post processing", &DXVA2_ModeWMV8_A, AV_CODEC_ID_NONE, NULL},

        {"Windows Media Video 9 IDCT", &DXVA2_ModeWMV9_C, AV_CODEC_ID_NONE, NULL},
        {"Windows Media Video 9 motion compensation", &DXVA2_ModeWMV9_B, AV_CODEC_ID_NONE, NULL},
        {"Windows Media Video 9 post processing", &DXVA2_ModeWMV9_A, AV_CODEC_ID_NONE, NULL},

        /* VC-1 */
        {"VC-1 variable-length decoder", &DXVA2_ModeVC1_D, AV_CODEC_ID_VC1, NULL},
        {"VC-1 variable-length decoder", &DXVA2_ModeVC1_D, AV_CODEC_ID_WMV3, NULL},
        {"VC-1 variable-length decoder", &DXVA2_ModeVC1_D2010, AV_CODEC_ID_VC1, NULL},
        {"VC-1 variable-length decoder", &DXVA2_ModeVC1_D2010, AV_CODEC_ID_WMV3, NULL},
        {"VC-1 variable-length decoder 2 (Intel)", &DXVA_Intel_VC1_ClearVideo_2, AV_CODEC_ID_NONE, NULL},
        {"VC-1 variable-length decoder (Intel)", &DXVA_Intel_VC1_ClearVideo, AV_CODEC_ID_NONE, NULL},

        {"VC-1 inverse discrete cosine transform", &DXVA2_ModeVC1_C, AV_CODEC_ID_NONE, NULL},
        {"VC-1 motion compensation", &DXVA2_ModeVC1_B, AV_CODEC_ID_NONE, NULL},
        {"VC-1 post processing", &DXVA2_ModeVC1_A, AV_CODEC_ID_NONE, NULL},

        /* Xvid/Divx: TODO */
        {"MPEG-4 Part 2 nVidia bitstream decoder", &DXVA_nVidia_MPEG4_ASP, AV_CODEC_ID_NONE, NULL},
        {"MPEG-4 Part 2 variable-length decoder, Simple Profile", &DXVA_ModeMPEG4pt2_VLD_Simple, AV_CODEC_ID_NONE, NULL},
        {"MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, no GMC", &DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC, AV_CODEC_ID_NONE,
         NULL},
        {"MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, GMC", &DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC, AV_CODEC_ID_NONE,
         NULL},
        {"MPEG-4 Part 2 variable-length decoder, Simple&Advanced Profile, Avivo", &DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo, AV_CODEC_ID_NONE,
         NULL},

        /* HEVC */
        {"HEVC Main profile", &DXVA_ModeHEVC_VLD_Main, AV_CODEC_ID_HEVC, PROF_HEVC_MAIN},
        {"HEVC Main 10 profile", &DXVA_ModeHEVC_VLD_Main10, AV_CODEC_ID_HEVC, PROF_HEVC_MAIN10},

        /* H.261 */
        {"H.261 decoder, restricted profile A", &DXVA_ModeH261_A, AV_CODEC_ID_NONE, NULL},
        {"H.261 decoder, restricted profile B", &DXVA_ModeH261_B, AV_CODEC_ID_NONE, NULL},

        /* H.263 */
        {"H.263 decoder, restricted profile A", &DXVA_ModeH263_A, AV_CODEC_ID_NONE, NULL},
        {"H.263 decoder, restricted profile B", &DXVA_ModeH263_B, AV_CODEC_ID_NONE, NULL},
        {"H.263 decoder, restricted profile C", &DXVA_ModeH263_C, AV_CODEC_ID_NONE, NULL},
        {"H.263 decoder, restricted profile D", &DXVA_ModeH263_D, AV_CODEC_ID_NONE, NULL},
        {"H.263 decoder, restricted profile E", &DXVA_ModeH263_E, AV_CODEC_ID_NONE, NULL},
        {"H.263 decoder, restricted profile F", &DXVA_ModeH263_F, AV_CODEC_ID_NONE, NULL},

        /* VPx */
        {"VP8", &DXVA_ModeVP8_VLD, AV_CODEC_ID_NONE, NULL},
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 17, 100) && LIBAVCODEC_VERSION_MICRO >= 100
        {"VP9 profile AV_CODEC_ID_NONE", &DXVA_ModeVP9_VLD_Profile0, AV_CODEC_ID_VP9, PROF_VP9_MAIN},
        {"VP9 profile 2", &DXVA_ModeVP9_VLD_10bit_Profile2, AV_CODEC_ID_VP9, PROF_VP9_10},
#else
        {"VP9 profile AV_CODEC_ID_NONE", &DXVA_ModeVP9_VLD_Profile0, AV_CODEC_ID_NONE, NULL},
        {"VP9 profile 2", &DXVA_ModeVP9_VLD_10bit_Profile2, AV_CODEC_ID_NONE, NULL},
#endif
        {"VP9 profile Intel", &DXVA_ModeVP9_VLD_Intel, AV_CODEC_ID_NONE, NULL},

        {NULL, NULL, AV_CODEC_ID_NONE, NULL}};


static bool h264_get_profile_level(uint8_t *p_extra, int i_extra, uint8_t *pi_profile, uint8_t *pi_level, uint8_t *pi_nal_length_size)
{
    uint8_t *p = (uint8_t *) p_extra;
    if (i_extra < 8) return false;

    /* Check the profile / level */
    if (p[0] == 1 && i_extra >= 12) {
        if (pi_nal_length_size) *pi_nal_length_size = 1 + (p[4] & 0x03);
        p += 8;
    } else if (!p[0] && !p[1]) /* FIXME: WTH is setting AnnexB data here ? */
    {
        if (!p[2] && p[3] == 1)
            p += 4;
        else if (p[2] == 1)
            p += 3;
        else
            return false;
    } else
        return false;

    if (((*p++) & 0x1f) != 7) return false;

    if (pi_profile) *pi_profile = p[0];

    if (pi_level) *pi_level = p[2];

    return true;
}

static bool hevc_get_profile_level(uint8_t *p_extra, int i_extra, uint8_t *pi_profile, uint8_t *pi_level, uint8_t *pi_nal_length_size)
{
    const uint8_t *p = (const uint8_t *) p_extra;
    if (i_extra < 23 || p[0] != 1) return false;

    /* HEVCDecoderConfigurationRecord */
    if (pi_profile) *pi_profile = p[1] & 0x1F;

    if (pi_level) *pi_level = p[12];

    if (pi_nal_length_size) *pi_nal_length_size = 1 + (p[21] & 0x03);

    return true;
}

bool DirectXVA::profileSupported(const directx_va_mode_t *mode, const AVCodecContext *avctx)
{
    bool is_supported = mode->p_profiles == NULL;
    if (!is_supported) {
        int profile = avctx->profile;
        if (mode->codec == AV_CODEC_ID_H264) {
            uint8_t h264_profile;
            if (h264_get_profile_level(avctx->extradata, avctx->extradata_size, &h264_profile, NULL, NULL)) profile = h264_profile;
        }
        if (mode->codec == AV_CODEC_ID_HEVC) {
            uint8_t hevc_profile;
            if (hevc_get_profile_level(avctx->extradata, avctx->extradata_size, &hevc_profile, NULL, NULL)) profile = hevc_profile;
        }

        if (profile <= 0)
            is_supported = true;
        else
            for (const int *p_profile = &mode->p_profiles[0]; *p_profile != FF_PROFILE_UNKNOWN; ++p_profile) {
                if (*p_profile == profile) {
                    is_supported = true;
                    break;
                }
            }
    }
    return is_supported;
}
int DirectXVA::FindVideoServiceConversion(const AVCodecContext *avctx)
{
    input_list_t p_list = {0};
    int err = getInputList(&p_list);
    if (err != VLC_SUCCESS) return err;
    if (p_list.count == 0) {
        std::clog << "No input format found for HWAccel";
        return VLC_EGENERIC;
    }

    err = VLC_EGENERIC;
    /* Retreive supported modes from the decoder service */
    for (unsigned i = 0; i < p_list.count; i++) {
        const GUID *g = &p_list.list[i];
        char *psz_decoder_name = directxVAGetDecoderName(g);
        std::clog << "- " << psz_decoder_name << " is supported";
        free(psz_decoder_name);
    }

    /* Try all supported mode by our priority */
    const directx_va_mode_t *mode = DXVA_MODES;
    for (; mode->name; ++mode) {
        if (!mode->codec || mode->codec != avctx->codec_id) continue;

        /* */
        bool is_supported = false;
        for (const GUID *g = &p_list.list[0]; !is_supported && g < &p_list.list[p_list.count]; g++) {
            is_supported = IsEqualGUID(*mode->guid, *g);
        }
        if (is_supported) {
            is_supported = profileSupported(mode, avctx);
            if (!is_supported) {
                char *psz_name = directxVAGetDecoderName(mode->guid);
                std::clog << "Unsupported profile " << avctx->profile << " for " << psz_name;
                free(psz_name);
            }
        }
        if (!is_supported) continue;

        /* */
        std::clog << "Trying to use " << mode->name << " as input";
        if (setupOutput(mode->guid) == VLC_SUCCESS) {
            input = *mode->guid;
            err = VLC_SUCCESS;
            break;
        }
    }

    p_list.pf_release(&p_list);
    return err;
}

int DirectXVA::directxVAOpen()
{
    if (mD3d->openDXResource() != VLC_SUCCESS) goto error;

    return VLC_SUCCESS;

error:
    return VLC_EGENERIC;
}
void DirectXVA::directxVAClose()
{
    mD3d->closeDXResource();
}
int DirectXVA::directxVASetup(const AVCodecContext *avctx, int flag_xbox)
{
    /* */
    if (FindVideoServiceConversion(avctx)) {
        std::clog << "FindVideoServiceConversion failed";
        return VLC_EGENERIC;
    }

    int surface_alignment = 16;
    unsigned surface_count = 2;

    switch (avctx->codec_id) {
        case AV_CODEC_ID_MPEG2VIDEO:
            /* decoding MPEG-2 requires additional alignment on some Intel GPUs,
           but it causes issues for H.264 on certain AMD GPUs..... */
            surface_alignment = 32;
            surface_count += 2 + 2; /* 2 for deinterlacing which can hold up to 2
                                 * pictures from the decoder for smoothing */
            break;
        case AV_CODEC_ID_HEVC:
            /* the HEVC DXVA2 spec asks for 128 pixel aligned surfaces to ensure
           all coding features have enough room to work with */
            /* On the Xbox 1/S, the decoder cannot do 4K aligned to 128 but is OK with 64 */
            if (flag_xbox)
                surface_alignment = 16;
            else
                surface_alignment = 128;
            surface_count += 16;
            break;
        case AV_CODEC_ID_H264:
            surface_count += 16;
            break;
        case AV_CODEC_ID_VP9:
            surface_count += 4;
            break;
        default:
            surface_count += 2;
    }

    if (avctx->active_thread_type & FF_THREAD_FRAME) surface_count += avctx->thread_count;

    int err = mD3d->setupDecoder(avctx, surface_count, surface_alignment);
    if (err != VLC_SUCCESS) return err;
    if (can_extern_pool) return VLC_SUCCESS;
    return mD3d->setupSurfaces(surface_count);
}
char *DirectXVA::directxVAGetDecoderName(const GUID *guid)
{
    for (unsigned i = 0; DXVA_MODES[i].name; i++) {
        if (IsEqualGUID(*DXVA_MODES[i].guid, *guid)) return strdup(DXVA_MODES[i].name);
    }

    char *psz_name;
    if (asprintf(&psz_name, "Unknown decoder " GUID_FMT, GUID_PRINT(*guid)) < 0) return NULL;
    return psz_name;
}


/* */
typedef struct {
    const char *name;
    D3DFORMAT format;
    vlc_fourcc_t codec;
} d3d9_format_t;
/* XXX Prefered format must come first */
static const d3d9_format_t d3d_formats[] = {{"YV12", (D3DFORMAT) MAKEFOURCC('Y', 'V', '1', '2'), VLC_CODEC_YV12},
                                            {"NV12", (D3DFORMAT) MAKEFOURCC('N', 'V', '1', '2'), VLC_CODEC_NV12},
                                            //{ "IMC3",   MAKEFOURCC('I','M','C','3'),    VLC_CODEC_YV12 },
                                            {"P010", (D3DFORMAT) MAKEFOURCC('P', '0', '1', '0'), VLC_CODEC_P010},

                                            {NULL, D3DFMT_UNKNOWN, 0}};

static const d3d9_format_t *D3dFindFormat(D3DFORMAT format)
{
    for (unsigned i = 0; d3d_formats[i].name; i++) {
        if (d3d_formats[i].format == format) return &d3d_formats[i];
    }
    return NULL;
}

DirectXVADXVA2::DirectXVADXVA2(VAD3D *d3d) : DirectXVA(d3d)
{
    mVAD3D = dynamic_cast<VADxva *>(d3d);
}

static void ReleaseInputList(input_list_t *p_list)
{
    CoTaskMemFree(p_list->list);
}

int DirectXVADXVA2::getInputList(input_list_t *p_list)
{
    UINT input_count = 0;
    GUID *input_list = NULL;
    if (FAILED(d3ddec->GetDecoderDeviceGuids(&input_count, &input_list))) {
        std::clog << "IDirectXVideoDecoderService_GetDecoderDeviceGuids failed";
        return VLC_EGENERIC;
    }

    p_list->count = input_count;
    p_list->list = input_list;
    p_list->pf_release = ReleaseInputList;
    return VLC_SUCCESS;
}
int DirectXVADXVA2::setupOutput(const GUID *input)
{
    D3DADAPTER_IDENTIFIER9 identifier;
    HRESULT hr = mVAD3D->hd3d.obj->GetAdapterIdentifier(mVAD3D->d3d_dev.adapterId, 0, &identifier);
    if (FAILED(hr)) return VLC_EGENERIC;

    UINT driverBuild = identifier.DriverVersion.LowPart & 0xFFFF;
    if (identifier.VendorId == GPU_MANUFACTURER_INTEL && (identifier.DriverVersion.LowPart >> 16) >= 100) {
        /* new Intel driver format */
        driverBuild += ((identifier.DriverVersion.LowPart >> 16) - 100) * 1000;
    }
    if (!directxVAcanUseDecoder(identifier.VendorId, identifier.DeviceId, input, driverBuild)) {
        char *psz_decoder_name = directxVAGetDecoderName(input);
        std::clog << "GPU blacklisted for %s codec" << psz_decoder_name;
        free(psz_decoder_name);
        return VLC_EGENERIC;
    }

    int err = VLC_EGENERIC;
    UINT output_count = 0;
    D3DFORMAT *output_list = NULL;
    if (FAILED(d3ddec->GetDecoderRenderTargets(*input, &output_count, &output_list))) {
        std::clog << "IDirectXVideoDecoderService_GetDecoderRenderTargets failed";
        return VLC_EGENERIC;
    }

    for (unsigned j = 0; j < output_count; j++) {
        const D3DFORMAT f = output_list[j];
        const d3d9_format_t *format = D3dFindFormat(f);
        if (format) {
            std::clog << format->name << " is supported for output";
        } else {
            std::clog << "%d is supported for output (%4.4s)" << f << (const char *) &f;
        }
    }

    /* */
    for (unsigned pass = 0; pass < 2 && err != VLC_SUCCESS; ++pass) {
        for (unsigned j = 0; d3d_formats[j].name; j++) {
            const d3d9_format_t *format = &d3d_formats[j];

            /* */
            bool is_supported = false;
            for (unsigned k = 0; !is_supported && k < output_count; k++) {
                is_supported = format->format == output_list[k];
            }
            if (!is_supported) continue;
            if (pass == 0 && format->format != mVAD3D->render) continue;

            /* We have our solution */
            std::clog << "Using decoder output " << format->name;
            mVAD3D->render = format->format;
            err = VLC_SUCCESS;
            break;
        }
    }
    CoTaskMemFree(output_list);
    return err;
}