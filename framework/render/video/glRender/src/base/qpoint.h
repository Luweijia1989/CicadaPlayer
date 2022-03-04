#pragma once

#include "common.h"

class QPoint {
public:
    QPoint();
    QPoint(int xpos, int ypos);

    inline bool isNull() const;

    inline int x() const;
    inline int y() const;
    inline void setX(int x);
    inline void setY(int y);

    inline int manhattanLength() const;

    inline int &rx();
    inline int &ry();

    inline QPoint &operator+=(const QPoint &p);
    inline QPoint &operator-=(const QPoint &p);

    inline QPoint &operator*=(float factor);
    inline QPoint &operator*=(double factor);
    inline QPoint &operator*=(int factor);

    inline QPoint &operator/=(double divisor);

    static inline int dotProduct(const QPoint &p1, const QPoint &p2)
    {
        return p1.xp * p2.xp + p1.yp * p2.yp;
    }

    friend inline bool operator==(const QPoint &, const QPoint &);
    friend inline bool operator!=(const QPoint &, const QPoint &);
    friend inline const QPoint operator+(const QPoint &, const QPoint &);
    friend inline const QPoint operator-(const QPoint &, const QPoint &);
    friend inline const QPoint operator*(const QPoint &, float);
    friend inline const QPoint operator*(float, const QPoint &);
    friend inline const QPoint operator*(const QPoint &, double);
    friend inline const QPoint operator*(double, const QPoint &);
    friend inline const QPoint operator*(const QPoint &, int);
    friend inline const QPoint operator*(int, const QPoint &);
    friend inline const QPoint operator+(const QPoint &);
    friend inline const QPoint operator-(const QPoint &);
    friend inline const QPoint operator/(const QPoint &, double);

private:
    int xp;
    int yp;
};

inline QPoint::QPoint() : xp(0), yp(0)
{}

inline QPoint::QPoint(int xpos, int ypos) : xp(xpos), yp(ypos)
{}

inline bool QPoint::isNull() const
{
    return xp == 0 && yp == 0;
}

inline int QPoint::x() const
{
    return xp;
}

inline int QPoint::y() const
{
    return yp;
}

inline void QPoint::setX(int xpos)
{
    xp = xpos;
}

inline void QPoint::setY(int ypos)
{
    yp = ypos;
}

inline int QPoint::manhattanLength() const
{
    return std::abs(x()) + std::abs(y());
}

inline int &QPoint::rx()
{
    return xp;
}

inline int &QPoint::ry()
{
    return yp;
}

inline QPoint &QPoint::operator+=(const QPoint &p)
{
    xp += p.xp;
    yp += p.yp;
    return *this;
}

inline QPoint &QPoint::operator-=(const QPoint &p)
{
    xp -= p.xp;
    yp -= p.yp;
    return *this;
}

inline QPoint &QPoint::operator*=(float factor)
{
    xp = std::round(xp * factor);
    yp = std::round(yp * factor);
    return *this;
}

inline QPoint &QPoint::operator*=(double factor)
{
    xp = std::round(xp * factor);
    yp = std::round(yp * factor);
    return *this;
}

inline QPoint &QPoint::operator*=(int factor)
{
    xp = xp * factor;
    yp = yp * factor;
    return *this;
}

inline bool operator==(const QPoint &p1, const QPoint &p2)
{
    return p1.xp == p2.xp && p1.yp == p2.yp;
}

inline bool operator!=(const QPoint &p1, const QPoint &p2)
{
    return p1.xp != p2.xp || p1.yp != p2.yp;
}

inline const QPoint operator+(const QPoint &p1, const QPoint &p2)
{
    return QPoint(p1.xp + p2.xp, p1.yp + p2.yp);
}

inline const QPoint operator-(const QPoint &p1, const QPoint &p2)
{
    return QPoint(p1.xp - p2.xp, p1.yp - p2.yp);
}

inline const QPoint operator*(const QPoint &p, float factor)
{
    return QPoint(std::round(p.xp * factor), std::round(p.yp * factor));
}

inline const QPoint operator*(const QPoint &p, double factor)
{
    return QPoint(std::round(p.xp * factor), std::round(p.yp * factor));
}

inline const QPoint operator*(const QPoint &p, int factor)
{
    return QPoint(p.xp * factor, p.yp * factor);
}

inline const QPoint operator*(float factor, const QPoint &p)
{
    return QPoint(std::round(p.xp * factor), std::round(p.yp * factor));
}

inline const QPoint operator*(double factor, const QPoint &p)
{
    return QPoint(std::round(p.xp * factor), std::round(p.yp * factor));
}

inline const QPoint operator*(int factor, const QPoint &p)
{
    return QPoint(p.xp * factor, p.yp * factor);
}

inline const QPoint operator+(const QPoint &p)
{
    return p;
}

inline const QPoint operator-(const QPoint &p)
{
    return QPoint(-p.xp, -p.yp);
}

inline QPoint &QPoint::operator/=(double c)
{
    xp = std::round(xp / c);
    yp = std::round(yp / c);
    return *this;
}

inline const QPoint operator/(const QPoint &p, double c)
{
    return QPoint(std::round(p.xp / c), std::round(p.yp / c));
}

class QPointF {
public:
    QPointF();
    QPointF(const QPoint &p);
    QPointF(double xpos, double ypos);

    inline double manhattanLength() const;

    inline bool isNull() const;

    inline double x() const;
    inline double y() const;
    inline void setX(double x);
    inline void setY(double y);

    inline double &rx();
    inline double &ry();

    inline QPointF &operator+=(const QPointF &p);
    inline QPointF &operator-=(const QPointF &p);
    inline QPointF &operator*=(double c);
    inline QPointF &operator/=(double c);

    static inline double dotProduct(const QPointF &p1, const QPointF &p2)
    {
        return p1.xp * p2.xp + p1.yp * p2.yp;
    }

    friend inline bool operator==(const QPointF &, const QPointF &);
    friend inline bool operator!=(const QPointF &, const QPointF &);
    friend inline const QPointF operator+(const QPointF &, const QPointF &);
    friend inline const QPointF operator-(const QPointF &, const QPointF &);
    friend inline const QPointF operator*(double, const QPointF &);
    friend inline const QPointF operator*(const QPointF &, double);
    friend inline const QPointF operator+(const QPointF &);
    friend inline const QPointF operator-(const QPointF &);
    friend inline const QPointF operator/(const QPointF &, double);

    QPoint toPoint() const;

private:
    friend class QMatrix;
    friend class QTransform;

    double xp;
    double yp;
};

inline QPointF::QPointF() : xp(0), yp(0)
{}

inline QPointF::QPointF(double xpos, double ypos) : xp(xpos), yp(ypos)
{}

inline QPointF::QPointF(const QPoint &p) : xp(p.x()), yp(p.y())
{}

inline double QPointF::manhattanLength() const
{
    return std::abs(x()) + std::abs(y());
}

inline bool QPointF::isNull() const
{
    return qIsNull(xp) && qIsNull(yp);
}

inline double QPointF::x() const
{
    return xp;
}

inline double QPointF::y() const
{
    return yp;
}

inline void QPointF::setX(double xpos)
{
    xp = xpos;
}

inline void QPointF::setY(double ypos)
{
    yp = ypos;
}

inline double &QPointF::rx()
{
    return xp;
}

inline double &QPointF::ry()
{
    return yp;
}

inline QPointF &QPointF::operator+=(const QPointF &p)
{
    xp += p.xp;
    yp += p.yp;
    return *this;
}

inline QPointF &QPointF::operator-=(const QPointF &p)
{
    xp -= p.xp;
    yp -= p.yp;
    return *this;
}

inline QPointF &QPointF::operator*=(double c)
{
    xp *= c;
    yp *= c;
    return *this;
}

inline bool operator==(const QPointF &p1, const QPointF &p2)
{
    return ((!p1.xp || !p2.xp) ? qFuzzyIsNull(p1.xp - p2.xp) : qFuzzyCompare(p1.xp, p2.xp)) &&
           ((!p1.yp || !p2.yp) ? qFuzzyIsNull(p1.yp - p2.yp) : qFuzzyCompare(p1.yp, p2.yp));
}

inline bool operator!=(const QPointF &p1, const QPointF &p2)
{
    return !(p1 == p2);
}

inline const QPointF operator+(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.xp + p2.xp, p1.yp + p2.yp);
}

inline const QPointF operator-(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.xp - p2.xp, p1.yp - p2.yp);
}

inline const QPointF operator*(const QPointF &p, double c)
{
    return QPointF(p.xp * c, p.yp * c);
}

inline const QPointF operator*(double c, const QPointF &p)
{
    return QPointF(p.xp * c, p.yp * c);
}

inline const QPointF operator+(const QPointF &p)
{
    return p;
}

inline const QPointF operator-(const QPointF &p)
{
    return QPointF(-p.xp, -p.yp);
}

inline QPointF &QPointF::operator/=(double divisor)
{
    xp /= divisor;
    yp /= divisor;
    return *this;
}

inline const QPointF operator/(const QPointF &p, double divisor)
{
    return QPointF(p.xp / divisor, p.yp / divisor);
}

inline QPoint QPointF::toPoint() const
{
    return QPoint(std::round(xp), std::round(yp));
}
