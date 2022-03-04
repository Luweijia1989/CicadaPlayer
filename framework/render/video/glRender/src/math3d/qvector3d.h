#ifndef QVECTOR3D_H
#define QVECTOR3D_H

#include "qpoint.h"
#include <cassert>

class QMatrix4x4;
class QVector2D;
class QVector4D;
class QRect;

class QVector3D {
public:
    QVector3D();
    QVector3D(float xpos, float ypos, float zpos) : v{xpos, ypos, zpos}
    {}

    explicit QVector3D(const QPoint &point);
    explicit QVector3D(const QPointF &point);

    QVector3D(const QVector2D &vector);
    QVector3D(const QVector2D &vector, float zpos);
    explicit QVector3D(const QVector4D &vector);

    bool isNull() const;

    float x() const;
    float y() const;
    float z() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);

    float &operator[](int i);
    float operator[](int i) const;

    float length() const;
    float lengthSquared() const;

    QVector3D normalized() const;
    void normalize();

    QVector3D &operator+=(const QVector3D &vector);
    QVector3D &operator-=(const QVector3D &vector);
    QVector3D &operator*=(float factor);
    QVector3D &operator*=(const QVector3D &vector);
    QVector3D &operator/=(float divisor);
    inline QVector3D &operator/=(const QVector3D &vector);

    static float dotProduct(const QVector3D &v1, const QVector3D &v2);      //In Qt 6 convert to inline and constexpr
    static QVector3D crossProduct(const QVector3D &v1, const QVector3D &v2);//in Qt 6 convert to inline and constexpr

    static QVector3D normal(const QVector3D &v1, const QVector3D &v2);
    static QVector3D normal(const QVector3D &v1, const QVector3D &v2, const QVector3D &v3);

    QVector3D project(const QMatrix4x4 &modelView, const QMatrix4x4 &projection, const QRect &viewport) const;
    QVector3D unproject(const QMatrix4x4 &modelView, const QMatrix4x4 &projection, const QRect &viewport) const;

    float distanceToPoint(const QVector3D &point) const;
    float distanceToPlane(const QVector3D &plane, const QVector3D &normal) const;
    float distanceToPlane(const QVector3D &plane1, const QVector3D &plane2, const QVector3D &plane3) const;
    float distanceToLine(const QVector3D &point, const QVector3D &direction) const;

    friend inline bool operator==(const QVector3D &v1, const QVector3D &v2);
    friend inline bool operator!=(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator+(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator-(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator*(float factor, const QVector3D &vector);
    friend inline const QVector3D operator*(const QVector3D &vector, float factor);
    friend const QVector3D operator*(const QVector3D &v1, const QVector3D &v2);
    friend inline const QVector3D operator-(const QVector3D &vector);
    friend inline const QVector3D operator/(const QVector3D &vector, float divisor);
    friend inline const QVector3D operator/(const QVector3D &vector, const QVector3D &divisor);

    friend inline bool qFuzzyCompare(const QVector3D &v1, const QVector3D &v2);

    QVector2D toVector2D() const;
    QVector4D toVector4D() const;

    QPoint toPoint() const;
    QPointF toPointF() const;

private:
    float v[3];

    friend class QVector2D;
    friend class QVector4D;
    friend QVector3D operator*(const QVector3D &vector, const QMatrix4x4 &matrix);
    friend QVector3D operator*(const QMatrix4x4 &matrix, const QVector3D &vector);
};


inline QVector3D::QVector3D() : v{0.0f, 0.0f, 0.0f}
{}

inline QVector3D::QVector3D(const QPoint &point) : v{float(point.x()), float(point.y()), float(0.0f)}
{}

inline QVector3D::QVector3D(const QPointF &point) : v{float(point.x()), float(point.y()), 0.0f}
{}

inline bool QVector3D::isNull() const
{
    return qIsNull(v[0]) && qIsNull(v[1]) && qIsNull(v[2]);
}

inline float QVector3D::x() const
{
    return v[0];
}
inline float QVector3D::y() const
{
    return v[1];
}
inline float QVector3D::z() const
{
    return v[2];
}

inline void QVector3D::setX(float aX)
{
    v[0] = aX;
}
inline void QVector3D::setY(float aY)
{
    v[1] = aY;
}
inline void QVector3D::setZ(float aZ)
{
    v[2] = aZ;
}

inline float &QVector3D::operator[](int i)
{
    assert(unsigned int(i) < 3u);
    return v[i];
}

inline float QVector3D::operator[](int i) const
{
    assert(unsigned int(i) < 3u);
    return v[i];
}

inline QVector3D &QVector3D::operator+=(const QVector3D &vector)
{
    v[0] += vector.v[0];
    v[1] += vector.v[1];
    v[2] += vector.v[2];
    return *this;
}

inline QVector3D &QVector3D::operator-=(const QVector3D &vector)
{
    v[0] -= vector.v[0];
    v[1] -= vector.v[1];
    v[2] -= vector.v[2];
    return *this;
}

inline QVector3D &QVector3D::operator*=(float factor)
{
    v[0] *= factor;
    v[1] *= factor;
    v[2] *= factor;
    return *this;
}

inline QVector3D &QVector3D::operator*=(const QVector3D &vector)
{
    v[0] *= vector.v[0];
    v[1] *= vector.v[1];
    v[2] *= vector.v[2];
    return *this;
}

inline QVector3D &QVector3D::operator/=(float divisor)
{
    v[0] /= divisor;
    v[1] /= divisor;
    v[2] /= divisor;
    return *this;
}

inline QVector3D &QVector3D::operator/=(const QVector3D &vector)
{
    v[0] /= vector.v[0];
    v[1] /= vector.v[1];
    v[2] /= vector.v[2];
    return *this;
}

inline bool operator==(const QVector3D &v1, const QVector3D &v2)
{
    return v1.v[0] == v2.v[0] && v1.v[1] == v2.v[1] && v1.v[2] == v2.v[2];
}

inline bool operator!=(const QVector3D &v1, const QVector3D &v2)
{
    return v1.v[0] != v2.v[0] || v1.v[1] != v2.v[1] || v1.v[2] != v2.v[2];
}

inline const QVector3D operator+(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.v[0] + v2.v[0], v1.v[1] + v2.v[1], v1.v[2] + v2.v[2]);
}

inline const QVector3D operator-(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.v[0] - v2.v[0], v1.v[1] - v2.v[1], v1.v[2] - v2.v[2]);
}

inline const QVector3D operator*(float factor, const QVector3D &vector)
{
    return QVector3D(vector.v[0] * factor, vector.v[1] * factor, vector.v[2] * factor);
}

inline const QVector3D operator*(const QVector3D &vector, float factor)
{
    return QVector3D(vector.v[0] * factor, vector.v[1] * factor, vector.v[2] * factor);
}

inline const QVector3D operator*(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.v[0] * v2.v[0], v1.v[1] * v2.v[1], v1.v[2] * v2.v[2]);
}

inline const QVector3D operator-(const QVector3D &vector)
{
    return QVector3D(-vector.v[0], -vector.v[1], -vector.v[2]);
}

inline const QVector3D operator/(const QVector3D &vector, float divisor)
{
    return QVector3D(vector.v[0] / divisor, vector.v[1] / divisor, vector.v[2] / divisor);
}

inline const QVector3D operator/(const QVector3D &vector, const QVector3D &divisor)
{
    return QVector3D(vector.v[0] / divisor.v[0], vector.v[1] / divisor.v[1], vector.v[2] / divisor.v[2]);
}

inline bool qFuzzyCompare(const QVector3D &v1, const QVector3D &v2)
{
    return qFuzzyCompare(v1.v[0], v2.v[0]) && qFuzzyCompare(v1.v[1], v2.v[1]) && qFuzzyCompare(v1.v[2], v2.v[2]);
}

inline QPoint QVector3D::toPoint() const
{
    return QPoint(std::round(v[0]), std::round(v[1]));
}

inline QPointF QVector3D::toPointF() const
{
    return QPointF(double(v[0]), double(v[1]));
}
#endif
