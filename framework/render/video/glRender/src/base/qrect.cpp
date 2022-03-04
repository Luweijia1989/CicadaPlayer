#include "qrect.h"

QRect QRect::normalized() const noexcept
{
    QRect r;
    if (x2 < x1 - 1) {// swap bad x values
        r.x1 = x2;
        r.x2 = x1;
    } else {
        r.x1 = x1;
        r.x2 = x2;
    }
    if (y2 < y1 - 1) {// swap bad y values
        r.y1 = y2;
        r.y2 = y1;
    } else {
        r.y1 = y1;
        r.y2 = y2;
    }
    return r;
}


bool QRect::contains(const QPoint &p, bool proper) const noexcept
{
    int l, r;
    if (x2 < x1 - 1) {
        l = x2;
        r = x1;
    } else {
        l = x1;
        r = x2;
    }
    if (proper) {
        if (p.x() <= l || p.x() >= r) return false;
    } else {
        if (p.x() < l || p.x() > r) return false;
    }
    int t, b;
    if (y2 < y1 - 1) {
        t = y2;
        b = y1;
    } else {
        t = y1;
        b = y2;
    }
    if (proper) {
        if (p.y() <= t || p.y() >= b) return false;
    } else {
        if (p.y() < t || p.y() > b) return false;
    }
    return true;
}

bool QRect::contains(const QRect &r, bool proper) const noexcept
{
    if (isNull() || r.isNull()) return false;

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    if (proper) {
        if (l2 <= l1 || r2 >= r1) return false;
    } else {
        if (l2 < l1 || r2 > r1) return false;
    }

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    if (proper) {
        if (t2 <= t1 || b2 >= b1) return false;
    } else {
        if (t2 < t1 || b2 > b1) return false;
    }

    return true;
}

QRect QRect::operator|(const QRect &r) const noexcept
{
    if (isNull()) return r;
    if (r.isNull()) return *this;

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    QRect tmp;
    tmp.x1 = std::min(l1, l2);
    tmp.x2 = std::max(r1, r2);
    tmp.y1 = std::min(t1, t2);
    tmp.y2 = std::max(b1, b2);
    return tmp;
}


QRect QRect::operator&(const QRect &r) const noexcept
{
    if (isNull() || r.isNull()) return QRect();

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    if (l1 > r2 || l2 > r1) return QRect();

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    if (t1 > b2 || t2 > b1) return QRect();

    QRect tmp;
    tmp.x1 = std::max(l1, l2);
    tmp.x2 = std::min(r1, r2);
    tmp.y1 = std::max(t1, t2);
    tmp.y2 = std::min(b1, b2);
    return tmp;
}

bool QRect::intersects(const QRect &r) const noexcept
{
    if (isNull() || r.isNull()) return false;

    int l1 = x1;
    int r1 = x1;
    if (x2 - x1 + 1 < 0)
        l1 = x2;
    else
        r1 = x2;

    int l2 = r.x1;
    int r2 = r.x1;
    if (r.x2 - r.x1 + 1 < 0)
        l2 = r.x2;
    else
        r2 = r.x2;

    if (l1 > r2 || l2 > r1) return false;

    int t1 = y1;
    int b1 = y1;
    if (y2 - y1 + 1 < 0)
        t1 = y2;
    else
        b1 = y2;

    int t2 = r.y1;
    int b2 = r.y1;
    if (r.y2 - r.y1 + 1 < 0)
        t2 = r.y2;
    else
        b2 = r.y2;

    if (t1 > b2 || t2 > b1) return false;

    return true;
}

QRectF QRectF::normalized() const noexcept
{
    QRectF r = *this;
    if (r.w < 0) {
        r.xp += r.w;
        r.w = -r.w;
    }
    if (r.h < 0) {
        r.yp += r.h;
        r.h = -r.h;
    }
    return r;
}

bool QRectF::contains(const QPointF &p) const noexcept
{
    double l = xp;
    double r = xp;
    if (w < 0)
        l += w;
    else
        r += w;
    if (l == r)// null rect
        return false;

    if (p.x() < l || p.x() > r) return false;

    double t = yp;
    double b = yp;
    if (h < 0)
        t += h;
    else
        b += h;
    if (t == b)// null rect
        return false;

    if (p.y() < t || p.y() > b) return false;

    return true;
}

bool QRectF::contains(const QRectF &r) const noexcept
{
    double l1 = xp;
    double r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1)// null rect
        return false;

    double l2 = r.xp;
    double r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2)// null rect
        return false;

    if (l2 < l1 || r2 > r1) return false;

    double t1 = yp;
    double b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1)// null rect
        return false;

    double t2 = r.yp;
    double b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2)// null rect
        return false;

    if (t2 < t1 || b2 > b1) return false;

    return true;
}

QRectF QRectF::operator|(const QRectF &r) const noexcept
{
    if (isNull()) return r;
    if (r.isNull()) return *this;

    double left = xp;
    double right = xp;
    if (w < 0)
        left += w;
    else
        right += w;

    if (r.w < 0) {
        left = std::min(left, r.xp + r.w);
        right = std::max(right, r.xp);
    } else {
        left = std::min(left, r.xp);
        right = std::max(right, r.xp + r.w);
    }

    double top = yp;
    double bottom = yp;
    if (h < 0)
        top += h;
    else
        bottom += h;

    if (r.h < 0) {
        top = std::min(top, r.yp + r.h);
        bottom = std::max(bottom, r.yp);
    } else {
        top = std::min(top, r.yp);
        bottom = std::max(bottom, r.yp + r.h);
    }

    return QRectF(left, top, right - left, bottom - top);
}

QRectF QRectF::operator&(const QRectF &r) const noexcept
{
    double l1 = xp;
    double r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1)// null rect
        return QRectF();

    double l2 = r.xp;
    double r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2)// null rect
        return QRectF();

    if (l1 >= r2 || l2 >= r1) return QRectF();

    double t1 = yp;
    double b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1)// null rect
        return QRectF();

    double t2 = r.yp;
    double b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2)// null rect
        return QRectF();

    if (t1 >= b2 || t2 >= b1) return QRectF();

    QRectF tmp;
    tmp.xp = std::max(l1, l2);
    tmp.yp = std::max(t1, t2);
    tmp.w = std::min(r1, r2) - tmp.xp;
    tmp.h = std::min(b1, b2) - tmp.yp;
    return tmp;
}

bool QRectF::intersects(const QRectF &r) const noexcept
{
    double l1 = xp;
    double r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1)// null rect
        return false;

    double l2 = r.xp;
    double r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2)// null rect
        return false;

    if (l1 >= r2 || l2 >= r1) return false;

    double t1 = yp;
    double b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1)// null rect
        return false;

    double t2 = r.yp;
    double b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2)// null rect
        return false;

    if (t1 >= b2 || t2 >= b1) return false;

    return true;
}

QRect QRectF::toAlignedRect() const noexcept
{
    int xmin = int(std::floor(xp));
    int xmax = int(std::ceil(xp + w));
    int ymin = int(std::floor(yp));
    int ymax = int(std::ceil(yp + h));
    return QRect(xmin, ymin, xmax - xmin, ymax - ymin);
}