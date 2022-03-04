#include "qvector3d.h"
#include "qrect.h"
#include "qmatrix4x4.h"
#include "qvector2d.h"
#include "qvector4d.h"


QVector3D::QVector3D(const QVector2D &vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = 0.0f;
}

QVector3D::QVector3D(const QVector2D &vector, float zpos)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = zpos;
}

QVector3D::QVector3D(const QVector4D &vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = vector.v[2];
}

QVector3D QVector3D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]) + double(v[2]) * double(v[2]);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = std::sqrt(len);
        return QVector3D(float(double(v[0]) / sqrtLen), float(double(v[1]) / sqrtLen), float(double(v[2]) / sqrtLen));
    } else {
        return QVector3D();
    }
}

void QVector3D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]) + double(v[2]) * double(v[2]);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len)) return;

    len = std::sqrt(len);

    v[0] = float(double(v[0]) / len);
    v[1] = float(double(v[1]) / len);
    v[2] = float(double(v[2]) / len);
}

float QVector3D::dotProduct(const QVector3D &v1, const QVector3D &v2)
{
    return v1.v[0] * v2.v[0] + v1.v[1] * v2.v[1] + v1.v[2] * v2.v[2];
}

QVector3D QVector3D::crossProduct(const QVector3D &v1, const QVector3D &v2)
{
    return QVector3D(v1.v[1] * v2.v[2] - v1.v[2] * v2.v[1], v1.v[2] * v2.v[0] - v1.v[0] * v2.v[2], v1.v[0] * v2.v[1] - v1.v[1] * v2.v[0]);
}

QVector3D QVector3D::normal(const QVector3D &v1, const QVector3D &v2)
{
    return crossProduct(v1, v2).normalized();
}

QVector3D QVector3D::normal(const QVector3D &v1, const QVector3D &v2, const QVector3D &v3)
{
    return crossProduct((v2 - v1), (v3 - v1)).normalized();
}

QVector3D QVector3D::project(const QMatrix4x4 &modelView, const QMatrix4x4 &projection, const QRect &viewport) const
{
    QVector4D tmp(*this, 1.0f);
    tmp = projection * modelView * tmp;
    if (qFuzzyIsNull(tmp.w())) tmp.setW(1.0f);
    tmp /= tmp.w();

    tmp = tmp * 0.5f + QVector4D(0.5f, 0.5f, 0.5f, 0.5f);
    tmp.setX(tmp.x() * viewport.width() + viewport.x());
    tmp.setY(tmp.y() * viewport.height() + viewport.y());

    return tmp.toVector3D();
}

QVector3D QVector3D::unproject(const QMatrix4x4 &modelView, const QMatrix4x4 &projection, const QRect &viewport) const
{
    QMatrix4x4 inverse = QMatrix4x4(projection * modelView).inverted();

    QVector4D tmp(*this, 1.0f);
    tmp.setX((tmp.x() - float(viewport.x())) / float(viewport.width()));
    tmp.setY((tmp.y() - float(viewport.y())) / float(viewport.height()));
    tmp = tmp * 2.0f - QVector4D(1.0f, 1.0f, 1.0f, 1.0f);

    QVector4D obj = inverse * tmp;
    if (qFuzzyIsNull(obj.w())) obj.setW(1.0f);
    obj /= obj.w();
    return obj.toVector3D();
}

float QVector3D::distanceToPoint(const QVector3D &point) const
{
    return (*this - point).length();
}

float QVector3D::distanceToPlane(const QVector3D &plane, const QVector3D &normal) const
{
    return dotProduct(*this - plane, normal);
}

float QVector3D::distanceToPlane(const QVector3D &plane1, const QVector3D &plane2, const QVector3D &plane3) const
{
    QVector3D n = normal(plane2 - plane1, plane3 - plane1);
    return dotProduct(*this - plane1, n);
}

float QVector3D::distanceToLine(const QVector3D &point, const QVector3D &direction) const
{
    if (direction.isNull()) return (*this - point).length();
    QVector3D p = point + dotProduct(*this - point, direction) * direction;
    return (*this - p).length();
}

QVector2D QVector3D::toVector2D() const
{
    return QVector2D(v[0], v[1]);
}

QVector4D QVector3D::toVector4D() const
{
    return QVector4D(v[0], v[1], v[2], 0.0f);
}

float QVector3D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) + double(v[1]) * double(v[1]) + double(v[2]) * double(v[2]);
    return float(std::sqrt(len));
}

float QVector3D::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}
