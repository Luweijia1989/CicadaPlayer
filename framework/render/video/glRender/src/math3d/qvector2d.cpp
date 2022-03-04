#include "qvector2d.h"
#include "qvector3d.h"
#include "qvector4d.h"

QVector2D::QVector2D(const QVector3D &vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
}

QVector2D::QVector2D(const QVector4D &vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
}

float QVector2D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]);
    return float(std::sqrt(len));
}

float QVector2D::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1];
}

QVector2D QVector2D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = std::sqrt(len);
        return QVector2D(float(double(v[0]) / sqrtLen), float(double(v[1]) / sqrtLen));
    } else {
        return QVector2D();
    }
}

void QVector2D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len)) return;

    len = std::sqrt(len);

    v[0] = float(double(v[0]) / len);
    v[1] = float(double(v[1]) / len);
}

float QVector2D::distanceToPoint(const QVector2D &point) const
{
    return (*this - point).length();
}

float QVector2D::distanceToLine(const QVector2D &point, const QVector2D &direction) const
{
    if (direction.isNull()) return (*this - point).length();
    QVector2D p = point + dotProduct(*this - point, direction) * direction;
    return (*this - p).length();
}

float QVector2D::dotProduct(const QVector2D &v1, const QVector2D &v2)
{
    return v1.v[0] * v2.v[0] + v1.v[1] * v2.v[1];
}


QVector3D QVector2D::toVector3D() const
{
    return QVector3D(v[0], v[1], 0.0f);
}

QVector4D QVector2D::toVector4D() const
{
    return QVector4D(v[0], v[1], 0.0f, 0.0f);
}
