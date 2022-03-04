#pragma once

#include "qpoint.h"
#include "qsize.h"

class QRect {
public:
    QRect() noexcept : x1(0), y1(0), x2(-1), y2(-1)
    {}
    QRect(const QPoint &topleft, const QPoint &bottomright) noexcept;
    QRect(const QPoint &topleft, const QSize &size) noexcept;
    QRect(int left, int top, int width, int height) noexcept;

    inline bool isNull() const noexcept;
    inline bool isEmpty() const noexcept;
    inline bool isValid() const noexcept;

    inline int left() const noexcept;
    inline int top() const noexcept;
    inline int right() const noexcept;
    inline int bottom() const noexcept;
    QRect normalized() const noexcept;

    inline int x() const noexcept;
    inline int y() const noexcept;
    inline void setLeft(int pos) noexcept;
    inline void setTop(int pos) noexcept;
    inline void setRight(int pos) noexcept;
    inline void setBottom(int pos) noexcept;
    inline void setX(int x) noexcept;
    inline void setY(int y) noexcept;

    inline void setTopLeft(const QPoint &p) noexcept;
    inline void setBottomRight(const QPoint &p) noexcept;
    inline void setTopRight(const QPoint &p) noexcept;
    inline void setBottomLeft(const QPoint &p) noexcept;

    inline QPoint topLeft() const noexcept;
    inline QPoint bottomRight() const noexcept;
    inline QPoint topRight() const noexcept;
    inline QPoint bottomLeft() const noexcept;
    inline QPoint center() const noexcept;

    inline void moveLeft(int pos) noexcept;
    inline void moveTop(int pos) noexcept;
    inline void moveRight(int pos) noexcept;
    inline void moveBottom(int pos) noexcept;
    inline void moveTopLeft(const QPoint &p) noexcept;
    inline void moveBottomRight(const QPoint &p) noexcept;
    inline void moveTopRight(const QPoint &p) noexcept;
    inline void moveBottomLeft(const QPoint &p) noexcept;
    inline void moveCenter(const QPoint &p) noexcept;

    inline void translate(int dx, int dy) noexcept;
    inline void translate(const QPoint &p) noexcept;
    inline QRect translated(int dx, int dy) const noexcept;
    inline QRect translated(const QPoint &p) const noexcept;
    inline QRect transposed() const noexcept;

    inline void moveTo(int x, int t) noexcept;
    inline void moveTo(const QPoint &p) noexcept;

    inline void setRect(int x, int y, int w, int h) noexcept;
    inline void getRect(int *x, int *y, int *w, int *h) const;

    inline void setCoords(int x1, int y1, int x2, int y2) noexcept;
    inline void getCoords(int *x1, int *y1, int *x2, int *y2) const;

    inline void adjust(int x1, int y1, int x2, int y2) noexcept;
    inline QRect adjusted(int x1, int y1, int x2, int y2) const noexcept;

    inline QSize size() const noexcept;
    inline int width() const noexcept;
    inline int height() const noexcept;
    inline void setWidth(int w) noexcept;
    inline void setHeight(int h) noexcept;
    inline void setSize(const QSize &s) noexcept;

    QRect operator|(const QRect &r) const noexcept;
    QRect operator&(const QRect &r) const noexcept;
    inline QRect &operator|=(const QRect &r) noexcept;
    inline QRect &operator&=(const QRect &r) noexcept;

    bool contains(const QRect &r, bool proper = false) const noexcept;
    bool contains(const QPoint &p, bool proper = false) const noexcept;
    inline bool contains(int x, int y) const noexcept;
    inline bool contains(int x, int y, bool proper) const noexcept;
    inline QRect united(const QRect &other) const noexcept;
    inline QRect intersected(const QRect &other) const noexcept;
    bool intersects(const QRect &r) const noexcept;

    friend inline bool operator==(const QRect &, const QRect &) noexcept;
    friend inline bool operator!=(const QRect &, const QRect &) noexcept;

private:
    int x1;
    int y1;
    int x2;
    int y2;
};

inline bool operator==(const QRect &, const QRect &) noexcept;
inline bool operator!=(const QRect &, const QRect &) noexcept;

inline QRect::QRect(int aleft, int atop, int awidth, int aheight) noexcept
    : x1(aleft), y1(atop), x2(aleft + awidth - 1), y2(atop + aheight - 1)
{}

inline QRect::QRect(const QPoint &atopLeft, const QPoint &abottomRight) noexcept
    : x1(atopLeft.x()), y1(atopLeft.y()), x2(abottomRight.x()), y2(abottomRight.y())
{}

inline QRect::QRect(const QPoint &atopLeft, const QSize &asize) noexcept
    : x1(atopLeft.x()), y1(atopLeft.y()), x2(atopLeft.x() + asize.width() - 1), y2(atopLeft.y() + asize.height() - 1)
{}

inline bool QRect::isNull() const noexcept
{
    return x2 == x1 - 1 && y2 == y1 - 1;
}

inline bool QRect::isEmpty() const noexcept
{
    return x1 > x2 || y1 > y2;
}

inline bool QRect::isValid() const noexcept
{
    return x1 <= x2 && y1 <= y2;
}

inline int QRect::left() const noexcept
{
    return x1;
}

inline int QRect::top() const noexcept
{
    return y1;
}

inline int QRect::right() const noexcept
{
    return x2;
}

inline int QRect::bottom() const noexcept
{
    return y2;
}

inline int QRect::x() const noexcept
{
    return x1;
}

inline int QRect::y() const noexcept
{
    return y1;
}

inline void QRect::setLeft(int pos) noexcept
{
    x1 = pos;
}

inline void QRect::setTop(int pos) noexcept
{
    y1 = pos;
}

inline void QRect::setRight(int pos) noexcept
{
    x2 = pos;
}

inline void QRect::setBottom(int pos) noexcept
{
    y2 = pos;
}

inline void QRect::setTopLeft(const QPoint &p) noexcept
{
    x1 = p.x();
    y1 = p.y();
}

inline void QRect::setBottomRight(const QPoint &p) noexcept
{
    x2 = p.x();
    y2 = p.y();
}

inline void QRect::setTopRight(const QPoint &p) noexcept
{
    x2 = p.x();
    y1 = p.y();
}

inline void QRect::setBottomLeft(const QPoint &p) noexcept
{
    x1 = p.x();
    y2 = p.y();
}

inline void QRect::setX(int ax) noexcept
{
    x1 = ax;
}

inline void QRect::setY(int ay) noexcept
{
    y1 = ay;
}

inline QPoint QRect::topLeft() const noexcept
{
    return QPoint(x1, y1);
}

inline QPoint QRect::bottomRight() const noexcept
{
    return QPoint(x2, y2);
}

inline QPoint QRect::topRight() const noexcept
{
    return QPoint(x2, y1);
}

inline QPoint QRect::bottomLeft() const noexcept
{
    return QPoint(x1, y2);
}

inline QPoint QRect::center() const noexcept
{
    return QPoint(int((__int64(x1) + x2) / 2), int((__int64(y1) + y2) / 2));
}// cast avoids overflow on addition

inline int QRect::width() const noexcept
{
    return x2 - x1 + 1;
}

inline int QRect::height() const noexcept
{
    return y2 - y1 + 1;
}

inline QSize QRect::size() const noexcept
{
    return QSize(width(), height());
}

inline void QRect::translate(int dx, int dy) noexcept
{
    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;
}

inline void QRect::translate(const QPoint &p) noexcept
{
    x1 += p.x();
    y1 += p.y();
    x2 += p.x();
    y2 += p.y();
}

inline QRect QRect::translated(int dx, int dy) const noexcept
{
    return QRect(QPoint(x1 + dx, y1 + dy), QPoint(x2 + dx, y2 + dy));
}

inline QRect QRect::translated(const QPoint &p) const noexcept
{
    return QRect(QPoint(x1 + p.x(), y1 + p.y()), QPoint(x2 + p.x(), y2 + p.y()));
}

inline QRect QRect::transposed() const noexcept
{
    return QRect(topLeft(), size().transposed());
}

inline void QRect::moveTo(int ax, int ay) noexcept
{
    x2 += ax - x1;
    y2 += ay - y1;
    x1 = ax;
    y1 = ay;
}

inline void QRect::moveTo(const QPoint &p) noexcept
{
    x2 += p.x() - x1;
    y2 += p.y() - y1;
    x1 = p.x();
    y1 = p.y();
}

inline void QRect::moveLeft(int pos) noexcept
{
    x2 += (pos - x1);
    x1 = pos;
}

inline void QRect::moveTop(int pos) noexcept
{
    y2 += (pos - y1);
    y1 = pos;
}

inline void QRect::moveRight(int pos) noexcept
{
    x1 += (pos - x2);
    x2 = pos;
}

inline void QRect::moveBottom(int pos) noexcept
{
    y1 += (pos - y2);
    y2 = pos;
}

inline void QRect::moveTopLeft(const QPoint &p) noexcept
{
    moveLeft(p.x());
    moveTop(p.y());
}

inline void QRect::moveBottomRight(const QPoint &p) noexcept
{
    moveRight(p.x());
    moveBottom(p.y());
}

inline void QRect::moveTopRight(const QPoint &p) noexcept
{
    moveRight(p.x());
    moveTop(p.y());
}

inline void QRect::moveBottomLeft(const QPoint &p) noexcept
{
    moveLeft(p.x());
    moveBottom(p.y());
}

inline void QRect::moveCenter(const QPoint &p) noexcept
{
    int w = x2 - x1;
    int h = y2 - y1;
    x1 = p.x() - w / 2;
    y1 = p.y() - h / 2;
    x2 = x1 + w;
    y2 = y1 + h;
}

inline void QRect::getRect(int *ax, int *ay, int *aw, int *ah) const
{
    *ax = x1;
    *ay = y1;
    *aw = x2 - x1 + 1;
    *ah = y2 - y1 + 1;
}

inline void QRect::setRect(int ax, int ay, int aw, int ah) noexcept
{
    x1 = ax;
    y1 = ay;
    x2 = (ax + aw - 1);
    y2 = (ay + ah - 1);
}

inline void QRect::getCoords(int *xp1, int *yp1, int *xp2, int *yp2) const
{
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

inline void QRect::setCoords(int xp1, int yp1, int xp2, int yp2) noexcept
{
    x1 = xp1;
    y1 = yp1;
    x2 = xp2;
    y2 = yp2;
}

inline QRect QRect::adjusted(int xp1, int yp1, int xp2, int yp2) const noexcept
{
    return QRect(QPoint(x1 + xp1, y1 + yp1), QPoint(x2 + xp2, y2 + yp2));
}

inline void QRect::adjust(int dx1, int dy1, int dx2, int dy2) noexcept
{
    x1 += dx1;
    y1 += dy1;
    x2 += dx2;
    y2 += dy2;
}

inline void QRect::setWidth(int w) noexcept
{
    x2 = (x1 + w - 1);
}

inline void QRect::setHeight(int h) noexcept
{
    y2 = (y1 + h - 1);
}

inline void QRect::setSize(const QSize &s) noexcept
{
    x2 = (s.width() + x1 - 1);
    y2 = (s.height() + y1 - 1);
}

inline bool QRect::contains(int ax, int ay, bool aproper) const noexcept
{
    return contains(QPoint(ax, ay), aproper);
}

inline bool QRect::contains(int ax, int ay) const noexcept
{
    return contains(QPoint(ax, ay), false);
}

inline QRect &QRect::operator|=(const QRect &r) noexcept
{
    *this = *this | r;
    return *this;
}

inline QRect &QRect::operator&=(const QRect &r) noexcept
{
    *this = *this & r;
    return *this;
}

inline QRect QRect::intersected(const QRect &other) const noexcept
{
    return *this & other;
}

inline QRect QRect::united(const QRect &r) const noexcept
{
    return *this | r;
}

inline bool operator==(const QRect &r1, const QRect &r2) noexcept
{
    return r1.x1 == r2.x1 && r1.x2 == r2.x2 && r1.y1 == r2.y1 && r1.y2 == r2.y2;
}

inline bool operator!=(const QRect &r1, const QRect &r2) noexcept
{
    return r1.x1 != r2.x1 || r1.x2 != r2.x2 || r1.y1 != r2.y1 || r1.y2 != r2.y2;
}

class QRectF {
public:
    QRectF() noexcept : xp(0.), yp(0.), w(0.), h(0.)
    {}
    QRectF(const QPointF &topleft, const QSizeF &size) noexcept;
    QRectF(const QPointF &topleft, const QPointF &bottomRight) noexcept;
    QRectF(double left, double top, double width, double height) noexcept;
    QRectF(const QRect &rect) noexcept;

    inline bool isNull() const noexcept;
    inline bool isEmpty() const noexcept;
    inline bool isValid() const noexcept;
    QRectF normalized() const noexcept;

    inline double left() const noexcept
    {
        return xp;
    }
    inline double top() const noexcept
    {
        return yp;
    }
    inline double right() const noexcept
    {
        return xp + w;
    }
    inline double bottom() const noexcept
    {
        return yp + h;
    }

    inline double x() const noexcept;
    inline double y() const noexcept;
    inline void setLeft(double pos) noexcept;
    inline void setTop(double pos) noexcept;
    inline void setRight(double pos) noexcept;
    inline void setBottom(double pos) noexcept;
    inline void setX(double pos) noexcept
    {
        setLeft(pos);
    }
    inline void setY(double pos) noexcept
    {
        setTop(pos);
    }

    inline QPointF topLeft() const noexcept
    {
        return QPointF(xp, yp);
    }
    inline QPointF bottomRight() const noexcept
    {
        return QPointF(xp + w, yp + h);
    }
    inline QPointF topRight() const noexcept
    {
        return QPointF(xp + w, yp);
    }
    inline QPointF bottomLeft() const noexcept
    {
        return QPointF(xp, yp + h);
    }
    inline QPointF center() const noexcept;

    inline void setTopLeft(const QPointF &p) noexcept;
    inline void setBottomRight(const QPointF &p) noexcept;
    inline void setTopRight(const QPointF &p) noexcept;
    inline void setBottomLeft(const QPointF &p) noexcept;

    inline void moveLeft(double pos) noexcept;
    inline void moveTop(double pos) noexcept;
    inline void moveRight(double pos) noexcept;
    inline void moveBottom(double pos) noexcept;
    inline void moveTopLeft(const QPointF &p) noexcept;
    inline void moveBottomRight(const QPointF &p) noexcept;
    inline void moveTopRight(const QPointF &p) noexcept;
    inline void moveBottomLeft(const QPointF &p) noexcept;
    inline void moveCenter(const QPointF &p) noexcept;

    inline void translate(double dx, double dy) noexcept;
    inline void translate(const QPointF &p) noexcept;

    inline QRectF translated(double dx, double dy) const noexcept;
    inline QRectF translated(const QPointF &p) const noexcept;

    inline QRectF transposed() const noexcept;

    inline void moveTo(double x, double y) noexcept;
    inline void moveTo(const QPointF &p) noexcept;

    inline void setRect(double x, double y, double w, double h) noexcept;
    inline void getRect(double *x, double *y, double *w, double *h) const;

    inline void setCoords(double x1, double y1, double x2, double y2) noexcept;
    inline void getCoords(double *x1, double *y1, double *x2, double *y2) const;

    inline void adjust(double x1, double y1, double x2, double y2) noexcept;
    inline QRectF adjusted(double x1, double y1, double x2, double y2) const noexcept;

    inline QSizeF size() const noexcept;
    inline double width() const noexcept;
    inline double height() const noexcept;
    inline void setWidth(double w) noexcept;
    inline void setHeight(double h) noexcept;
    inline void setSize(const QSizeF &s) noexcept;

    QRectF operator|(const QRectF &r) const noexcept;
    QRectF operator&(const QRectF &r) const noexcept;
    inline QRectF &operator|=(const QRectF &r) noexcept;
    inline QRectF &operator&=(const QRectF &r) noexcept;

    bool contains(const QRectF &r) const noexcept;
    bool contains(const QPointF &p) const noexcept;
    inline bool contains(double x, double y) const noexcept;
    inline QRectF united(const QRectF &other) const noexcept;
    inline QRectF intersected(const QRectF &other) const noexcept;
    bool intersects(const QRectF &r) const noexcept;

    friend inline bool operator==(const QRectF &, const QRectF &) noexcept;
    friend inline bool operator!=(const QRectF &, const QRectF &) noexcept;

    inline QRect toRect() const noexcept;
    QRect toAlignedRect() const noexcept;

private:
    double xp;
    double yp;
    double w;
    double h;
};

inline bool operator==(const QRectF &, const QRectF &) noexcept;
inline bool operator!=(const QRectF &, const QRectF &) noexcept;

inline QRectF::QRectF(double aleft, double atop, double awidth, double aheight) noexcept : xp(aleft), yp(atop), w(awidth), h(aheight)
{}

inline QRectF::QRectF(const QPointF &atopLeft, const QSizeF &asize) noexcept
    : xp(atopLeft.x()), yp(atopLeft.y()), w(asize.width()), h(asize.height())
{}


inline QRectF::QRectF(const QPointF &atopLeft, const QPointF &abottomRight) noexcept
    : xp(atopLeft.x()), yp(atopLeft.y()), w(abottomRight.x() - atopLeft.x()), h(abottomRight.y() - atopLeft.y())
{}

inline QRectF::QRectF(const QRect &r) noexcept : xp(r.x()), yp(r.y()), w(r.width()), h(r.height())
{}

inline bool QRectF::isNull() const noexcept
{
    return w == 0. && h == 0.;
}

inline bool QRectF::isEmpty() const noexcept
{
    return w <= 0. || h <= 0.;
}

inline bool QRectF::isValid() const noexcept
{
    return w > 0. && h > 0.;
}

inline double QRectF::x() const noexcept
{
    return xp;
}

inline double QRectF::y() const noexcept
{
    return yp;
}

inline void QRectF::setLeft(double pos) noexcept
{
    double diff = pos - xp;
    xp += diff;
    w -= diff;
}

inline void QRectF::setRight(double pos) noexcept
{
    w = pos - xp;
}

inline void QRectF::setTop(double pos) noexcept
{
    double diff = pos - yp;
    yp += diff;
    h -= diff;
}

inline void QRectF::setBottom(double pos) noexcept
{
    h = pos - yp;
}

inline void QRectF::setTopLeft(const QPointF &p) noexcept
{
    setLeft(p.x());
    setTop(p.y());
}

inline void QRectF::setTopRight(const QPointF &p) noexcept
{
    setRight(p.x());
    setTop(p.y());
}

inline void QRectF::setBottomLeft(const QPointF &p) noexcept
{
    setLeft(p.x());
    setBottom(p.y());
}

inline void QRectF::setBottomRight(const QPointF &p) noexcept
{
    setRight(p.x());
    setBottom(p.y());
}

inline QPointF QRectF::center() const noexcept
{
    return QPointF(xp + w / 2, yp + h / 2);
}

inline void QRectF::moveLeft(double pos) noexcept
{
    xp = pos;
}

inline void QRectF::moveTop(double pos) noexcept
{
    yp = pos;
}

inline void QRectF::moveRight(double pos) noexcept
{
    xp = pos - w;
}

inline void QRectF::moveBottom(double pos) noexcept
{
    yp = pos - h;
}

inline void QRectF::moveTopLeft(const QPointF &p) noexcept
{
    moveLeft(p.x());
    moveTop(p.y());
}

inline void QRectF::moveTopRight(const QPointF &p) noexcept
{
    moveRight(p.x());
    moveTop(p.y());
}

inline void QRectF::moveBottomLeft(const QPointF &p) noexcept
{
    moveLeft(p.x());
    moveBottom(p.y());
}

inline void QRectF::moveBottomRight(const QPointF &p) noexcept
{
    moveRight(p.x());
    moveBottom(p.y());
}

inline void QRectF::moveCenter(const QPointF &p) noexcept
{
    xp = p.x() - w / 2;
    yp = p.y() - h / 2;
}

inline double QRectF::width() const noexcept
{
    return w;
}

inline double QRectF::height() const noexcept
{
    return h;
}

inline QSizeF QRectF::size() const noexcept
{
    return QSizeF(w, h);
}

inline void QRectF::translate(double dx, double dy) noexcept
{
    xp += dx;
    yp += dy;
}

inline void QRectF::translate(const QPointF &p) noexcept
{
    xp += p.x();
    yp += p.y();
}

inline void QRectF::moveTo(double ax, double ay) noexcept
{
    xp = ax;
    yp = ay;
}

inline void QRectF::moveTo(const QPointF &p) noexcept
{
    xp = p.x();
    yp = p.y();
}

inline QRectF QRectF::translated(double dx, double dy) const noexcept
{
    return QRectF(xp + dx, yp + dy, w, h);
}

inline QRectF QRectF::translated(const QPointF &p) const noexcept
{
    return QRectF(xp + p.x(), yp + p.y(), w, h);
}

inline QRectF QRectF::transposed() const noexcept
{
    return QRectF(topLeft(), size().transposed());
}

inline void QRectF::getRect(double *ax, double *ay, double *aaw, double *aah) const
{
    *ax = this->xp;
    *ay = this->yp;
    *aaw = this->w;
    *aah = this->h;
}

inline void QRectF::setRect(double ax, double ay, double aaw, double aah) noexcept
{
    this->xp = ax;
    this->yp = ay;
    this->w = aaw;
    this->h = aah;
}

inline void QRectF::getCoords(double *xp1, double *yp1, double *xp2, double *yp2) const
{
    *xp1 = xp;
    *yp1 = yp;
    *xp2 = xp + w;
    *yp2 = yp + h;
}

inline void QRectF::setCoords(double xp1, double yp1, double xp2, double yp2) noexcept
{
    xp = xp1;
    yp = yp1;
    w = xp2 - xp1;
    h = yp2 - yp1;
}

inline void QRectF::adjust(double xp1, double yp1, double xp2, double yp2) noexcept
{
    xp += xp1;
    yp += yp1;
    w += xp2 - xp1;
    h += yp2 - yp1;
}

inline QRectF QRectF::adjusted(double xp1, double yp1, double xp2, double yp2) const noexcept
{
    return QRectF(xp + xp1, yp + yp1, w + xp2 - xp1, h + yp2 - yp1);
}

inline void QRectF::setWidth(double aw) noexcept
{
    this->w = aw;
}

inline void QRectF::setHeight(double ah) noexcept
{
    this->h = ah;
}

inline void QRectF::setSize(const QSizeF &s) noexcept
{
    w = s.width();
    h = s.height();
}

inline bool QRectF::contains(double ax, double ay) const noexcept
{
    return contains(QPointF(ax, ay));
}

inline QRectF &QRectF::operator|=(const QRectF &r) noexcept
{
    *this = *this | r;
    return *this;
}

inline QRectF &QRectF::operator&=(const QRectF &r) noexcept
{
    *this = *this & r;
    return *this;
}

inline QRectF QRectF::intersected(const QRectF &r) const noexcept
{
    return *this & r;
}

inline QRectF QRectF::united(const QRectF &r) const noexcept
{
    return *this | r;
}

inline bool operator==(const QRectF &r1, const QRectF &r2) noexcept
{
    return qFuzzyCompare(r1.xp, r2.xp) && qFuzzyCompare(r1.yp, r2.yp) && qFuzzyCompare(r1.w, r2.w) && qFuzzyCompare(r1.h, r2.h);
}

inline bool operator!=(const QRectF &r1, const QRectF &r2) noexcept
{
    return !qFuzzyCompare(r1.xp, r2.xp) || !qFuzzyCompare(r1.yp, r2.yp) || !qFuzzyCompare(r1.w, r2.w) || !qFuzzyCompare(r1.h, r2.h);
}

inline QRect QRectF::toRect() const noexcept
{
    return QRect(QPoint(std::round(xp), std::round(yp)), QPoint(std::round(xp + w) - 1, std::round(yp + h) - 1));
}