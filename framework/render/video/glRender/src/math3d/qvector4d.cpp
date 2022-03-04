#include "qvector4d.h"
#include "qvector2d.h"
#include "qvector3d.h"


QVector4D::QVector4D(const QVector2D &vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = 0.0f;
    v[3] = 0.0f;
}

QVector4D::QVector4D(const QVector2D &vector, float zpos, float wpos)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = zpos;
    v[3] = wpos;
}

QVector4D::QVector4D(const QVector3D &vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = vector.v[2];
    v[3] = 0.0f;
}

QVector4D::QVector4D(const QVector3D &vector, float wpos)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = vector.v[2];
    v[3] = wpos;
}

float QVector4D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]) + double(v[2]) * double(v[2]) + double(v[3]) * double(v[3]);
    return float(std::sqrt(len));
}

float QVector4D::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
}

QVector4D QVector4D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]) + double(v[2]) * double(v[2]) + double(v[3]) * double(v[3]);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = std::sqrt(len);
        return QVector4D(float(double(v[0]) / sqrtLen), float(double(v[1]) / sqrtLen), float(double(v[2]) / sqrtLen),
                         float(double(v[3]) / sqrtLen));
    } else {
        return QVector4D();
    }
}

void QVector4D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]) + double(v[2]) * double(v[2]) + double(v[3]) * double(v[3]);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len)) return;

    len = std::sqrt(len);

    v[0] = float(double(v[0]) / len);
    v[1] = float(double(v[1]) / len);
    v[2] = float(double(v[2]) / len);
    v[3] = float(double(v[3]) / len);
}

float QVector4D::dotProduct(const QVector4D &v1, const QVector4D &v2)
{
    return v1.v[0] * v2.v[0] + v1.v[1] * v2.v[1] + v1.v[2] * v2.v[2] + v1.v[3] * v2.v[3];
}

QVector2D QVector4D::toVector2D() const
{
    return QVector2D(v[0], v[1]);
}

QVector2D QVector4D::toVector2DAffine() const
{
    if (qIsNull(v[3])) return QVector2D();
    return QVector2D(v[0] / v[3], v[1] / v[3]);
}

QVector3D QVector4D::toVector3D() const
{
    return QVector3D(v[0], v[1], v[2]);
}

QVector3D QVector4D::toVector3DAffine() const
{
    if (qIsNull(v[3])) return QVector3D();
    return QVector3D(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
}