#include "qquaternion.h"
#include <cmath>


float QQuaternion::length() const
{
    return std::sqrt(xp * xp + yp * yp + zp * zp + wp * wp);
}

float QQuaternion::lengthSquared() const
{
    return xp * xp + yp * yp + zp * zp + wp * wp;
}

QQuaternion QQuaternion::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) + double(yp) * double(yp) + double(zp) * double(zp) + double(wp) * double(wp);
    if (qFuzzyIsNull(len - 1.0f))
        return *this;
    else if (!qFuzzyIsNull(len))
        return *this / std::sqrt(len);
    else
        return QQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
}

void QQuaternion::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) + double(yp) * double(yp) + double(zp) * double(zp) + double(wp) * double(wp);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len)) return;

    len = std::sqrt(len);

    xp /= len;
    yp /= len;
    zp /= len;
    wp /= len;
}

QVector3D QQuaternion::rotatedVector(const QVector3D &vector) const
{
    return (*this * QQuaternion(0, vector) * conjugated()).vector();
}

QQuaternion QQuaternion::fromAxisAndAngle(const QVector3D &axis, float angle)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q56
    // We normalize the result just in case the values are close
    // to zero, as suggested in the above FAQ.
    float a = qDegreesToRadians(angle / 2.0f);
    float s = std::sin(a);
    float c = std::cos(a);
    QVector3D ax = axis.normalized();
    return QQuaternion(c, ax.x() * s, ax.y() * s, ax.z() * s).normalized();
}

void QQuaternion::getAxisAndAngle(float *x, float *y, float *z, float *angle) const
{
    assert(x && y && z && angle);

    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    float length = xp * xp + yp * yp + zp * zp;
    if (!qFuzzyIsNull(length)) {
        *x = xp;
        *y = yp;
        *z = zp;
        if (!qFuzzyIsNull(length - 1.0f)) {
            length = std::sqrt(length);
            *x /= length;
            *y /= length;
            *z /= length;
        }
        *angle = 2.0f * std::acos(wp);
    } else {
        // angle is 0 (mod 2*pi), so any axis will fit
        *x = *y = *z = *angle = 0.0f;
    }

    *angle = qRadiansToDegrees(*angle);
}

QQuaternion QQuaternion::fromAxisAndAngle(float x, float y, float z, float angle)
{
    float length = std::sqrt(x * x + y * y + z * z);
    if (!qFuzzyIsNull(length - 1.0f) && !qFuzzyIsNull(length)) {
        x /= length;
        y /= length;
        z /= length;
    }
    float a = qDegreesToRadians(angle / 2.0f);
    float s = std::sin(a);
    float c = std::cos(a);
    return QQuaternion(c, x * s, y * s, z * s).normalized();
}

void QQuaternion::getEulerAngles(float *pitch, float *yaw, float *roll) const
{
    assert(pitch && yaw && roll);

    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q37

    float xx = xp * xp;
    float xy = xp * yp;
    float xz = xp * zp;
    float xw = xp * wp;
    float yy = yp * yp;
    float yz = yp * zp;
    float yw = yp * wp;
    float zz = zp * zp;
    float zw = zp * wp;

    const float lengthSquared = xx + yy + zz + wp * wp;
    if (!qFuzzyIsNull(lengthSquared - 1.0f) && !qFuzzyIsNull(lengthSquared)) {
        xx /= lengthSquared;
        xy /= lengthSquared;// same as (xp / length) * (yp / length)
        xz /= lengthSquared;
        xw /= lengthSquared;
        yy /= lengthSquared;
        yz /= lengthSquared;
        yw /= lengthSquared;
        zz /= lengthSquared;
        zw /= lengthSquared;
    }

    *pitch = std::asin(-2.0f * (yz - xw));
    if (*pitch < M_PI_2) {
        if (*pitch > -M_PI_2) {
            *yaw = std::atan2(2.0f * (xz + yw), 1.0f - 2.0f * (xx + yy));
            *roll = std::atan2(2.0f * (xy + zw), 1.0f - 2.0f * (xx + zz));
        } else {
            // not a unique solution
            *roll = 0.0f;
            *yaw = -std::atan2(-2.0f * (xy - zw), 1.0f - 2.0f * (yy + zz));
        }
    } else {
        // not a unique solution
        *roll = 0.0f;
        *yaw = std::atan2(-2.0f * (xy - zw), 1.0f - 2.0f * (yy + zz));
    }

    *pitch = qRadiansToDegrees(*pitch);
    *yaw = qRadiansToDegrees(*yaw);
    *roll = qRadiansToDegrees(*roll);
}

QQuaternion QQuaternion::fromEulerAngles(float pitch, float yaw, float roll)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q60

    pitch = qDegreesToRadians(pitch);
    yaw = qDegreesToRadians(yaw);
    roll = qDegreesToRadians(roll);

    pitch *= 0.5f;
    yaw *= 0.5f;
    roll *= 0.5f;

    const float c1 = std::cos(yaw);
    const float s1 = std::sin(yaw);
    const float c2 = std::cos(roll);
    const float s2 = std::sin(roll);
    const float c3 = std::cos(pitch);
    const float s3 = std::sin(pitch);
    const float c1c2 = c1 * c2;
    const float s1s2 = s1 * s2;

    const float w = c1c2 * c3 + s1s2 * s3;
    const float x = c1c2 * s3 + s1s2 * c3;
    const float y = s1 * c2 * c3 - c1 * s2 * s3;
    const float z = c1 * s2 * c3 - s1 * c2 * s3;

    return QQuaternion(w, x, y, z);
}

QMatrix3x3 QQuaternion::toRotationMatrix() const
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q54

    QMatrix3x3 rot3x3(YPP::Uninitialized);

    const float f2x = xp + xp;
    const float f2y = yp + yp;
    const float f2z = zp + zp;
    const float f2xw = f2x * wp;
    const float f2yw = f2y * wp;
    const float f2zw = f2z * wp;
    const float f2xx = f2x * xp;
    const float f2xy = f2x * yp;
    const float f2xz = f2x * zp;
    const float f2yy = f2y * yp;
    const float f2yz = f2y * zp;
    const float f2zz = f2z * zp;

    rot3x3(0, 0) = 1.0f - (f2yy + f2zz);
    rot3x3(0, 1) = f2xy - f2zw;
    rot3x3(0, 2) = f2xz + f2yw;
    rot3x3(1, 0) = f2xy + f2zw;
    rot3x3(1, 1) = 1.0f - (f2xx + f2zz);
    rot3x3(1, 2) = f2yz - f2xw;
    rot3x3(2, 0) = f2xz - f2yw;
    rot3x3(2, 1) = f2yz + f2xw;
    rot3x3(2, 2) = 1.0f - (f2xx + f2yy);

    return rot3x3;
}

QQuaternion QQuaternion::fromRotationMatrix(const QMatrix3x3 &rot3x3)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q55

    float scalar;
    float axis[3];

    const float trace = rot3x3(0, 0) + rot3x3(1, 1) + rot3x3(2, 2);
    if (trace > 0.00000001f) {
        const float s = 2.0f * std::sqrt(trace + 1.0f);
        scalar = 0.25f * s;
        axis[0] = (rot3x3(2, 1) - rot3x3(1, 2)) / s;
        axis[1] = (rot3x3(0, 2) - rot3x3(2, 0)) / s;
        axis[2] = (rot3x3(1, 0) - rot3x3(0, 1)) / s;
    } else {
        static int s_next[3] = {1, 2, 0};
        int i = 0;
        if (rot3x3(1, 1) > rot3x3(0, 0)) i = 1;
        if (rot3x3(2, 2) > rot3x3(i, i)) i = 2;
        int j = s_next[i];
        int k = s_next[j];

        const float s = 2.0f * std::sqrt(rot3x3(i, i) - rot3x3(j, j) - rot3x3(k, k) + 1.0f);
        axis[i] = 0.25f * s;
        scalar = (rot3x3(k, j) - rot3x3(j, k)) / s;
        axis[j] = (rot3x3(j, i) + rot3x3(i, j)) / s;
        axis[k] = (rot3x3(k, i) + rot3x3(i, k)) / s;
    }

    return QQuaternion(scalar, axis[0], axis[1], axis[2]);
}

void QQuaternion::getAxes(QVector3D *xAxis, QVector3D *yAxis, QVector3D *zAxis) const
{
    assert(xAxis && yAxis && zAxis);

    const QMatrix3x3 rot3x3(toRotationMatrix());

    *xAxis = QVector3D(rot3x3(0, 0), rot3x3(1, 0), rot3x3(2, 0));
    *yAxis = QVector3D(rot3x3(0, 1), rot3x3(1, 1), rot3x3(2, 1));
    *zAxis = QVector3D(rot3x3(0, 2), rot3x3(1, 2), rot3x3(2, 2));
}

QQuaternion QQuaternion::fromAxes(const QVector3D &xAxis, const QVector3D &yAxis, const QVector3D &zAxis)
{
    QMatrix3x3 rot3x3(YPP::Uninitialized);
    rot3x3(0, 0) = xAxis.x();
    rot3x3(1, 0) = xAxis.y();
    rot3x3(2, 0) = xAxis.z();
    rot3x3(0, 1) = yAxis.x();
    rot3x3(1, 1) = yAxis.y();
    rot3x3(2, 1) = yAxis.z();
    rot3x3(0, 2) = zAxis.x();
    rot3x3(1, 2) = zAxis.y();
    rot3x3(2, 2) = zAxis.z();

    return QQuaternion::fromRotationMatrix(rot3x3);
}

QQuaternion QQuaternion::fromDirection(const QVector3D &direction, const QVector3D &up)
{
    if (qFuzzyIsNull(direction.x()) && qFuzzyIsNull(direction.y()) && qFuzzyIsNull(direction.z())) return QQuaternion();

    const QVector3D zAxis(direction.normalized());
    QVector3D xAxis(QVector3D::crossProduct(up, zAxis));
    if (qFuzzyIsNull(xAxis.lengthSquared())) {
        // collinear or invalid up vector; derive shortest arc to new direction
        return QQuaternion::rotationTo(QVector3D(0.0f, 0.0f, 1.0f), zAxis);
    }

    xAxis.normalize();
    const QVector3D yAxis(QVector3D::crossProduct(zAxis, xAxis));

    return QQuaternion::fromAxes(xAxis, yAxis, zAxis);
}

QQuaternion QQuaternion::rotationTo(const QVector3D &from, const QVector3D &to)
{
    // Based on Stan Melax's article in Game Programming Gems

    const QVector3D v0(from.normalized());
    const QVector3D v1(to.normalized());

    float d = QVector3D::dotProduct(v0, v1) + 1.0f;

    // if dest vector is close to the inverse of source vector, ANY axis of rotation is valid
    if (qFuzzyIsNull(d)) {
        QVector3D axis = QVector3D::crossProduct(QVector3D(1.0f, 0.0f, 0.0f), v0);
        if (qFuzzyIsNull(axis.lengthSquared())) axis = QVector3D::crossProduct(QVector3D(0.0f, 1.0f, 0.0f), v0);
        axis.normalize();

        // same as QQuaternion::fromAxisAndAngle(axis, 180.0f)
        return QQuaternion(0.0f, axis.x(), axis.y(), axis.z());
    }

    d = std::sqrt(2.0f * d);
    const QVector3D axis(QVector3D::crossProduct(v0, v1) / d);

    return QQuaternion(d * 0.5f, axis).normalized();
}

QQuaternion QQuaternion::slerp(const QQuaternion &q1, const QQuaternion &q2, float t)
{
    // Handle the easy cases first.
    if (t <= 0.0f)
        return q1;
    else if (t >= 1.0f)
        return q2;

    // Determine the angle between the two quaternions.
    QQuaternion q2b(q2);
    float dot = QQuaternion::dotProduct(q1, q2);
    if (dot < 0.0f) {
        q2b = -q2b;
        dot = -dot;
    }

    // Get the scale factors.  If they are too small,
    // then revert to simple linear interpolation.
    float factor1 = 1.0f - t;
    float factor2 = t;
    if ((1.0f - dot) > 0.0000001) {
        float angle = std::acos(dot);
        float sinOfAngle = std::sin(angle);
        if (sinOfAngle > 0.0000001) {
            factor1 = std::sin((1.0f - t) * angle) / sinOfAngle;
            factor2 = std::sin(t * angle) / sinOfAngle;
        }
    }

    // Construct the result quaternion.
    return q1 * factor1 + q2b * factor2;
}

QQuaternion QQuaternion::nlerp(const QQuaternion &q1, const QQuaternion &q2, float t)
{
    // Handle the easy cases first.
    if (t <= 0.0f)
        return q1;
    else if (t >= 1.0f)
        return q2;

    // Determine the angle between the two quaternions.
    QQuaternion q2b(q2);
    float dot = QQuaternion::dotProduct(q1, q2);
    if (dot < 0.0f) q2b = -q2b;

    // Perform the linear interpolation.
    return (q1 * (1.0f - t) + q2b * t).normalized();
}
