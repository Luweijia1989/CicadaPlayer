#include "qmatrix4x4.h"
#include <cmath>

static const float inv_dist_to_plane = 1.0f / 1024.0f;

QMatrix4x4::QMatrix4x4(const float *values)
{
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) m[col][row] = values[row * 4 + col];
    flagBits = General;
}

QMatrix4x4::QMatrix4x4(const float *values, int cols, int rows)
{
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (col < cols && row < rows)
                m[col][row] = values[col * rows + row];
            else if (col == row)
                m[col][row] = 1.0f;
            else
                m[col][row] = 0.0f;
        }
    }
    flagBits = General;
}

static inline double matrixDet2(const double m[4][4], int col0, int col1, int row0, int row1)
{
    return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
}

static inline double matrixDet3(const double m[4][4], int col0, int col1, int col2, int row0, int row1, int row2)
{
    return m[col0][row0] * matrixDet2(m, col1, col2, row1, row2) - m[col1][row0] * matrixDet2(m, col0, col2, row1, row2) +
           m[col2][row0] * matrixDet2(m, col0, col1, row1, row2);
}

static inline double matrixDet4(const double m[4][4])
{
    double det;
    det = m[0][0] * matrixDet3(m, 1, 2, 3, 1, 2, 3);
    det -= m[1][0] * matrixDet3(m, 0, 2, 3, 1, 2, 3);
    det += m[2][0] * matrixDet3(m, 0, 1, 3, 1, 2, 3);
    det -= m[3][0] * matrixDet3(m, 0, 1, 2, 1, 2, 3);
    return det;
}

static inline void copyToDoubles(const float m[4][4], double mm[4][4])
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) mm[i][j] = double(m[i][j]);
}

double QMatrix4x4::determinant() const
{
    if ((flagBits & ~(Translation | Rotation2D | Rotation)) == Identity) return 1.0;

    double mm[4][4];
    copyToDoubles(m, mm);
    if (flagBits < Rotation2D) return mm[0][0] * mm[1][1] * mm[2][2];// Translation | Scale
    if (flagBits < Perspective) return matrixDet3(mm, 0, 1, 2, 0, 1, 2);
    return matrixDet4(mm);
}

QMatrix4x4 QMatrix4x4::inverted(bool *invertible) const
{
    // Handle some of the easy cases first.
    if (flagBits == Identity) {
        if (invertible) *invertible = true;
        return QMatrix4x4();
    } else if (flagBits == Translation) {
        QMatrix4x4 inv;
        inv.m[3][0] = -m[3][0];
        inv.m[3][1] = -m[3][1];
        inv.m[3][2] = -m[3][2];
        inv.flagBits = Translation;
        if (invertible) *invertible = true;
        return inv;
    } else if (flagBits < Rotation2D) {
        // Translation | Scale
        if (m[0][0] == 0 || m[1][1] == 0 || m[2][2] == 0) {
            if (invertible) *invertible = false;
            return QMatrix4x4();
        }
        QMatrix4x4 inv;
        inv.m[0][0] = 1.0f / m[0][0];
        inv.m[1][1] = 1.0f / m[1][1];
        inv.m[2][2] = 1.0f / m[2][2];
        inv.m[3][0] = -m[3][0] * inv.m[0][0];
        inv.m[3][1] = -m[3][1] * inv.m[1][1];
        inv.m[3][2] = -m[3][2] * inv.m[2][2];
        inv.flagBits = flagBits;

        if (invertible) *invertible = true;
        return inv;
    } else if ((flagBits & ~(Translation | Rotation2D | Rotation)) == Identity) {
        if (invertible) *invertible = true;
        return orthonormalInverse();
    } else if (flagBits < Perspective) {
        QMatrix4x4 inv(1);// The "1" says to not load the identity.

        double mm[4][4];
        copyToDoubles(m, mm);

        double det = matrixDet3(mm, 0, 1, 2, 0, 1, 2);
        if (det == 0.0f) {
            if (invertible) *invertible = false;
            return QMatrix4x4();
        }
        det = 1.0f / det;

        inv.m[0][0] = matrixDet2(mm, 1, 2, 1, 2) * det;
        inv.m[0][1] = -matrixDet2(mm, 0, 2, 1, 2) * det;
        inv.m[0][2] = matrixDet2(mm, 0, 1, 1, 2) * det;
        inv.m[0][3] = 0;
        inv.m[1][0] = -matrixDet2(mm, 1, 2, 0, 2) * det;
        inv.m[1][1] = matrixDet2(mm, 0, 2, 0, 2) * det;
        inv.m[1][2] = -matrixDet2(mm, 0, 1, 0, 2) * det;
        inv.m[1][3] = 0;
        inv.m[2][0] = matrixDet2(mm, 1, 2, 0, 1) * det;
        inv.m[2][1] = -matrixDet2(mm, 0, 2, 0, 1) * det;
        inv.m[2][2] = matrixDet2(mm, 0, 1, 0, 1) * det;
        inv.m[2][3] = 0;
        inv.m[3][0] = -inv.m[0][0] * m[3][0] - inv.m[1][0] * m[3][1] - inv.m[2][0] * m[3][2];
        inv.m[3][1] = -inv.m[0][1] * m[3][0] - inv.m[1][1] * m[3][1] - inv.m[2][1] * m[3][2];
        inv.m[3][2] = -inv.m[0][2] * m[3][0] - inv.m[1][2] * m[3][1] - inv.m[2][2] * m[3][2];
        inv.m[3][3] = 1;
        inv.flagBits = flagBits;

        if (invertible) *invertible = true;
        return inv;
    }

    QMatrix4x4 inv(1);// The "1" says to not load the identity.

    double mm[4][4];
    copyToDoubles(m, mm);

    double det = matrixDet4(mm);
    if (det == 0.0f) {
        if (invertible) *invertible = false;
        return QMatrix4x4();
    }
    det = 1.0f / det;

    inv.m[0][0] = matrixDet3(mm, 1, 2, 3, 1, 2, 3) * det;
    inv.m[0][1] = -matrixDet3(mm, 0, 2, 3, 1, 2, 3) * det;
    inv.m[0][2] = matrixDet3(mm, 0, 1, 3, 1, 2, 3) * det;
    inv.m[0][3] = -matrixDet3(mm, 0, 1, 2, 1, 2, 3) * det;
    inv.m[1][0] = -matrixDet3(mm, 1, 2, 3, 0, 2, 3) * det;
    inv.m[1][1] = matrixDet3(mm, 0, 2, 3, 0, 2, 3) * det;
    inv.m[1][2] = -matrixDet3(mm, 0, 1, 3, 0, 2, 3) * det;
    inv.m[1][3] = matrixDet3(mm, 0, 1, 2, 0, 2, 3) * det;
    inv.m[2][0] = matrixDet3(mm, 1, 2, 3, 0, 1, 3) * det;
    inv.m[2][1] = -matrixDet3(mm, 0, 2, 3, 0, 1, 3) * det;
    inv.m[2][2] = matrixDet3(mm, 0, 1, 3, 0, 1, 3) * det;
    inv.m[2][3] = -matrixDet3(mm, 0, 1, 2, 0, 1, 3) * det;
    inv.m[3][0] = -matrixDet3(mm, 1, 2, 3, 0, 1, 2) * det;
    inv.m[3][1] = matrixDet3(mm, 0, 2, 3, 0, 1, 2) * det;
    inv.m[3][2] = -matrixDet3(mm, 0, 1, 3, 0, 1, 2) * det;
    inv.m[3][3] = matrixDet3(mm, 0, 1, 2, 0, 1, 2) * det;
    inv.flagBits = flagBits;

    if (invertible) *invertible = true;
    return inv;
}

QMatrix3x3 QMatrix4x4::normalMatrix() const
{
    QMatrix3x3 inv;

    // Handle the simple cases first.
    if (flagBits < Scale) {
        // Translation
        return inv;
    } else if (flagBits < Rotation2D) {
        // Translation | Scale
        if (m[0][0] == 0.0f || m[1][1] == 0.0f || m[2][2] == 0.0f) return inv;
        inv.data()[0] = 1.0f / m[0][0];
        inv.data()[4] = 1.0f / m[1][1];
        inv.data()[8] = 1.0f / m[2][2];
        return inv;
    } else if ((flagBits & ~(Translation | Rotation2D | Rotation)) == Identity) {
        float *invm = inv.data();
        invm[0 + 0 * 3] = m[0][0];
        invm[1 + 0 * 3] = m[0][1];
        invm[2 + 0 * 3] = m[0][2];
        invm[0 + 1 * 3] = m[1][0];
        invm[1 + 1 * 3] = m[1][1];
        invm[2 + 1 * 3] = m[1][2];
        invm[0 + 2 * 3] = m[2][0];
        invm[1 + 2 * 3] = m[2][1];
        invm[2 + 2 * 3] = m[2][2];
        return inv;
    }

    double mm[4][4];
    copyToDoubles(m, mm);
    double det = matrixDet3(mm, 0, 1, 2, 0, 1, 2);
    if (det == 0.0f) return inv;
    det = 1.0f / det;

    float *invm = inv.data();

    // Invert and transpose in a single step.
    invm[0 + 0 * 3] = (mm[1][1] * mm[2][2] - mm[2][1] * mm[1][2]) * det;
    invm[1 + 0 * 3] = -(mm[1][0] * mm[2][2] - mm[1][2] * mm[2][0]) * det;
    invm[2 + 0 * 3] = (mm[1][0] * mm[2][1] - mm[1][1] * mm[2][0]) * det;
    invm[0 + 1 * 3] = -(mm[0][1] * mm[2][2] - mm[2][1] * mm[0][2]) * det;
    invm[1 + 1 * 3] = (mm[0][0] * mm[2][2] - mm[0][2] * mm[2][0]) * det;
    invm[2 + 1 * 3] = -(mm[0][0] * mm[2][1] - mm[0][1] * mm[2][0]) * det;
    invm[0 + 2 * 3] = (mm[0][1] * mm[1][2] - mm[0][2] * mm[1][1]) * det;
    invm[1 + 2 * 3] = -(mm[0][0] * mm[1][2] - mm[0][2] * mm[1][0]) * det;
    invm[2 + 2 * 3] = (mm[0][0] * mm[1][1] - mm[1][0] * mm[0][1]) * det;

    return inv;
}

QMatrix4x4 QMatrix4x4::transposed() const
{
    QMatrix4x4 result(1);// The "1" says to not load the identity.
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.m[col][row] = m[row][col];
        }
    }
    // When a translation is transposed, it becomes a perspective transformation.
    result.flagBits = (flagBits & Translation ? General : flagBits);
    return result;
}

QMatrix4x4 &QMatrix4x4::operator/=(float divisor)
{
    m[0][0] /= divisor;
    m[0][1] /= divisor;
    m[0][2] /= divisor;
    m[0][3] /= divisor;
    m[1][0] /= divisor;
    m[1][1] /= divisor;
    m[1][2] /= divisor;
    m[1][3] /= divisor;
    m[2][0] /= divisor;
    m[2][1] /= divisor;
    m[2][2] /= divisor;
    m[2][3] /= divisor;
    m[3][0] /= divisor;
    m[3][1] /= divisor;
    m[3][2] /= divisor;
    m[3][3] /= divisor;
    flagBits = General;
    return *this;
}

QMatrix4x4 operator/(const QMatrix4x4 &matrix, float divisor)
{
    QMatrix4x4 m(1);// The "1" says to not load the identity.
    m.m[0][0] = matrix.m[0][0] / divisor;
    m.m[0][1] = matrix.m[0][1] / divisor;
    m.m[0][2] = matrix.m[0][2] / divisor;
    m.m[0][3] = matrix.m[0][3] / divisor;
    m.m[1][0] = matrix.m[1][0] / divisor;
    m.m[1][1] = matrix.m[1][1] / divisor;
    m.m[1][2] = matrix.m[1][2] / divisor;
    m.m[1][3] = matrix.m[1][3] / divisor;
    m.m[2][0] = matrix.m[2][0] / divisor;
    m.m[2][1] = matrix.m[2][1] / divisor;
    m.m[2][2] = matrix.m[2][2] / divisor;
    m.m[2][3] = matrix.m[2][3] / divisor;
    m.m[3][0] = matrix.m[3][0] / divisor;
    m.m[3][1] = matrix.m[3][1] / divisor;
    m.m[3][2] = matrix.m[3][2] / divisor;
    m.m[3][3] = matrix.m[3][3] / divisor;
    m.flagBits = QMatrix4x4::General;
    return m;
}

void QMatrix4x4::scale(const QVector3D &vector)
{
    float vx = vector.x();
    float vy = vector.y();
    float vz = vector.z();
    if (flagBits < Scale) {
        m[0][0] = vx;
        m[1][1] = vy;
        m[2][2] = vz;
    } else if (flagBits < Rotation2D) {
        m[0][0] *= vx;
        m[1][1] *= vy;
        m[2][2] *= vz;
    } else if (flagBits < Rotation) {
        m[0][0] *= vx;
        m[0][1] *= vx;
        m[1][0] *= vy;
        m[1][1] *= vy;
        m[2][2] *= vz;
    } else {
        m[0][0] *= vx;
        m[0][1] *= vx;
        m[0][2] *= vx;
        m[0][3] *= vx;
        m[1][0] *= vy;
        m[1][1] *= vy;
        m[1][2] *= vy;
        m[1][3] *= vy;
        m[2][0] *= vz;
        m[2][1] *= vz;
        m[2][2] *= vz;
        m[2][3] *= vz;
    }
    flagBits |= Scale;
}

void QMatrix4x4::scale(float x, float y)
{
    if (flagBits < Scale) {
        m[0][0] = x;
        m[1][1] = y;
    } else if (flagBits < Rotation2D) {
        m[0][0] *= x;
        m[1][1] *= y;
    } else if (flagBits < Rotation) {
        m[0][0] *= x;
        m[0][1] *= x;
        m[1][0] *= y;
        m[1][1] *= y;
    } else {
        m[0][0] *= x;
        m[0][1] *= x;
        m[0][2] *= x;
        m[0][3] *= x;
        m[1][0] *= y;
        m[1][1] *= y;
        m[1][2] *= y;
        m[1][3] *= y;
    }
    flagBits |= Scale;
}

void QMatrix4x4::scale(float x, float y, float z)
{
    if (flagBits < Scale) {
        m[0][0] = x;
        m[1][1] = y;
        m[2][2] = z;
    } else if (flagBits < Rotation2D) {
        m[0][0] *= x;
        m[1][1] *= y;
        m[2][2] *= z;
    } else if (flagBits < Rotation) {
        m[0][0] *= x;
        m[0][1] *= x;
        m[1][0] *= y;
        m[1][1] *= y;
        m[2][2] *= z;
    } else {
        m[0][0] *= x;
        m[0][1] *= x;
        m[0][2] *= x;
        m[0][3] *= x;
        m[1][0] *= y;
        m[1][1] *= y;
        m[1][2] *= y;
        m[1][3] *= y;
        m[2][0] *= z;
        m[2][1] *= z;
        m[2][2] *= z;
        m[2][3] *= z;
    }
    flagBits |= Scale;
}

void QMatrix4x4::scale(float factor)
{
    if (flagBits < Scale) {
        m[0][0] = factor;
        m[1][1] = factor;
        m[2][2] = factor;
    } else if (flagBits < Rotation2D) {
        m[0][0] *= factor;
        m[1][1] *= factor;
        m[2][2] *= factor;
    } else if (flagBits < Rotation) {
        m[0][0] *= factor;
        m[0][1] *= factor;
        m[1][0] *= factor;
        m[1][1] *= factor;
        m[2][2] *= factor;
    } else {
        m[0][0] *= factor;
        m[0][1] *= factor;
        m[0][2] *= factor;
        m[0][3] *= factor;
        m[1][0] *= factor;
        m[1][1] *= factor;
        m[1][2] *= factor;
        m[1][3] *= factor;
        m[2][0] *= factor;
        m[2][1] *= factor;
        m[2][2] *= factor;
        m[2][3] *= factor;
    }
    flagBits |= Scale;
}

void QMatrix4x4::translate(const QVector3D &vector)
{
    float vx = vector.x();
    float vy = vector.y();
    float vz = vector.z();
    if (flagBits == Identity) {
        m[3][0] = vx;
        m[3][1] = vy;
        m[3][2] = vz;
    } else if (flagBits == Translation) {
        m[3][0] += vx;
        m[3][1] += vy;
        m[3][2] += vz;
    } else if (flagBits == Scale) {
        m[3][0] = m[0][0] * vx;
        m[3][1] = m[1][1] * vy;
        m[3][2] = m[2][2] * vz;
    } else if (flagBits == (Translation | Scale)) {
        m[3][0] += m[0][0] * vx;
        m[3][1] += m[1][1] * vy;
        m[3][2] += m[2][2] * vz;
    } else if (flagBits < Rotation) {
        m[3][0] += m[0][0] * vx + m[1][0] * vy;
        m[3][1] += m[0][1] * vx + m[1][1] * vy;
        m[3][2] += m[2][2] * vz;
    } else {
        m[3][0] += m[0][0] * vx + m[1][0] * vy + m[2][0] * vz;
        m[3][1] += m[0][1] * vx + m[1][1] * vy + m[2][1] * vz;
        m[3][2] += m[0][2] * vx + m[1][2] * vy + m[2][2] * vz;
        m[3][3] += m[0][3] * vx + m[1][3] * vy + m[2][3] * vz;
    }
    flagBits |= Translation;
}

void QMatrix4x4::translate(float x, float y)
{
    if (flagBits == Identity) {
        m[3][0] = x;
        m[3][1] = y;
    } else if (flagBits == Translation) {
        m[3][0] += x;
        m[3][1] += y;
    } else if (flagBits == Scale) {
        m[3][0] = m[0][0] * x;
        m[3][1] = m[1][1] * y;
    } else if (flagBits == (Translation | Scale)) {
        m[3][0] += m[0][0] * x;
        m[3][1] += m[1][1] * y;
    } else if (flagBits < Rotation) {
        m[3][0] += m[0][0] * x + m[1][0] * y;
        m[3][1] += m[0][1] * x + m[1][1] * y;
    } else {
        m[3][0] += m[0][0] * x + m[1][0] * y;
        m[3][1] += m[0][1] * x + m[1][1] * y;
        m[3][2] += m[0][2] * x + m[1][2] * y;
        m[3][3] += m[0][3] * x + m[1][3] * y;
    }
    flagBits |= Translation;
}

void QMatrix4x4::translate(float x, float y, float z)
{
    if (flagBits == Identity) {
        m[3][0] = x;
        m[3][1] = y;
        m[3][2] = z;
    } else if (flagBits == Translation) {
        m[3][0] += x;
        m[3][1] += y;
        m[3][2] += z;
    } else if (flagBits == Scale) {
        m[3][0] = m[0][0] * x;
        m[3][1] = m[1][1] * y;
        m[3][2] = m[2][2] * z;
    } else if (flagBits == (Translation | Scale)) {
        m[3][0] += m[0][0] * x;
        m[3][1] += m[1][1] * y;
        m[3][2] += m[2][2] * z;
    } else if (flagBits < Rotation) {
        m[3][0] += m[0][0] * x + m[1][0] * y;
        m[3][1] += m[0][1] * x + m[1][1] * y;
        m[3][2] += m[2][2] * z;
    } else {
        m[3][0] += m[0][0] * x + m[1][0] * y + m[2][0] * z;
        m[3][1] += m[0][1] * x + m[1][1] * y + m[2][1] * z;
        m[3][2] += m[0][2] * x + m[1][2] * y + m[2][2] * z;
        m[3][3] += m[0][3] * x + m[1][3] * y + m[2][3] * z;
    }
    flagBits |= Translation;
}

void QMatrix4x4::rotate(float angle, const QVector3D &vector)
{
    rotate(angle, vector.x(), vector.y(), vector.z());
}

void QMatrix4x4::rotate(float angle, float x, float y, float z)
{
    if (angle == 0.0f) return;
    float c, s;
    if (angle == 90.0f || angle == -270.0f) {
        s = 1.0f;
        c = 0.0f;
    } else if (angle == -90.0f || angle == 270.0f) {
        s = -1.0f;
        c = 0.0f;
    } else if (angle == 180.0f || angle == -180.0f) {
        s = 0.0f;
        c = -1.0f;
    } else {
        float a = qDegreesToRadians(angle);
        c = std::cos(a);
        s = std::sin(a);
    }
    if (x == 0.0f) {
        if (y == 0.0f) {
            if (z != 0.0f) {
                // Rotate around the Z axis.
                if (z < 0) s = -s;
                float tmp;
                m[0][0] = (tmp = m[0][0]) * c + m[1][0] * s;
                m[1][0] = m[1][0] * c - tmp * s;
                m[0][1] = (tmp = m[0][1]) * c + m[1][1] * s;
                m[1][1] = m[1][1] * c - tmp * s;
                m[0][2] = (tmp = m[0][2]) * c + m[1][2] * s;
                m[1][2] = m[1][2] * c - tmp * s;
                m[0][3] = (tmp = m[0][3]) * c + m[1][3] * s;
                m[1][3] = m[1][3] * c - tmp * s;

                flagBits |= Rotation2D;
                return;
            }
        } else if (z == 0.0f) {
            // Rotate around the Y axis.
            if (y < 0) s = -s;
            float tmp;
            m[2][0] = (tmp = m[2][0]) * c + m[0][0] * s;
            m[0][0] = m[0][0] * c - tmp * s;
            m[2][1] = (tmp = m[2][1]) * c + m[0][1] * s;
            m[0][1] = m[0][1] * c - tmp * s;
            m[2][2] = (tmp = m[2][2]) * c + m[0][2] * s;
            m[0][2] = m[0][2] * c - tmp * s;
            m[2][3] = (tmp = m[2][3]) * c + m[0][3] * s;
            m[0][3] = m[0][3] * c - tmp * s;

            flagBits |= Rotation;
            return;
        }
    } else if (y == 0.0f && z == 0.0f) {
        // Rotate around the X axis.
        if (x < 0) s = -s;
        float tmp;
        m[1][0] = (tmp = m[1][0]) * c + m[2][0] * s;
        m[2][0] = m[2][0] * c - tmp * s;
        m[1][1] = (tmp = m[1][1]) * c + m[2][1] * s;
        m[2][1] = m[2][1] * c - tmp * s;
        m[1][2] = (tmp = m[1][2]) * c + m[2][2] * s;
        m[2][2] = m[2][2] * c - tmp * s;
        m[1][3] = (tmp = m[1][3]) * c + m[2][3] * s;
        m[2][3] = m[2][3] * c - tmp * s;

        flagBits |= Rotation;
        return;
    }

    double len = double(x) * double(x) + double(y) * double(y) + double(z) * double(z);
    if (!qFuzzyCompare(len, 1.0) && !qFuzzyIsNull(len)) {
        len = std::sqrt(len);
        x = float(double(x) / len);
        y = float(double(y) / len);
        z = float(double(z) / len);
    }
    float ic = 1.0f - c;
    QMatrix4x4 rot(1);// The "1" says to not load the identity.
    rot.m[0][0] = x * x * ic + c;
    rot.m[1][0] = x * y * ic - z * s;
    rot.m[2][0] = x * z * ic + y * s;
    rot.m[3][0] = 0.0f;
    rot.m[0][1] = y * x * ic + z * s;
    rot.m[1][1] = y * y * ic + c;
    rot.m[2][1] = y * z * ic - x * s;
    rot.m[3][1] = 0.0f;
    rot.m[0][2] = x * z * ic - y * s;
    rot.m[1][2] = y * z * ic + x * s;
    rot.m[2][2] = z * z * ic + c;
    rot.m[3][2] = 0.0f;
    rot.m[0][3] = 0.0f;
    rot.m[1][3] = 0.0f;
    rot.m[2][3] = 0.0f;
    rot.m[3][3] = 1.0f;
    rot.flagBits = Rotation;
    *this *= rot;
}

void QMatrix4x4::projectedRotate(float angle, float x, float y, float z)
{
    // Used by QGraphicsRotation::applyTo() to perform a rotation
    // and projection back to 2D in a single step.
    if (angle == 0.0f) return;
    float c, s;
    if (angle == 90.0f || angle == -270.0f) {
        s = 1.0f;
        c = 0.0f;
    } else if (angle == -90.0f || angle == 270.0f) {
        s = -1.0f;
        c = 0.0f;
    } else if (angle == 180.0f || angle == -180.0f) {
        s = 0.0f;
        c = -1.0f;
    } else {
        float a = qDegreesToRadians(angle);
        c = std::cos(a);
        s = std::sin(a);
    }
    if (x == 0.0f) {
        if (y == 0.0f) {
            if (z != 0.0f) {
                // Rotate around the Z axis.
                if (z < 0) s = -s;
                float tmp;
                m[0][0] = (tmp = m[0][0]) * c + m[1][0] * s;
                m[1][0] = m[1][0] * c - tmp * s;
                m[0][1] = (tmp = m[0][1]) * c + m[1][1] * s;
                m[1][1] = m[1][1] * c - tmp * s;
                m[0][2] = (tmp = m[0][2]) * c + m[1][2] * s;
                m[1][2] = m[1][2] * c - tmp * s;
                m[0][3] = (tmp = m[0][3]) * c + m[1][3] * s;
                m[1][3] = m[1][3] * c - tmp * s;

                flagBits |= Rotation2D;
                return;
            }
        } else if (z == 0.0f) {
            // Rotate around the Y axis.
            if (y < 0) s = -s;
            m[0][0] = m[0][0] * c + m[3][0] * s * inv_dist_to_plane;
            m[0][1] = m[0][1] * c + m[3][1] * s * inv_dist_to_plane;
            m[0][2] = m[0][2] * c + m[3][2] * s * inv_dist_to_plane;
            m[0][3] = m[0][3] * c + m[3][3] * s * inv_dist_to_plane;
            flagBits = General;
            return;
        }
    } else if (y == 0.0f && z == 0.0f) {
        // Rotate around the X axis.
        if (x < 0) s = -s;
        m[1][0] = m[1][0] * c - m[3][0] * s * inv_dist_to_plane;
        m[1][1] = m[1][1] * c - m[3][1] * s * inv_dist_to_plane;
        m[1][2] = m[1][2] * c - m[3][2] * s * inv_dist_to_plane;
        m[1][3] = m[1][3] * c - m[3][3] * s * inv_dist_to_plane;
        flagBits = General;
        return;
    }
    double len = double(x) * double(x) + double(y) * double(y) + double(z) * double(z);
    if (!qFuzzyCompare(len, 1.0) && !qFuzzyIsNull(len)) {
        len = std::sqrt(len);
        x = float(double(x) / len);
        y = float(double(y) / len);
        z = float(double(z) / len);
    }
    float ic = 1.0f - c;
    QMatrix4x4 rot(1);// The "1" says to not load the identity.
    rot.m[0][0] = x * x * ic + c;
    rot.m[1][0] = x * y * ic - z * s;
    rot.m[2][0] = 0.0f;
    rot.m[3][0] = 0.0f;
    rot.m[0][1] = y * x * ic + z * s;
    rot.m[1][1] = y * y * ic + c;
    rot.m[2][1] = 0.0f;
    rot.m[3][1] = 0.0f;
    rot.m[0][2] = 0.0f;
    rot.m[1][2] = 0.0f;
    rot.m[2][2] = 1.0f;
    rot.m[3][2] = 0.0f;
    rot.m[0][3] = (x * z * ic - y * s) * -inv_dist_to_plane;
    rot.m[1][3] = (y * z * ic + x * s) * -inv_dist_to_plane;
    rot.m[2][3] = 0.0f;
    rot.m[3][3] = 1.0f;
    rot.flagBits = General;
    *this *= rot;
}

void QMatrix4x4::rotate(const QQuaternion &quaternion)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q54

    QMatrix4x4 m(YPP::Uninitialized);

    const float f2x = quaternion.x() + quaternion.x();
    const float f2y = quaternion.y() + quaternion.y();
    const float f2z = quaternion.z() + quaternion.z();
    const float f2xw = f2x * quaternion.scalar();
    const float f2yw = f2y * quaternion.scalar();
    const float f2zw = f2z * quaternion.scalar();
    const float f2xx = f2x * quaternion.x();
    const float f2xy = f2x * quaternion.y();
    const float f2xz = f2x * quaternion.z();
    const float f2yy = f2y * quaternion.y();
    const float f2yz = f2y * quaternion.z();
    const float f2zz = f2z * quaternion.z();

    m.m[0][0] = 1.0f - (f2yy + f2zz);
    m.m[1][0] = f2xy - f2zw;
    m.m[2][0] = f2xz + f2yw;
    m.m[3][0] = 0.0f;
    m.m[0][1] = f2xy + f2zw;
    m.m[1][1] = 1.0f - (f2xx + f2zz);
    m.m[2][1] = f2yz - f2xw;
    m.m[3][1] = 0.0f;
    m.m[0][2] = f2xz - f2yw;
    m.m[1][2] = f2yz + f2xw;
    m.m[2][2] = 1.0f - (f2xx + f2yy);
    m.m[3][2] = 0.0f;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;
    m.flagBits = Rotation;
    *this *= m;
}

void QMatrix4x4::ortho(const QRect &rect)
{
    // Note: rect.right() and rect.bottom() subtract 1 in QRect,
    // which gives the location of a pixel within the rectangle,
    // instead of the extent of the rectangle.  We want the extent.
    // QRectF expresses the extent properly.
    ortho(rect.x(), rect.x() + rect.width(), rect.y() + rect.height(), rect.y(), -1.0f, 1.0f);
}

void QMatrix4x4::ortho(const QRectF &rect)
{
    ortho(rect.left(), rect.right(), rect.bottom(), rect.top(), -1.0f, 1.0f);
}

void QMatrix4x4::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    // Bail out if the projection volume is zero-sized.
    if (left == right || bottom == top || nearPlane == farPlane) return;

    // Construct the projection.
    float width = right - left;
    float invheight = top - bottom;
    float clip = farPlane - nearPlane;
    QMatrix4x4 m(1);
    m.m[0][0] = 2.0f / width;
    m.m[1][0] = 0.0f;
    m.m[2][0] = 0.0f;
    m.m[3][0] = -(left + right) / width;
    m.m[0][1] = 0.0f;
    m.m[1][1] = 2.0f / invheight;
    m.m[2][1] = 0.0f;
    m.m[3][1] = -(top + bottom) / invheight;
    m.m[0][2] = 0.0f;
    m.m[1][2] = 0.0f;
    m.m[2][2] = -2.0f / clip;
    m.m[3][2] = -(nearPlane + farPlane) / clip;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;
    m.flagBits = Translation | Scale;

    // Apply the projection.
    *this *= m;
}

void QMatrix4x4::frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    // Bail out if the projection volume is zero-sized.
    if (left == right || bottom == top || nearPlane == farPlane) return;

    // Construct the projection.
    QMatrix4x4 m(1);
    float width = right - left;
    float invheight = top - bottom;
    float clip = farPlane - nearPlane;
    m.m[0][0] = 2.0f * nearPlane / width;
    m.m[1][0] = 0.0f;
    m.m[2][0] = (left + right) / width;
    m.m[3][0] = 0.0f;
    m.m[0][1] = 0.0f;
    m.m[1][1] = 2.0f * nearPlane / invheight;
    m.m[2][1] = (top + bottom) / invheight;
    m.m[3][1] = 0.0f;
    m.m[0][2] = 0.0f;
    m.m[1][2] = 0.0f;
    m.m[2][2] = -(nearPlane + farPlane) / clip;
    m.m[3][2] = -2.0f * nearPlane * farPlane / clip;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = -1.0f;
    m.m[3][3] = 0.0f;
    m.flagBits = General;

    // Apply the projection.
    *this *= m;
}

void QMatrix4x4::perspective(float verticalAngle, float aspectRatio, float nearPlane, float farPlane)
{
    // Bail out if the projection volume is zero-sized.
    if (nearPlane == farPlane || aspectRatio == 0.0f) return;

    // Construct the projection.
    QMatrix4x4 m(1);
    float radians = qDegreesToRadians(verticalAngle / 2.0f);
    float sine = std::sin(radians);
    if (sine == 0.0f) return;
    float cotan = std::cos(radians) / sine;
    float clip = farPlane - nearPlane;
    m.m[0][0] = cotan / aspectRatio;
    m.m[1][0] = 0.0f;
    m.m[2][0] = 0.0f;
    m.m[3][0] = 0.0f;
    m.m[0][1] = 0.0f;
    m.m[1][1] = cotan;
    m.m[2][1] = 0.0f;
    m.m[3][1] = 0.0f;
    m.m[0][2] = 0.0f;
    m.m[1][2] = 0.0f;
    m.m[2][2] = -(nearPlane + farPlane) / clip;
    m.m[3][2] = -(2.0f * nearPlane * farPlane) / clip;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = -1.0f;
    m.m[3][3] = 0.0f;
    m.flagBits = General;

    // Apply the projection.
    *this *= m;
}

void QMatrix4x4::lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up)
{
    QVector3D forward = center - eye;
    if (qFuzzyIsNull(forward.x()) && qFuzzyIsNull(forward.y()) && qFuzzyIsNull(forward.z())) return;

    forward.normalize();
    QVector3D side = QVector3D::crossProduct(forward, up).normalized();
    QVector3D upVector = QVector3D::crossProduct(side, forward);

    QMatrix4x4 m(1);
    m.m[0][0] = side.x();
    m.m[1][0] = side.y();
    m.m[2][0] = side.z();
    m.m[3][0] = 0.0f;
    m.m[0][1] = upVector.x();
    m.m[1][1] = upVector.y();
    m.m[2][1] = upVector.z();
    m.m[3][1] = 0.0f;
    m.m[0][2] = -forward.x();
    m.m[1][2] = -forward.y();
    m.m[2][2] = -forward.z();
    m.m[3][2] = 0.0f;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;
    m.flagBits = Rotation;

    *this *= m;
    translate(-eye);
}

void QMatrix4x4::viewport(float left, float bottom, float width, float height, float nearPlane, float farPlane)
{
    const float w2 = width / 2.0f;
    const float h2 = height / 2.0f;

    QMatrix4x4 m(1);
    m.m[0][0] = w2;
    m.m[1][0] = 0.0f;
    m.m[2][0] = 0.0f;
    m.m[3][0] = left + w2;
    m.m[0][1] = 0.0f;
    m.m[1][1] = h2;
    m.m[2][1] = 0.0f;
    m.m[3][1] = bottom + h2;
    m.m[0][2] = 0.0f;
    m.m[1][2] = 0.0f;
    m.m[2][2] = (farPlane - nearPlane) / 2.0f;
    m.m[3][2] = (nearPlane + farPlane) / 2.0f;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;
    m.flagBits = General;

    *this *= m;
}

void QMatrix4x4::flipCoordinates()
{
    // Multiplying the y and z coordinates with -1 does NOT flip between right-handed and
    // left-handed coordinate systems, it just rotates 180 degrees around the x axis, so
    // I'm deprecating this function.
    if (flagBits < Rotation2D) {
        // Translation | Scale
        m[1][1] = -m[1][1];
        m[2][2] = -m[2][2];
    } else {
        m[1][0] = -m[1][0];
        m[1][1] = -m[1][1];
        m[1][2] = -m[1][2];
        m[1][3] = -m[1][3];
        m[2][0] = -m[2][0];
        m[2][1] = -m[2][1];
        m[2][2] = -m[2][2];
        m[2][3] = -m[2][3];
    }
    flagBits |= Scale;
}

void QMatrix4x4::copyDataTo(float *values) const
{
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) values[row * 4 + col] = float(m[col][row]);
}

QRect QMatrix4x4::mapRect(const QRect &rect) const
{
    if (flagBits < Scale) {
        // Translation
        return QRect(std::round(rect.x() + m[3][0]), std::round(rect.y() + m[3][1]), rect.width(), rect.height());
    } else if (flagBits < Rotation2D) {
        // Translation | Scale
        float x = rect.x() * m[0][0] + m[3][0];
        float y = rect.y() * m[1][1] + m[3][1];
        float w = rect.width() * m[0][0];
        float h = rect.height() * m[1][1];
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        return QRect(std::round(x), std::round(y), std::round(w), std::round(h));
    }

    QPoint tl = map(rect.topLeft());
    QPoint tr = map(QPoint(rect.x() + rect.width(), rect.y()));
    QPoint bl = map(QPoint(rect.x(), rect.y() + rect.height()));
    QPoint br = map(QPoint(rect.x() + rect.width(), rect.y() + rect.height()));

    int xmin = std::min(std::min(tl.x(), tr.x()), std::min(bl.x(), br.x()));
    int xmax = std::max(std::max(tl.x(), tr.x()), std::max(bl.x(), br.x()));
    int ymin = std::min(std::min(tl.y(), tr.y()), std::min(bl.y(), br.y()));
    int ymax = std::max(std::max(tl.y(), tr.y()), std::max(bl.y(), br.y()));

    return QRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

QRectF QMatrix4x4::mapRect(const QRectF &rect) const
{
    if (flagBits < Scale) {
        // Translation
        return rect.translated(m[3][0], m[3][1]);
    } else if (flagBits < Rotation2D) {
        // Translation | Scale
        float x = rect.x() * m[0][0] + m[3][0];
        float y = rect.y() * m[1][1] + m[3][1];
        float w = rect.width() * m[0][0];
        float h = rect.height() * m[1][1];
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        return QRectF(x, y, w, h);
    }

    QPointF tl = map(rect.topLeft());
    QPointF tr = map(rect.topRight());
    QPointF bl = map(rect.bottomLeft());
    QPointF br = map(rect.bottomRight());

    float xmin = std::min(std::min(tl.x(), tr.x()), std::min(bl.x(), br.x()));
    float xmax = std::max(std::max(tl.x(), tr.x()), std::max(bl.x(), br.x()));
    float ymin = std::min(std::min(tl.y(), tr.y()), std::min(bl.y(), br.y()));
    float ymax = std::max(std::max(tl.y(), tr.y()), std::max(bl.y(), br.y()));

    return QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
}

QMatrix4x4 QMatrix4x4::orthonormalInverse() const
{
    QMatrix4x4 result(1);// The '1' says not to load identity

    result.m[0][0] = m[0][0];
    result.m[1][0] = m[0][1];
    result.m[2][0] = m[0][2];

    result.m[0][1] = m[1][0];
    result.m[1][1] = m[1][1];
    result.m[2][1] = m[1][2];

    result.m[0][2] = m[2][0];
    result.m[1][2] = m[2][1];
    result.m[2][2] = m[2][2];

    result.m[0][3] = 0.0f;
    result.m[1][3] = 0.0f;
    result.m[2][3] = 0.0f;

    result.m[3][0] = -(result.m[0][0] * m[3][0] + result.m[1][0] * m[3][1] + result.m[2][0] * m[3][2]);
    result.m[3][1] = -(result.m[0][1] * m[3][0] + result.m[1][1] * m[3][1] + result.m[2][1] * m[3][2]);
    result.m[3][2] = -(result.m[0][2] * m[3][0] + result.m[1][2] * m[3][1] + result.m[2][2] * m[3][2]);
    result.m[3][3] = 1.0f;

    result.flagBits = flagBits;

    return result;
}

void QMatrix4x4::optimize()
{
    // If the last row is not (0, 0, 0, 1), the matrix is not a special type.
    flagBits = General;
    if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1) return;

    flagBits &= ~Perspective;

    // If the last column is (0, 0, 0, 1), then there is no translation.
    if (m[3][0] == 0 && m[3][1] == 0 && m[3][2] == 0) flagBits &= ~Translation;

    // If the two first elements of row 3 and column 3 are 0, then any rotation must be about Z.
    if (!m[0][2] && !m[1][2] && !m[2][0] && !m[2][1]) {
        flagBits &= ~Rotation;
        // If the six non-diagonal elements in the top left 3x3 matrix are 0, there is no rotation.
        if (!m[0][1] && !m[1][0]) {
            flagBits &= ~Rotation2D;
            // Check for identity.
            if (m[0][0] == 1 && m[1][1] == 1 && m[2][2] == 1) flagBits &= ~Scale;
        } else {
            // If the columns are orthonormal and form a right-handed system, then there is no scale.
            double mm[4][4];
            copyToDoubles(m, mm);
            double det = matrixDet2(mm, 0, 1, 0, 1);
            double lenX = mm[0][0] * mm[0][0] + mm[0][1] * mm[0][1];
            double lenY = mm[1][0] * mm[1][0] + mm[1][1] * mm[1][1];
            double lenZ = mm[2][2];
            if (qFuzzyCompare(det, 1.0) && qFuzzyCompare(lenX, 1.0) && qFuzzyCompare(lenY, 1.0) && qFuzzyCompare(lenZ, 1.0)) {
                flagBits &= ~Scale;
            }
        }
    } else {
        // If the columns are orthonormal and form a right-handed system, then there is no scale.
        double mm[4][4];
        copyToDoubles(m, mm);
        double det = matrixDet3(mm, 0, 1, 2, 0, 1, 2);
        double lenX = mm[0][0] * mm[0][0] + mm[0][1] * mm[0][1] + mm[0][2] * mm[0][2];
        double lenY = mm[1][0] * mm[1][0] + mm[1][1] * mm[1][1] + mm[1][2] * mm[1][2];
        double lenZ = mm[2][0] * mm[2][0] + mm[2][1] * mm[2][1] + mm[2][2] * mm[2][2];
        if (qFuzzyCompare(det, 1.0) && qFuzzyCompare(lenX, 1.0) && qFuzzyCompare(lenY, 1.0) && qFuzzyCompare(lenZ, 1.0)) {
            flagBits &= ~Scale;
        }
    }
}
