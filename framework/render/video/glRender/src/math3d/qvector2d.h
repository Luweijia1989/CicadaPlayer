#ifndef QVECTOR2D_H
#define QVECTOR2D_H

#include "qpoint.h"
#include <cassert>

class QVector3D;
class QVector4D;

class QVector2D {
public:
    QVector2D();
    QVector2D(float xpos, float ypos);
    explicit QVector2D(const QPoint &point);
    explicit QVector2D(const QPointF &point);
    explicit QVector2D(const QVector3D &vector);
    explicit QVector2D(const QVector4D &vector);

    bool isNull() const;

    float x() const;
    float y() const;

    void setX(float x);
    void setY(float y);

    float &operator[](int i);
    float operator[](int i) const;

    float length() const;
    float lengthSquared() const;//In Qt 6 convert to inline and constexpr

    QVector2D normalized() const;
    void normalize();

    float distanceToPoint(const QVector2D &point) const;
    float distanceToLine(const QVector2D &point, const QVector2D &direction) const;

    QVector2D &operator+=(const QVector2D &vector);
    QVector2D &operator-=(const QVector2D &vector);
    QVector2D &operator*=(float factor);
    QVector2D &operator*=(const QVector2D &vector);
    QVector2D &operator/=(float divisor);
    inline QVector2D &operator/=(const QVector2D &vector);

    static float dotProduct(const QVector2D &v1, const QVector2D &v2);//In Qt 6 convert to inline and constexpr

    friend inline bool operator==(const QVector2D &v1, const QVector2D &v2);
    friend inline bool operator!=(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator+(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator-(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator*(float factor, const QVector2D &vector);
    friend inline const QVector2D operator*(const QVector2D &vector, float factor);
    friend inline const QVector2D operator*(const QVector2D &v1, const QVector2D &v2);
    friend inline const QVector2D operator-(const QVector2D &vector);
    friend inline const QVector2D operator/(const QVector2D &vector, float divisor);
    friend inline const QVector2D operator/(const QVector2D &vector, const QVector2D &divisor);

    friend inline bool qFuzzyCompare(const QVector2D &v1, const QVector2D &v2);

    QVector3D toVector3D() const;
    QVector4D toVector4D() const;

    QPoint toPoint() const;
    QPointF toPointF() const;

private:
    float v[2];

    friend class QVector3D;
    friend class QVector4D;
};

inline QVector2D::QVector2D() : v{0.0f, 0.0f}
{}

inline QVector2D::QVector2D(float xpos, float ypos) : v{xpos, ypos}
{}

inline QVector2D::QVector2D(const QPoint &point) : v{float(point.x()), float(point.y())}
{}

inline QVector2D::QVector2D(const QPointF &point) : v{float(point.x()), float(point.y())}
{}

inline bool QVector2D::isNull() const
{
    return qIsNull(v[0]) && qIsNull(v[1]);
}

inline float QVector2D::x() const
{
    return v[0];
}
inline float QVector2D::y() const
{
    return v[1];
}

inline void QVector2D::setX(float aX)
{
    v[0] = aX;
}
inline void QVector2D::setY(float aY)
{
    v[1] = aY;
}

inline float &QVector2D::operator[](int i)
{
    assert(unsigned int(i) < 2u);
    return v[i];
}

inline float QVector2D::operator[](int i) const
{
    assert(unsigned int(i) < 2u);
    return v[i];
}

inline QVector2D &QVector2D::operator+=(const QVector2D &vector)
{
    v[0] += vector.v[0];
    v[1] += vector.v[1];
    return *this;
}

inline QVector2D &QVector2D::operator-=(const QVector2D &vector)
{
    v[0] -= vector.v[0];
    v[1] -= vector.v[1];
    return *this;
}

inline QVector2D &QVector2D::operator*=(float factor)
{
    v[0] *= factor;
    v[1] *= factor;
    return *this;
}

inline QVector2D &QVector2D::operator*=(const QVector2D &vector)
{
    v[0] *= vector.v[0];
    v[1] *= vector.v[1];
    return *this;
}

inline QVector2D &QVector2D::operator/=(float divisor)
{
    v[0] /= divisor;
    v[1] /= divisor;
    return *this;
}

inline QVector2D &QVector2D::operator/=(const QVector2D &vector)
{
    v[0] /= vector.v[0];
    v[1] /= vector.v[1];
    return *this;
}

inline bool operator==(const QVector2D &v1, const QVector2D &v2)
{
    return v1.v[0] == v2.v[0] && v1.v[1] == v2.v[1];
}

inline bool operator!=(const QVector2D &v1, const QVector2D &v2)
{
    return v1.v[0] != v2.v[0] || v1.v[1] != v2.v[1];
}

inline const QVector2D operator+(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.v[0] + v2.v[0], v1.v[1] + v2.v[1]);
}

inline const QVector2D operator-(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.v[0] - v2.v[0], v1.v[1] - v2.v[1]);
}

inline const QVector2D operator*(float factor, const QVector2D &vector)
{
    return QVector2D(vector.v[0] * factor, vector.v[1] * factor);
}

inline const QVector2D operator*(const QVector2D &vector, float factor)
{
    return QVector2D(vector.v[0] * factor, vector.v[1] * factor);
}

inline const QVector2D operator*(const QVector2D &v1, const QVector2D &v2)
{
    return QVector2D(v1.v[0] * v2.v[0], v1.v[1] * v2.v[1]);
}

inline const QVector2D operator-(const QVector2D &vector)
{
    return QVector2D(-vector.v[0], -vector.v[1]);
}

inline const QVector2D operator/(const QVector2D &vector, float divisor)
{
    return QVector2D(vector.v[0] / divisor, vector.v[1] / divisor);
}

inline const QVector2D operator/(const QVector2D &vector, const QVector2D &divisor)
{
    return QVector2D(vector.v[0] / divisor.v[0], vector.v[1] / divisor.v[1]);
}

inline bool qFuzzyCompare(const QVector2D &v1, const QVector2D &v2)
{
    return qFuzzyCompare(v1.v[0], v2.v[0]) && qFuzzyCompare(v1.v[1], v2.v[1]);
}

inline QPoint QVector2D::toPoint() const
{
    return QPoint(std::round(v[0]), std::round(v[1]));
}

inline QPointF QVector2D::toPointF() const
{
    return QPointF(double(v[0]), double(v[1]));
}

#endif
