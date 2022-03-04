#ifndef QVECTOR4D_H
#define QVECTOR4D_H

#include "qpoint.h"
#include <cassert>

class QMatrix4x4;
class QVector2D;
class QVector3D;

class QVector4D {
public:
    QVector4D();
    QVector4D(float xpos, float ypos, float zpos, float wpos);
    explicit QVector4D(const QPoint &point);
    explicit QVector4D(const QPointF &point);

    QVector4D(const QVector2D &vector);
    QVector4D(const QVector2D &vector, float zpos, float wpos);
    QVector4D(const QVector3D &vector);
    QVector4D(const QVector3D &vector, float wpos);

    bool isNull() const;

    float x() const;
    float y() const;
    float z() const;
    float w() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setW(float w);

    float &operator[](int i);
    float operator[](int i) const;

    float length() const;
    float lengthSquared() const;//In Qt 6 convert to inline and constexpr

    QVector4D normalized() const;
    void normalize();

    QVector4D &operator+=(const QVector4D &vector);
    QVector4D &operator-=(const QVector4D &vector);
    QVector4D &operator*=(float factor);
    QVector4D &operator*=(const QVector4D &vector);
    QVector4D &operator/=(float divisor);
    inline QVector4D &operator/=(const QVector4D &vector);

    static float dotProduct(const QVector4D &v1, const QVector4D &v2);//In Qt 6 convert to inline and constexpr

    friend inline bool operator==(const QVector4D &v1, const QVector4D &v2);
    friend inline bool operator!=(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator+(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator-(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator*(float factor, const QVector4D &vector);
    friend inline const QVector4D operator*(const QVector4D &vector, float factor);
    friend inline const QVector4D operator*(const QVector4D &v1, const QVector4D &v2);
    friend inline const QVector4D operator-(const QVector4D &vector);
    friend inline const QVector4D operator/(const QVector4D &vector, float divisor);
    friend inline const QVector4D operator/(const QVector4D &vector, const QVector4D &divisor);

    friend inline bool qFuzzyCompare(const QVector4D &v1, const QVector4D &v2);

    QVector2D toVector2D() const;
    QVector2D toVector2DAffine() const;
    QVector3D toVector3D() const;
    QVector3D toVector3DAffine() const;

    QPoint toPoint() const;
    QPointF toPointF() const;

private:
    float v[4];

    friend class QVector2D;
    friend class QVector3D;
    friend QVector4D operator*(const QVector4D &vector, const QMatrix4x4 &matrix);
    friend QVector4D operator*(const QMatrix4x4 &matrix, const QVector4D &vector);
};

inline QVector4D::QVector4D() : v{0.0f, 0.0f, 0.0f, 0.0f}
{}

inline QVector4D::QVector4D(float xpos, float ypos, float zpos, float wpos) : v{xpos, ypos, zpos, wpos}
{}

inline QVector4D::QVector4D(const QPoint &point) : v{float(point.x()), float(point.y()), 0.0f, 0.0f}
{}

inline QVector4D::QVector4D(const QPointF &point) : v{float(point.x()), float(point.y()), 0.0f, 0.0f}
{}

inline bool QVector4D::isNull() const
{
    return qIsNull(v[0]) && qIsNull(v[1]) && qIsNull(v[2]) && qIsNull(v[3]);
}

inline float QVector4D::x() const
{
    return v[0];
}
inline float QVector4D::y() const
{
    return v[1];
}
inline float QVector4D::z() const
{
    return v[2];
}
inline float QVector4D::w() const
{
    return v[3];
}

inline void QVector4D::setX(float aX)
{
    v[0] = aX;
}
inline void QVector4D::setY(float aY)
{
    v[1] = aY;
}
inline void QVector4D::setZ(float aZ)
{
    v[2] = aZ;
}
inline void QVector4D::setW(float aW)
{
    v[3] = aW;
}

inline float &QVector4D::operator[](int i)
{
    assert(unsigned int(i) < 4u);
    return v[i];
}

inline float QVector4D::operator[](int i) const
{
    assert(unsigned int(i) < 4u);
    return v[i];
}

inline QVector4D &QVector4D::operator+=(const QVector4D &vector)
{
    v[0] += vector.v[0];
    v[1] += vector.v[1];
    v[2] += vector.v[2];
    v[3] += vector.v[3];
    return *this;
}

inline QVector4D &QVector4D::operator-=(const QVector4D &vector)
{
    v[0] -= vector.v[0];
    v[1] -= vector.v[1];
    v[2] -= vector.v[2];
    v[3] -= vector.v[3];
    return *this;
}

inline QVector4D &QVector4D::operator*=(float factor)
{
    v[0] *= factor;
    v[1] *= factor;
    v[2] *= factor;
    v[3] *= factor;
    return *this;
}

inline QVector4D &QVector4D::operator*=(const QVector4D &vector)
{
    v[0] *= vector.v[0];
    v[1] *= vector.v[1];
    v[2] *= vector.v[2];
    v[3] *= vector.v[3];
    return *this;
}

inline QVector4D &QVector4D::operator/=(float divisor)
{
    v[0] /= divisor;
    v[1] /= divisor;
    v[2] /= divisor;
    v[3] /= divisor;
    return *this;
}

inline QVector4D &QVector4D::operator/=(const QVector4D &vector)
{
    v[0] /= vector.v[0];
    v[1] /= vector.v[1];
    v[2] /= vector.v[2];
    v[3] /= vector.v[3];
    return *this;
}

inline bool operator==(const QVector4D &v1, const QVector4D &v2)
{
    return v1.v[0] == v2.v[0] && v1.v[1] == v2.v[1] && v1.v[2] == v2.v[2] && v1.v[3] == v2.v[3];
}

inline bool operator!=(const QVector4D &v1, const QVector4D &v2)
{
    return v1.v[0] != v2.v[0] || v1.v[1] != v2.v[1] || v1.v[2] != v2.v[2] || v1.v[3] != v2.v[3];
}

inline const QVector4D operator+(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.v[0] + v2.v[0], v1.v[1] + v2.v[1], v1.v[2] + v2.v[2], v1.v[3] + v2.v[3]);
}

inline const QVector4D operator-(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.v[0] - v2.v[0], v1.v[1] - v2.v[1], v1.v[2] - v2.v[2], v1.v[3] - v2.v[3]);
}

inline const QVector4D operator*(float factor, const QVector4D &vector)
{
    return QVector4D(vector.v[0] * factor, vector.v[1] * factor, vector.v[2] * factor, vector.v[3] * factor);
}

inline const QVector4D operator*(const QVector4D &vector, float factor)
{
    return QVector4D(vector.v[0] * factor, vector.v[1] * factor, vector.v[2] * factor, vector.v[3] * factor);
}

inline const QVector4D operator*(const QVector4D &v1, const QVector4D &v2)
{
    return QVector4D(v1.v[0] * v2.v[0], v1.v[1] * v2.v[1], v1.v[2] * v2.v[2], v1.v[3] * v2.v[3]);
}

inline const QVector4D operator-(const QVector4D &vector)
{
    return QVector4D(-vector.v[0], -vector.v[1], -vector.v[2], -vector.v[3]);
}

inline const QVector4D operator/(const QVector4D &vector, float divisor)
{
    return QVector4D(vector.v[0] / divisor, vector.v[1] / divisor, vector.v[2] / divisor, vector.v[3] / divisor);
}

inline const QVector4D operator/(const QVector4D &vector, const QVector4D &divisor)
{
    return QVector4D(vector.v[0] / divisor.v[0], vector.v[1] / divisor.v[1], vector.v[2] / divisor.v[2], vector.v[3] / divisor.v[3]);
}

inline bool qFuzzyCompare(const QVector4D &v1, const QVector4D &v2)
{
    return qFuzzyCompare(v1.v[0], v2.v[0]) && qFuzzyCompare(v1.v[1], v2.v[1]) && qFuzzyCompare(v1.v[2], v2.v[2]) &&
           qFuzzyCompare(v1.v[3], v2.v[3]);
}

inline QPoint QVector4D::toPoint() const
{
    return QPoint(std::round(v[0]), std::round(v[1]));
}

inline QPointF QVector4D::toPointF() const
{
    return QPointF(double(v[0]), double(v[1]));
}

#endif
