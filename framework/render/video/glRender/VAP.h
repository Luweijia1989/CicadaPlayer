#pragma once

#include <utils/CicadaJSON.h>

inline int fromHex(int c) noexcept
{
    return ((c >= '0') && (c <= '9')) ? int(c - '0')
           : ((c >= 'A') && (c <= 'F')) ? int(c - 'A' + 10)
           : ((c >= 'a') && (c <= 'f')) ? int(c - 'a' + 10) :
           /* otherwise */ -1;
}

static inline int hex2int(const char *s, int n)
{
    if (n < 0) return -1;
    int result = 0;
    for (; n > 0; --n) {
        result = result * 16;
        const int h = fromHex(*s++);
        if (h < 0) return -1;
        result += h;
    }
    return result;
}

static bool get_hex_rgb(const char *name, size_t len, float* rgb)
{
    if (name[0] != '#') 
		return false;

    name++;
    --len;
    int a, r, g, b;
    a = 65535;
    if (len == 12) {
        r = hex2int(name + 0, 4);
        g = hex2int(name + 4, 4);
        b = hex2int(name + 8, 4);
    } else if (len == 9) {
        r = hex2int(name + 0, 3);
        g = hex2int(name + 3, 3);
        b = hex2int(name + 6, 3);
        r = (r << 4) | (r >> 8);
        g = (g << 4) | (g >> 8);
        b = (b << 4) | (b >> 8);
    } else if (len == 8) {
        a = hex2int(name + 0, 2) * 0x101;
        r = hex2int(name + 2, 2) * 0x101;
        g = hex2int(name + 4, 2) * 0x101;
        b = hex2int(name + 6, 2) * 0x101;
    } else if (len == 6) {
        r = hex2int(name + 0, 2) * 0x101;
        g = hex2int(name + 2, 2) * 0x101;
        b = hex2int(name + 4, 2) * 0x101;
    } else if (len == 3) {
        r = hex2int(name + 0, 1) * 0x1111;
        g = hex2int(name + 1, 1) * 0x1111;
        b = hex2int(name + 2, 1) * 0x1111;
    } else {
        r = g = b = -1;
    }
    if (r > 65535 || g > 65535 || b > 65535 || a > 65535) {
        rgb[0] = 0.0f;
        rgb[1] = 0.0f;
        rgb[2] = 0.0f;
        rgb[3] = 0.0f;
        return false;
    }
    rgb[0] = r / 65535.0f;
    rgb[1] = g / 65535.0f;
    rgb[2] = b / 65535.0f;
    rgb[3] = a / 65535.0f;
    return true;
}

class Rect {
public:
    Rect(size_t l, size_t t, size_t r, size_t b)
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }
	Rect()
	{
        left = 0;
        top = 0;
        right = 0;
        bottom = 0;
	}
    ~Rect() = default;

	size_t width() const
	{
		return right - left;
	}

	size_t height() const
	{
		return bottom - top;
	}

public:
    size_t left;
    size_t top;
    size_t right;
    size_t bottom;
};

struct VapAnimateConfig {
    int version = 0;
    int totalFrames = 0;
    int width = 0;
    int height = 0;
    int videoWidth = 0;
    int videoHeight = 0;
    int orien = 0;
    int fps = 0;
    bool isMix = 0;
    Rect alphaPointRect = Rect(0, 0, 0, 0);
    Rect rgbPointRect = Rect(0, 0, 0, 0);
};

class MixSrc {
public:
    enum SrcType {
        SRC_TYPE_UNKNOWN,
        IMG,
        TXT,
    };

    enum LoadType {
        LOAD_TYPE_UNKNOWN,
        NET,
        LOCAL,
    };

    enum FitType {
        FIT_XY,
        CENTER_FULL,
    };

    enum Style {
        DEFAULT,
        BOLD,
    };

    MixSrc()
    {}
    MixSrc(const CicadaJSONItem &json)
    {
        srcId = json.getString("srcId");
        w = json.getInt("w", 0);
        h = json.getInt("h", 0);
        std::string colorStr = json.getString("color");
        get_hex_rgb(colorStr.c_str(), colorStr.length(), color);

        srcTag = json.getString("srcTag");

        std::string srcTypeStr = json.getString("srcType");
        if (srcTypeStr.length() == 0)
            srcType = SrcType::SRC_TYPE_UNKNOWN;
        else if (srcTypeStr == "img")
            srcType = SrcType::IMG;
        else if (srcTypeStr == "txt")
            srcType = SrcType::TXT;

        std::string loadTypeStr = json.getString("loadType");
        if (loadTypeStr.length() == 0)
            loadType = LoadType::LOAD_TYPE_UNKNOWN;
        else if (loadTypeStr == "net")
            loadType = LoadType::NET;
        else if (loadTypeStr == "local")
            loadType = LoadType::LOCAL;

        std::string fitTypeStr = json.getString("fitType");
        if (fitTypeStr == "fitXY")
            fitType = FitType::FIT_XY;
        else
            fitType = FitType::CENTER_FULL;

        std::string styleStr = json.getString("style");
        if (styleStr == "bold")
            style = Style::BOLD;
        else
            style = Style::DEFAULT;
    }

    std::string srcId = "";
    int w = 0;
    int h = 0;
    SrcType srcType = SrcType::SRC_TYPE_UNKNOWN;
    LoadType loadType = LoadType::LOAD_TYPE_UNKNOWN;
    std::string srcTag = "";
    Style style = Style::DEFAULT;
    float color[4];
    FitType fitType = FitType::FIT_XY;
    unsigned int srcTexture = 0;
};

struct VAPFrame {
    std::string srcId = "";
    int z = 0;
    Rect frame;
    Rect mFrame;
    int mt = 0;

    VAPFrame(const CicadaJSONItem& json)
    {
        srcId = json.getString("srcId");
        z = json.getInt("z", 0);
        CicadaJSONArray f = json.getItem("frame");
        frame = Rect(f.getItem(0).getInt(), f.getItem(1).getInt(), f.getItem(0).getInt() + f.getItem(2).getInt(), f.getItem(1).getInt() + f.getItem(3).getInt());

        CicadaJSONArray m = json.getItem("mFrame");
        mFrame = Rect(m.getItem(0).getInt(), m.getItem(1).getInt(), m.getItem(0).getInt() + m.getItem(2).getInt(), m.getItem(1).getInt() + m.getItem(3).getInt());
        mt = json.getInt("mt", 0);
    }
};

struct FrameSet {
    int index = 0;
    std::map<int, VAPFrame> m_frames;
    FrameSet()
    {}
    FrameSet(const CicadaJSONItem &json)
    {
        index = json.getInt("i", 0);
        CicadaJSONArray objArr = json.getItem("obj");
        for (int i = 0; i < objArr.getSize(); i++) {
            CicadaJSONItem one = objArr.getItem(i);
			m_frames.insert({one.getInt("z", 0), VAPFrame(one)});
        }
    }
};
