#ifndef QMATRIX4X4_H
#define QMATRIX4X4_H

#include "qrect.h"
#include "qgenericmatrix.h"
#include "qquaternion.h"
#include "qvector3d.h"
#include "qvector4d.h"

class QMatrix4x4 {
public:
    inline QMatrix4x4()
    {
        setToIdentity();
    }
    explicit QMatrix4x4(YPP::Initialization) : flagBits(General)
    {}
    explicit QMatrix4x4(const float *values);
    inline QMatrix4x4(float m11, float m12, float m13, float m14, float m21, float m22, float m23, float m24, float m31, float m32,
                      float m33, float m34, float m41, float m42, float m43, float m44);

    template<int N, int M>
    explicit QMatrix4x4(const QGenericMatrix<N, M, float> &matrix);

    QMatrix4x4(const float *values, int cols, int rows);

    inline const float &operator()(int row, int column) const;
    inline float &operator()(int row, int column);

    inline QVector4D column(int index) const;
    inline void setColumn(int index, const QVector4D &value);

    inline QVector4D row(int index) const;
    inline void setRow(int index, const QVector4D &value);

    inline bool isAffine() const;

    inline bool isIdentity() const;
    inline void setToIdentity();

    inline void fill(float value);

    double determinant() const;
    QMatrix4x4 inverted(bool *invertible = nullptr) const;
    QMatrix4x4 transposed() const;
    QMatrix3x3 normalMatrix() const;

    inline QMatrix4x4 &operator+=(const QMatrix4x4 &other);
    inline QMatrix4x4 &operator-=(const QMatrix4x4 &other);
    inline QMatrix4x4 &operator*=(const QMatrix4x4 &other);
    inline QMatrix4x4 &operator*=(float factor);
    QMatrix4x4 &operator/=(float divisor);
    inline bool operator==(const QMatrix4x4 &other) const;
    inline bool operator!=(const QMatrix4x4 &other) const;

    friend QMatrix4x4 operator+(const QMatrix4x4 &m1, const QMatrix4x4 &m2);
    friend QMatrix4x4 operator-(const QMatrix4x4 &m1, const QMatrix4x4 &m2);
    friend QMatrix4x4 operator*(const QMatrix4x4 &m1, const QMatrix4x4 &m2);

    friend QVector3D operator*(const QMatrix4x4 &matrix, const QVector3D &vector);
    friend QVector3D operator*(const QVector3D &vector, const QMatrix4x4 &matrix);

    friend QVector4D operator*(const QVector4D &vector, const QMatrix4x4 &matrix);
    friend QVector4D operator*(const QMatrix4x4 &matrix, const QVector4D &vector);

    friend QPoint operator*(const QPoint &point, const QMatrix4x4 &matrix);
    friend QPointF operator*(const QPointF &point, const QMatrix4x4 &matrix);
    friend QMatrix4x4 operator-(const QMatrix4x4 &matrix);
    friend QPoint operator*(const QMatrix4x4 &matrix, const QPoint &point);
    friend QPointF operator*(const QMatrix4x4 &matrix, const QPointF &point);
    friend QMatrix4x4 operator*(float factor, const QMatrix4x4 &matrix);
    friend QMatrix4x4 operator*(const QMatrix4x4 &matrix, float factor);
    friend QMatrix4x4 operator/(const QMatrix4x4 &matrix, float divisor);

    friend inline bool qFuzzyCompare(const QMatrix4x4 &m1, const QMatrix4x4 &m2);

    void scale(const QVector3D &vector);
    void translate(const QVector3D &vector);
    void rotate(float angle, const QVector3D &vector);

    void scale(float x, float y);
    void scale(float x, float y, float z);
    void scale(float factor);
    void translate(float x, float y);
    void translate(float x, float y, float z);
    void rotate(float angle, float x, float y, float z = 0.0f);

    void rotate(const QQuaternion &quaternion);


    void ortho(const QRect &rect);
    void ortho(const QRectF &rect);
    void ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void perspective(float verticalAngle, float aspectRatio, float nearPlane, float farPlane);

    void lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up);

    void viewport(const QRectF &rect);
    void viewport(float left, float bottom, float width, float height, float nearPlane = 0.0f, float farPlane = 1.0f);
    void flipCoordinates();

    void copyDataTo(float *values) const;

    QPoint map(const QPoint &point) const;
    QPointF map(const QPointF &point) const;

    QVector3D map(const QVector3D &point) const;
    QVector3D mapVector(const QVector3D &vector) const;

    QVector4D map(const QVector4D &point) const;

    QRect mapRect(const QRect &rect) const;
    QRectF mapRect(const QRectF &rect) const;

    template<int N, int M>
    QGenericMatrix<N, M, float> toGenericMatrix() const;

    inline float *data();
    inline const float *data() const
    {
        return *m;
    }
    inline const float *constData() const
    {
        return *m;
    }

    void optimize();

private:
    float m[4][4];// Column-major order to match OpenGL.
    int flagBits; // Flag bits from the enum below.

    // When matrices are multiplied, the flag bits are or-ed together.
    enum {
        Identity = 0x0000,   // Identity matrix
        Translation = 0x0001,// Contains a translation
        Scale = 0x0002,      // Contains a scale
        Rotation2D = 0x0004, // Contains a rotation about the Z axis
        Rotation = 0x0008,   // Contains an arbitrary rotation
        Perspective = 0x0010,// Last row is different from (0, 0, 0, 1)
        General = 0x001f     // General matrix, unknown contents
    };

    // Construct without initializing identity matrix.
    explicit QMatrix4x4(int)
    {}

    QMatrix4x4 orthonormalInverse() const;

    void projectedRotate(float angle, float x, float y, float z);

    friend class QGraphicsRotation;
};

inline QMatrix4x4::QMatrix4x4(float m11, float m12, float m13, float m14, float m21, float m22, float m23, float m24, float m31, float m32,
                              float m33, float m34, float m41, float m42, float m43, float m44)
{
    m[0][0] = m11;
    m[0][1] = m21;
    m[0][2] = m31;
    m[0][3] = m41;
    m[1][0] = m12;
    m[1][1] = m22;
    m[1][2] = m32;
    m[1][3] = m42;
    m[2][0] = m13;
    m[2][1] = m23;
    m[2][2] = m33;
    m[2][3] = m43;
    m[3][0] = m14;
    m[3][1] = m24;
    m[3][2] = m34;
    m[3][3] = m44;
    flagBits = General;
}

template<int N, int M>
inline QMatrix4x4::QMatrix4x4(const QGenericMatrix<N, M, float> &matrix)
{
    const float *values = matrix.constData();
    for (int matrixCol = 0; matrixCol < 4; ++matrixCol) {
        for (int matrixRow = 0; matrixRow < 4; ++matrixRow) {
            if (matrixCol < N && matrixRow < M)
                m[matrixCol][matrixRow] = values[matrixCol * M + matrixRow];
            else if (matrixCol == matrixRow)
                m[matrixCol][matrixRow] = 1.0f;
            else
                m[matrixCol][matrixRow] = 0.0f;
        }
    }
    flagBits = General;
}

template<int N, int M>
QGenericMatrix<N, M, float> QMatrix4x4::toGenericMatrix() const
{
    QGenericMatrix<N, M, float> result;
    float *values = result.data();
    for (int matrixCol = 0; matrixCol < N; ++matrixCol) {
        for (int matrixRow = 0; matrixRow < M; ++matrixRow) {
            if (matrixCol < 4 && matrixRow < 4)
                values[matrixCol * M + matrixRow] = m[matrixCol][matrixRow];
            else if (matrixCol == matrixRow)
                values[matrixCol * M + matrixRow] = 1.0f;
            else
                values[matrixCol * M + matrixRow] = 0.0f;
        }
    }
    return result;
}

inline const float &QMatrix4x4::operator()(int aRow, int aColumn) const
{
    assert(aRow >= 0 && aRow < 4 && aColumn >= 0 && aColumn < 4);
    return m[aColumn][aRow];
}

inline float &QMatrix4x4::operator()(int aRow, int aColumn)
{
    assert(aRow >= 0 && aRow < 4 && aColumn >= 0 && aColumn < 4);
    flagBits = General;
    return m[aColumn][aRow];
}

inline QVector4D QMatrix4x4::column(int index) const
{
    assert(index >= 0 && index < 4);
    return QVector4D(m[index][0], m[index][1], m[index][2], m[index][3]);
}

inline void QMatrix4x4::setColumn(int index, const QVector4D &value)
{
    assert(index >= 0 && index < 4);
    m[index][0] = value.x();
    m[index][1] = value.y();
    m[index][2] = value.z();
    m[index][3] = value.w();
    flagBits = General;
}

inline QVector4D QMatrix4x4::row(int index) const
{
    assert(index >= 0 && index < 4);
    return QVector4D(m[0][index], m[1][index], m[2][index], m[3][index]);
}

inline void QMatrix4x4::setRow(int index, const QVector4D &value)
{
    assert(index >= 0 && index < 4);
    m[0][index] = value.x();
    m[1][index] = value.y();
    m[2][index] = value.z();
    m[3][index] = value.w();
    flagBits = General;
}

QMatrix4x4 operator/(const QMatrix4x4 &matrix, float divisor);

inline bool QMatrix4x4::isAffine() const
{
    return m[0][3] == 0.0f && m[1][3] == 0.0f && m[2][3] == 0.0f && m[3][3] == 1.0f;
}

inline bool QMatrix4x4::isIdentity() const
{
    if (flagBits == Identity) return true;
    if (m[0][0] != 1.0f || m[0][1] != 0.0f || m[0][2] != 0.0f) return false;
    if (m[0][3] != 0.0f || m[1][0] != 0.0f || m[1][1] != 1.0f) return false;
    if (m[1][2] != 0.0f || m[1][3] != 0.0f || m[2][0] != 0.0f) return false;
    if (m[2][1] != 0.0f || m[2][2] != 1.0f || m[2][3] != 0.0f) return false;
    if (m[3][0] != 0.0f || m[3][1] != 0.0f || m[3][2] != 0.0f) return false;
    return (m[3][3] == 1.0f);
}

inline void QMatrix4x4::setToIdentity()
{
    m[0][0] = 1.0f;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = 1.0f;
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 1.0f;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
    flagBits = Identity;
}

inline void QMatrix4x4::fill(float value)
{
    m[0][0] = value;
    m[0][1] = value;
    m[0][2] = value;
    m[0][3] = value;
    m[1][0] = value;
    m[1][1] = value;
    m[1][2] = value;
    m[1][3] = value;
    m[2][0] = value;
    m[2][1] = value;
    m[2][2] = value;
    m[2][3] = value;
    m[3][0] = value;
    m[3][1] = value;
    m[3][2] = value;
    m[3][3] = value;
    flagBits = General;
}

inline QMatrix4x4 &QMatrix4x4::operator+=(const QMatrix4x4 &other)
{
    m[0][0] += other.m[0][0];
    m[0][1] += other.m[0][1];
    m[0][2] += other.m[0][2];
    m[0][3] += other.m[0][3];
    m[1][0] += other.m[1][0];
    m[1][1] += other.m[1][1];
    m[1][2] += other.m[1][2];
    m[1][3] += other.m[1][3];
    m[2][0] += other.m[2][0];
    m[2][1] += other.m[2][1];
    m[2][2] += other.m[2][2];
    m[2][3] += other.m[2][3];
    m[3][0] += other.m[3][0];
    m[3][1] += other.m[3][1];
    m[3][2] += other.m[3][2];
    m[3][3] += other.m[3][3];
    flagBits = General;
    return *this;
}

inline QMatrix4x4 &QMatrix4x4::operator-=(const QMatrix4x4 &other)
{
    m[0][0] -= other.m[0][0];
    m[0][1] -= other.m[0][1];
    m[0][2] -= other.m[0][2];
    m[0][3] -= other.m[0][3];
    m[1][0] -= other.m[1][0];
    m[1][1] -= other.m[1][1];
    m[1][2] -= other.m[1][2];
    m[1][3] -= other.m[1][3];
    m[2][0] -= other.m[2][0];
    m[2][1] -= other.m[2][1];
    m[2][2] -= other.m[2][2];
    m[2][3] -= other.m[2][3];
    m[3][0] -= other.m[3][0];
    m[3][1] -= other.m[3][1];
    m[3][2] -= other.m[3][2];
    m[3][3] -= other.m[3][3];
    flagBits = General;
    return *this;
}

inline QMatrix4x4 &QMatrix4x4::operator*=(const QMatrix4x4 &o)
{
    const QMatrix4x4 other = o;// prevent aliasing when &o == this ### Qt 6: take o by value
    flagBits |= other.flagBits;

    if (flagBits < Rotation2D) {
        m[3][0] += m[0][0] * other.m[3][0];
        m[3][1] += m[1][1] * other.m[3][1];
        m[3][2] += m[2][2] * other.m[3][2];

        m[0][0] *= other.m[0][0];
        m[1][1] *= other.m[1][1];
        m[2][2] *= other.m[2][2];
        return *this;
    }

    float m0, m1, m2;
    m0 = m[0][0] * other.m[0][0] + m[1][0] * other.m[0][1] + m[2][0] * other.m[0][2] + m[3][0] * other.m[0][3];
    m1 = m[0][0] * other.m[1][0] + m[1][0] * other.m[1][1] + m[2][0] * other.m[1][2] + m[3][0] * other.m[1][3];
    m2 = m[0][0] * other.m[2][0] + m[1][0] * other.m[2][1] + m[2][0] * other.m[2][2] + m[3][0] * other.m[2][3];
    m[3][0] = m[0][0] * other.m[3][0] + m[1][0] * other.m[3][1] + m[2][0] * other.m[3][2] + m[3][0] * other.m[3][3];
    m[0][0] = m0;
    m[1][0] = m1;
    m[2][0] = m2;

    m0 = m[0][1] * other.m[0][0] + m[1][1] * other.m[0][1] + m[2][1] * other.m[0][2] + m[3][1] * other.m[0][3];
    m1 = m[0][1] * other.m[1][0] + m[1][1] * other.m[1][1] + m[2][1] * other.m[1][2] + m[3][1] * other.m[1][3];
    m2 = m[0][1] * other.m[2][0] + m[1][1] * other.m[2][1] + m[2][1] * other.m[2][2] + m[3][1] * other.m[2][3];
    m[3][1] = m[0][1] * other.m[3][0] + m[1][1] * other.m[3][1] + m[2][1] * other.m[3][2] + m[3][1] * other.m[3][3];
    m[0][1] = m0;
    m[1][1] = m1;
    m[2][1] = m2;

    m0 = m[0][2] * other.m[0][0] + m[1][2] * other.m[0][1] + m[2][2] * other.m[0][2] + m[3][2] * other.m[0][3];
    m1 = m[0][2] * other.m[1][0] + m[1][2] * other.m[1][1] + m[2][2] * other.m[1][2] + m[3][2] * other.m[1][3];
    m2 = m[0][2] * other.m[2][0] + m[1][2] * other.m[2][1] + m[2][2] * other.m[2][2] + m[3][2] * other.m[2][3];
    m[3][2] = m[0][2] * other.m[3][0] + m[1][2] * other.m[3][1] + m[2][2] * other.m[3][2] + m[3][2] * other.m[3][3];
    m[0][2] = m0;
    m[1][2] = m1;
    m[2][2] = m2;

    m0 = m[0][3] * other.m[0][0] + m[1][3] * other.m[0][1] + m[2][3] * other.m[0][2] + m[3][3] * other.m[0][3];
    m1 = m[0][3] * other.m[1][0] + m[1][3] * other.m[1][1] + m[2][3] * other.m[1][2] + m[3][3] * other.m[1][3];
    m2 = m[0][3] * other.m[2][0] + m[1][3] * other.m[2][1] + m[2][3] * other.m[2][2] + m[3][3] * other.m[2][3];
    m[3][3] = m[0][3] * other.m[3][0] + m[1][3] * other.m[3][1] + m[2][3] * other.m[3][2] + m[3][3] * other.m[3][3];
    m[0][3] = m0;
    m[1][3] = m1;
    m[2][3] = m2;
    return *this;
}

inline QMatrix4x4 &QMatrix4x4::operator*=(float factor)
{
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
    m[3][0] *= factor;
    m[3][1] *= factor;
    m[3][2] *= factor;
    m[3][3] *= factor;
    flagBits = General;
    return *this;
}

inline bool QMatrix4x4::operator==(const QMatrix4x4 &other) const
{
    return m[0][0] == other.m[0][0] && m[0][1] == other.m[0][1] && m[0][2] == other.m[0][2] && m[0][3] == other.m[0][3] &&
           m[1][0] == other.m[1][0] && m[1][1] == other.m[1][1] && m[1][2] == other.m[1][2] && m[1][3] == other.m[1][3] &&
           m[2][0] == other.m[2][0] && m[2][1] == other.m[2][1] && m[2][2] == other.m[2][2] && m[2][3] == other.m[2][3] &&
           m[3][0] == other.m[3][0] && m[3][1] == other.m[3][1] && m[3][2] == other.m[3][2] && m[3][3] == other.m[3][3];
}

inline bool QMatrix4x4::operator!=(const QMatrix4x4 &other) const
{
    return m[0][0] != other.m[0][0] || m[0][1] != other.m[0][1] || m[0][2] != other.m[0][2] || m[0][3] != other.m[0][3] ||
           m[1][0] != other.m[1][0] || m[1][1] != other.m[1][1] || m[1][2] != other.m[1][2] || m[1][3] != other.m[1][3] ||
           m[2][0] != other.m[2][0] || m[2][1] != other.m[2][1] || m[2][2] != other.m[2][2] || m[2][3] != other.m[2][3] ||
           m[3][0] != other.m[3][0] || m[3][1] != other.m[3][1] || m[3][2] != other.m[3][2] || m[3][3] != other.m[3][3];
}

inline QMatrix4x4 operator+(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
    QMatrix4x4 m(1);
    m.m[0][0] = m1.m[0][0] + m2.m[0][0];
    m.m[0][1] = m1.m[0][1] + m2.m[0][1];
    m.m[0][2] = m1.m[0][2] + m2.m[0][2];
    m.m[0][3] = m1.m[0][3] + m2.m[0][3];
    m.m[1][0] = m1.m[1][0] + m2.m[1][0];
    m.m[1][1] = m1.m[1][1] + m2.m[1][1];
    m.m[1][2] = m1.m[1][2] + m2.m[1][2];
    m.m[1][3] = m1.m[1][3] + m2.m[1][3];
    m.m[2][0] = m1.m[2][0] + m2.m[2][0];
    m.m[2][1] = m1.m[2][1] + m2.m[2][1];
    m.m[2][2] = m1.m[2][2] + m2.m[2][2];
    m.m[2][3] = m1.m[2][3] + m2.m[2][3];
    m.m[3][0] = m1.m[3][0] + m2.m[3][0];
    m.m[3][1] = m1.m[3][1] + m2.m[3][1];
    m.m[3][2] = m1.m[3][2] + m2.m[3][2];
    m.m[3][3] = m1.m[3][3] + m2.m[3][3];
    m.flagBits = QMatrix4x4::General;
    return m;
}

inline QMatrix4x4 operator-(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
    QMatrix4x4 m(1);
    m.m[0][0] = m1.m[0][0] - m2.m[0][0];
    m.m[0][1] = m1.m[0][1] - m2.m[0][1];
    m.m[0][2] = m1.m[0][2] - m2.m[0][2];
    m.m[0][3] = m1.m[0][3] - m2.m[0][3];
    m.m[1][0] = m1.m[1][0] - m2.m[1][0];
    m.m[1][1] = m1.m[1][1] - m2.m[1][1];
    m.m[1][2] = m1.m[1][2] - m2.m[1][2];
    m.m[1][3] = m1.m[1][3] - m2.m[1][3];
    m.m[2][0] = m1.m[2][0] - m2.m[2][0];
    m.m[2][1] = m1.m[2][1] - m2.m[2][1];
    m.m[2][2] = m1.m[2][2] - m2.m[2][2];
    m.m[2][3] = m1.m[2][3] - m2.m[2][3];
    m.m[3][0] = m1.m[3][0] - m2.m[3][0];
    m.m[3][1] = m1.m[3][1] - m2.m[3][1];
    m.m[3][2] = m1.m[3][2] - m2.m[3][2];
    m.m[3][3] = m1.m[3][3] - m2.m[3][3];
    m.flagBits = QMatrix4x4::General;
    return m;
}

inline QMatrix4x4 operator*(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
    int flagBits = m1.flagBits | m2.flagBits;
    if (flagBits < QMatrix4x4::Rotation2D) {
        QMatrix4x4 m = m1;
        m.m[3][0] += m.m[0][0] * m2.m[3][0];
        m.m[3][1] += m.m[1][1] * m2.m[3][1];
        m.m[3][2] += m.m[2][2] * m2.m[3][2];

        m.m[0][0] *= m2.m[0][0];
        m.m[1][1] *= m2.m[1][1];
        m.m[2][2] *= m2.m[2][2];
        m.flagBits = flagBits;
        return m;
    }

    QMatrix4x4 m(1);
    m.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2] + m1.m[3][0] * m2.m[0][3];
    m.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2] + m1.m[3][1] * m2.m[0][3];
    m.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2] + m1.m[3][2] * m2.m[0][3];
    m.m[0][3] = m1.m[0][3] * m2.m[0][0] + m1.m[1][3] * m2.m[0][1] + m1.m[2][3] * m2.m[0][2] + m1.m[3][3] * m2.m[0][3];

    m.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2] + m1.m[3][0] * m2.m[1][3];
    m.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2] + m1.m[3][1] * m2.m[1][3];
    m.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2] + m1.m[3][2] * m2.m[1][3];
    m.m[1][3] = m1.m[0][3] * m2.m[1][0] + m1.m[1][3] * m2.m[1][1] + m1.m[2][3] * m2.m[1][2] + m1.m[3][3] * m2.m[1][3];

    m.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2] + m1.m[3][0] * m2.m[2][3];
    m.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2] + m1.m[3][1] * m2.m[2][3];
    m.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2] + m1.m[3][2] * m2.m[2][3];
    m.m[2][3] = m1.m[0][3] * m2.m[2][0] + m1.m[1][3] * m2.m[2][1] + m1.m[2][3] * m2.m[2][2] + m1.m[3][3] * m2.m[2][3];

    m.m[3][0] = m1.m[0][0] * m2.m[3][0] + m1.m[1][0] * m2.m[3][1] + m1.m[2][0] * m2.m[3][2] + m1.m[3][0] * m2.m[3][3];
    m.m[3][1] = m1.m[0][1] * m2.m[3][0] + m1.m[1][1] * m2.m[3][1] + m1.m[2][1] * m2.m[3][2] + m1.m[3][1] * m2.m[3][3];
    m.m[3][2] = m1.m[0][2] * m2.m[3][0] + m1.m[1][2] * m2.m[3][1] + m1.m[2][2] * m2.m[3][2] + m1.m[3][2] * m2.m[3][3];
    m.m[3][3] = m1.m[0][3] * m2.m[3][0] + m1.m[1][3] * m2.m[3][1] + m1.m[2][3] * m2.m[3][2] + m1.m[3][3] * m2.m[3][3];
    m.flagBits = flagBits;
    return m;
}

inline QVector3D operator*(const QVector3D &vector, const QMatrix4x4 &matrix)
{
    float x, y, z, w;
    x = vector.x() * matrix.m[0][0] + vector.y() * matrix.m[0][1] + vector.z() * matrix.m[0][2] + matrix.m[0][3];
    y = vector.x() * matrix.m[1][0] + vector.y() * matrix.m[1][1] + vector.z() * matrix.m[1][2] + matrix.m[1][3];
    z = vector.x() * matrix.m[2][0] + vector.y() * matrix.m[2][1] + vector.z() * matrix.m[2][2] + matrix.m[2][3];
    w = vector.x() * matrix.m[3][0] + vector.y() * matrix.m[3][1] + vector.z() * matrix.m[3][2] + matrix.m[3][3];
    if (w == 1.0f)
        return QVector3D(x, y, z);
    else
        return QVector3D(x / w, y / w, z / w);
}

inline QVector3D operator*(const QMatrix4x4 &matrix, const QVector3D &vector)
{
    float x, y, z, w;
    if (matrix.flagBits == QMatrix4x4::Identity) {
        return vector;
    } else if (matrix.flagBits < QMatrix4x4::Rotation2D) {
        // Translation | Scale
        return QVector3D(vector.x() * matrix.m[0][0] + matrix.m[3][0], vector.y() * matrix.m[1][1] + matrix.m[3][1],
                         vector.z() * matrix.m[2][2] + matrix.m[3][2]);
    } else if (matrix.flagBits < QMatrix4x4::Rotation) {
        // Translation | Scale | Rotation2D
        return QVector3D(vector.x() * matrix.m[0][0] + vector.y() * matrix.m[1][0] + matrix.m[3][0],
                         vector.x() * matrix.m[0][1] + vector.y() * matrix.m[1][1] + matrix.m[3][1],
                         vector.z() * matrix.m[2][2] + matrix.m[3][2]);
    } else {
        x = vector.x() * matrix.m[0][0] + vector.y() * matrix.m[1][0] + vector.z() * matrix.m[2][0] + matrix.m[3][0];
        y = vector.x() * matrix.m[0][1] + vector.y() * matrix.m[1][1] + vector.z() * matrix.m[2][1] + matrix.m[3][1];
        z = vector.x() * matrix.m[0][2] + vector.y() * matrix.m[1][2] + vector.z() * matrix.m[2][2] + matrix.m[3][2];
        w = vector.x() * matrix.m[0][3] + vector.y() * matrix.m[1][3] + vector.z() * matrix.m[2][3] + matrix.m[3][3];
        if (w == 1.0f)
            return QVector3D(x, y, z);
        else
            return QVector3D(x / w, y / w, z / w);
    }
}

inline QVector4D operator*(const QVector4D &vector, const QMatrix4x4 &matrix)
{
    float x, y, z, w;
    x = vector.x() * matrix.m[0][0] + vector.y() * matrix.m[0][1] + vector.z() * matrix.m[0][2] + vector.w() * matrix.m[0][3];
    y = vector.x() * matrix.m[1][0] + vector.y() * matrix.m[1][1] + vector.z() * matrix.m[1][2] + vector.w() * matrix.m[1][3];
    z = vector.x() * matrix.m[2][0] + vector.y() * matrix.m[2][1] + vector.z() * matrix.m[2][2] + vector.w() * matrix.m[2][3];
    w = vector.x() * matrix.m[3][0] + vector.y() * matrix.m[3][1] + vector.z() * matrix.m[3][2] + vector.w() * matrix.m[3][3];
    return QVector4D(x, y, z, w);
}

inline QVector4D operator*(const QMatrix4x4 &matrix, const QVector4D &vector)
{
    float x, y, z, w;
    x = vector.x() * matrix.m[0][0] + vector.y() * matrix.m[1][0] + vector.z() * matrix.m[2][0] + vector.w() * matrix.m[3][0];
    y = vector.x() * matrix.m[0][1] + vector.y() * matrix.m[1][1] + vector.z() * matrix.m[2][1] + vector.w() * matrix.m[3][1];
    z = vector.x() * matrix.m[0][2] + vector.y() * matrix.m[1][2] + vector.z() * matrix.m[2][2] + vector.w() * matrix.m[3][2];
    w = vector.x() * matrix.m[0][3] + vector.y() * matrix.m[1][3] + vector.z() * matrix.m[2][3] + vector.w() * matrix.m[3][3];
    return QVector4D(x, y, z, w);
}

inline QPoint operator*(const QPoint &point, const QMatrix4x4 &matrix)
{
    float xin, yin;
    float x, y, w;
    xin = point.x();
    yin = point.y();
    x = xin * matrix.m[0][0] + yin * matrix.m[0][1] + matrix.m[0][3];
    y = xin * matrix.m[1][0] + yin * matrix.m[1][1] + matrix.m[1][3];
    w = xin * matrix.m[3][0] + yin * matrix.m[3][1] + matrix.m[3][3];
    if (w == 1.0f)
        return QPoint(std::round(x), std::round(y));
    else
        return QPoint(std::round(x / w), std::round(y / w));
}

inline QPointF operator*(const QPointF &point, const QMatrix4x4 &matrix)
{
    float xin, yin;
    float x, y, w;
    xin = float(point.x());
    yin = float(point.y());
    x = xin * matrix.m[0][0] + yin * matrix.m[0][1] + matrix.m[0][3];
    y = xin * matrix.m[1][0] + yin * matrix.m[1][1] + matrix.m[1][3];
    w = xin * matrix.m[3][0] + yin * matrix.m[3][1] + matrix.m[3][3];
    if (w == 1.0f) {
        return QPointF(double(x), double(y));
    } else {
        return QPointF(double(x / w), double(y / w));
    }
}

inline QPoint operator*(const QMatrix4x4 &matrix, const QPoint &point)
{
    float xin, yin;
    float x, y, w;
    xin = point.x();
    yin = point.y();
    if (matrix.flagBits == QMatrix4x4::Identity) {
        return point;
    } else if (matrix.flagBits < QMatrix4x4::Rotation2D) {
        // Translation | Scale
        return QPoint(std::round(xin * matrix.m[0][0] + matrix.m[3][0]), std::round(yin * matrix.m[1][1] + matrix.m[3][1]));
    } else if (matrix.flagBits < QMatrix4x4::Perspective) {
        return QPoint(std::round(xin * matrix.m[0][0] + yin * matrix.m[1][0] + matrix.m[3][0]),
                      std::round(xin * matrix.m[0][1] + yin * matrix.m[1][1] + matrix.m[3][1]));
    } else {
        x = xin * matrix.m[0][0] + yin * matrix.m[1][0] + matrix.m[3][0];
        y = xin * matrix.m[0][1] + yin * matrix.m[1][1] + matrix.m[3][1];
        w = xin * matrix.m[0][3] + yin * matrix.m[1][3] + matrix.m[3][3];
        if (w == 1.0f)
            return QPoint(std::round(x), std::round(y));
        else
            return QPoint(std::round(x / w), std::round(y / w));
    }
}

inline QPointF operator*(const QMatrix4x4 &matrix, const QPointF &point)
{
    double xin, yin;
    double x, y, w;
    xin = point.x();
    yin = point.y();
    if (matrix.flagBits == QMatrix4x4::Identity) {
        return point;
    } else if (matrix.flagBits < QMatrix4x4::Rotation2D) {
        // Translation | Scale
        return QPointF(xin * double(matrix.m[0][0]) + double(matrix.m[3][0]), yin *double(matrix.m[1][1]) + double(matrix.m[3][1]));
    } else if (matrix.flagBits < QMatrix4x4::Perspective) {
        return QPointF(xin * double(matrix.m[0][0]) + yin * double(matrix.m[1][0]) + double(matrix.m[3][0]),
                       xin *double(matrix.m[0][1]) + yin * double(matrix.m[1][1]) + double(matrix.m[3][1]));
    } else {
        x = xin * double(matrix.m[0][0]) + yin * double(matrix.m[1][0]) + double(matrix.m[3][0]);
        y = xin * double(matrix.m[0][1]) + yin * double(matrix.m[1][1]) + double(matrix.m[3][1]);
        w = xin * double(matrix.m[0][3]) + yin * double(matrix.m[1][3]) + double(matrix.m[3][3]);
        if (w == 1.0) {
            return QPointF(double(x), double(y));
        } else {
            return QPointF(double(x / w), double(y / w));
        }
    }
}

inline QMatrix4x4 operator-(const QMatrix4x4 &matrix)
{
    QMatrix4x4 m(1);
    m.m[0][0] = -matrix.m[0][0];
    m.m[0][1] = -matrix.m[0][1];
    m.m[0][2] = -matrix.m[0][2];
    m.m[0][3] = -matrix.m[0][3];
    m.m[1][0] = -matrix.m[1][0];
    m.m[1][1] = -matrix.m[1][1];
    m.m[1][2] = -matrix.m[1][2];
    m.m[1][3] = -matrix.m[1][3];
    m.m[2][0] = -matrix.m[2][0];
    m.m[2][1] = -matrix.m[2][1];
    m.m[2][2] = -matrix.m[2][2];
    m.m[2][3] = -matrix.m[2][3];
    m.m[3][0] = -matrix.m[3][0];
    m.m[3][1] = -matrix.m[3][1];
    m.m[3][2] = -matrix.m[3][2];
    m.m[3][3] = -matrix.m[3][3];
    m.flagBits = QMatrix4x4::General;
    return m;
}

inline QMatrix4x4 operator*(float factor, const QMatrix4x4 &matrix)
{
    QMatrix4x4 m(1);
    m.m[0][0] = matrix.m[0][0] * factor;
    m.m[0][1] = matrix.m[0][1] * factor;
    m.m[0][2] = matrix.m[0][2] * factor;
    m.m[0][3] = matrix.m[0][3] * factor;
    m.m[1][0] = matrix.m[1][0] * factor;
    m.m[1][1] = matrix.m[1][1] * factor;
    m.m[1][2] = matrix.m[1][2] * factor;
    m.m[1][3] = matrix.m[1][3] * factor;
    m.m[2][0] = matrix.m[2][0] * factor;
    m.m[2][1] = matrix.m[2][1] * factor;
    m.m[2][2] = matrix.m[2][2] * factor;
    m.m[2][3] = matrix.m[2][3] * factor;
    m.m[3][0] = matrix.m[3][0] * factor;
    m.m[3][1] = matrix.m[3][1] * factor;
    m.m[3][2] = matrix.m[3][2] * factor;
    m.m[3][3] = matrix.m[3][3] * factor;
    m.flagBits = QMatrix4x4::General;
    return m;
}

inline QMatrix4x4 operator*(const QMatrix4x4 &matrix, float factor)
{
    QMatrix4x4 m(1);
    m.m[0][0] = matrix.m[0][0] * factor;
    m.m[0][1] = matrix.m[0][1] * factor;
    m.m[0][2] = matrix.m[0][2] * factor;
    m.m[0][3] = matrix.m[0][3] * factor;
    m.m[1][0] = matrix.m[1][0] * factor;
    m.m[1][1] = matrix.m[1][1] * factor;
    m.m[1][2] = matrix.m[1][2] * factor;
    m.m[1][3] = matrix.m[1][3] * factor;
    m.m[2][0] = matrix.m[2][0] * factor;
    m.m[2][1] = matrix.m[2][1] * factor;
    m.m[2][2] = matrix.m[2][2] * factor;
    m.m[2][3] = matrix.m[2][3] * factor;
    m.m[3][0] = matrix.m[3][0] * factor;
    m.m[3][1] = matrix.m[3][1] * factor;
    m.m[3][2] = matrix.m[3][2] * factor;
    m.m[3][3] = matrix.m[3][3] * factor;
    m.flagBits = QMatrix4x4::General;
    return m;
}

inline bool qFuzzyCompare(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
    return qFuzzyCompare(m1.m[0][0], m2.m[0][0]) && qFuzzyCompare(m1.m[0][1], m2.m[0][1]) && qFuzzyCompare(m1.m[0][2], m2.m[0][2]) &&
           qFuzzyCompare(m1.m[0][3], m2.m[0][3]) && qFuzzyCompare(m1.m[1][0], m2.m[1][0]) && qFuzzyCompare(m1.m[1][1], m2.m[1][1]) &&
           qFuzzyCompare(m1.m[1][2], m2.m[1][2]) && qFuzzyCompare(m1.m[1][3], m2.m[1][3]) && qFuzzyCompare(m1.m[2][0], m2.m[2][0]) &&
           qFuzzyCompare(m1.m[2][1], m2.m[2][1]) && qFuzzyCompare(m1.m[2][2], m2.m[2][2]) && qFuzzyCompare(m1.m[2][3], m2.m[2][3]) &&
           qFuzzyCompare(m1.m[3][0], m2.m[3][0]) && qFuzzyCompare(m1.m[3][1], m2.m[3][1]) && qFuzzyCompare(m1.m[3][2], m2.m[3][2]) &&
           qFuzzyCompare(m1.m[3][3], m2.m[3][3]);
}

inline QPoint QMatrix4x4::map(const QPoint &point) const
{
    return *this * point;
}

inline QPointF QMatrix4x4::map(const QPointF &point) const
{
    return *this * point;
}

inline QVector3D QMatrix4x4::map(const QVector3D &point) const
{
    return *this * point;
}

inline QVector3D QMatrix4x4::mapVector(const QVector3D &vector) const
{
    if (flagBits < Scale) {
        // Translation
        return vector;
    } else if (flagBits < Rotation2D) {
        // Translation | Scale
        return QVector3D(vector.x() * m[0][0], vector.y() * m[1][1], vector.z() * m[2][2]);
    } else {
        return QVector3D(vector.x() * m[0][0] + vector.y() * m[1][0] + vector.z() * m[2][0],
                         vector.x() * m[0][1] + vector.y() * m[1][1] + vector.z() * m[2][1],
                         vector.x() * m[0][2] + vector.y() * m[1][2] + vector.z() * m[2][2]);
    }
}

inline QVector4D QMatrix4x4::map(const QVector4D &point) const
{
    return *this * point;
}

inline float *QMatrix4x4::data()
{
    // We have to assume that the caller will modify the matrix elements,
    // so we flip it over to "General" mode.
    flagBits = General;
    return *m;
}

inline void QMatrix4x4::viewport(const QRectF &rect)
{
    viewport(float(rect.x()), float(rect.y()), float(rect.width()), float(rect.height()));
}

#endif
