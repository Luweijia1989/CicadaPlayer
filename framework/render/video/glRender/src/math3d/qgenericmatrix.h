#ifndef QGENERICMATRIX_H
#define QGENERICMATRIX_H

#include <cassert>
#include "common.h"

template<int N, int M, typename T>
class QGenericMatrix {
public:
    QGenericMatrix();
    explicit QGenericMatrix(YPP::Initialization)
    {}
    QGenericMatrix(const T *values);

    const T &operator()(int row, int column) const;
    T &operator()(int row, int column);

    bool isIdentity() const;
    void setToIdentity();

    void fill(T value);

    QGenericMatrix<M, N, T> transposed() const;

    QGenericMatrix<N, M, T> &operator+=(const QGenericMatrix<N, M, T> &other);
    QGenericMatrix<N, M, T> &operator-=(const QGenericMatrix<N, M, T> &other);
    QGenericMatrix<N, M, T> &operator*=(T factor);
    QGenericMatrix<N, M, T> &operator/=(T divisor);
    bool operator==(const QGenericMatrix<N, M, T> &other) const;
    bool operator!=(const QGenericMatrix<N, M, T> &other) const;

    void copyDataTo(T *values) const;

    T *data()
    {
        return *m;
    }
    const T *data() const
    {
        return *m;
    }
    const T *constData() const
    {
        return *m;
    }

    template<int NN, int MM, typename TT>
    friend QGenericMatrix<NN, MM, TT> operator+(const QGenericMatrix<NN, MM, TT> &m1, const QGenericMatrix<NN, MM, TT> &m2);
    template<int NN, int MM, typename TT>
    friend QGenericMatrix<NN, MM, TT> operator-(const QGenericMatrix<NN, MM, TT> &m1, const QGenericMatrix<NN, MM, TT> &m2);
    template<int NN, int M1, int M2, typename TT>
    friend QGenericMatrix<M1, M2, TT> operator*(const QGenericMatrix<NN, M2, TT> &m1, const QGenericMatrix<M1, NN, TT> &m2);
    template<int NN, int MM, typename TT>
    friend QGenericMatrix<NN, MM, TT> operator-(const QGenericMatrix<NN, MM, TT> &matrix);
    template<int NN, int MM, typename TT>
    friend QGenericMatrix<NN, MM, TT> operator*(TT factor, const QGenericMatrix<NN, MM, TT> &matrix);
    template<int NN, int MM, typename TT>
    friend QGenericMatrix<NN, MM, TT> operator*(const QGenericMatrix<NN, MM, TT> &matrix, TT factor);
    template<int NN, int MM, typename TT>
    friend QGenericMatrix<NN, MM, TT> operator/(const QGenericMatrix<NN, MM, TT> &matrix, TT divisor);

private:
    T m[N][M];// Column-major order to match OpenGL.

    template<int NN, int MM, typename TT>
    friend class QGenericMatrix;
};

template<int N, int M, typename T>
inline QGenericMatrix<N, M, T>::QGenericMatrix()
{
    setToIdentity();
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T>::QGenericMatrix(const T *values)
{
    for (int col = 0; col < N; ++col)
        for (int row = 0; row < M; ++row) m[col][row] = values[row * N + col];
}

template<int N, int M, typename T>
inline const T &QGenericMatrix<N, M, T>::operator()(int row, int column) const
{
    assert(row >= 0 && row < M && column >= 0 && column < N);
    return m[column][row];
}

template<int N, int M, typename T>
inline T &QGenericMatrix<N, M, T>::operator()(int row, int column)
{
    assert(row >= 0 && row < M && column >= 0 && column < N);
    return m[column][row];
}

template<int N, int M, typename T>
bool QGenericMatrix<N, M, T>::isIdentity() const
{
    for (int col = 0; col < N; ++col) {
        for (int row = 0; row < M; ++row) {
            if (row == col) {
                if (m[col][row] != 1.0f) return false;
            } else {
                if (m[col][row] != 0.0f) return false;
            }
        }
    }
    return true;
}

template<int N, int M, typename T>
void QGenericMatrix<N, M, T>::setToIdentity()
{
    for (int col = 0; col < N; ++col) {
        for (int row = 0; row < M; ++row) {
            if (row == col)
                m[col][row] = 1.0f;
            else
                m[col][row] = 0.0f;
        }
    }
}

template<int N, int M, typename T>
void QGenericMatrix<N, M, T>::fill(T value)
{
    for (int col = 0; col < N; ++col)
        for (int row = 0; row < M; ++row) m[col][row] = value;
}

template<int N, int M, typename T>
QGenericMatrix<M, N, T> QGenericMatrix<N, M, T>::transposed() const
{
    QGenericMatrix<M, N, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[row][col] = m[col][row];
    return result;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> &QGenericMatrix<N, M, T>::operator+=(const QGenericMatrix<N, M, T> &other)
{
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) m[col][row] += other.m[col][row];
    return *this;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> &QGenericMatrix<N, M, T>::operator-=(const QGenericMatrix<N, M, T> &other)
{
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) m[col][row] -= other.m[col][row];
    return *this;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> &QGenericMatrix<N, M, T>::operator*=(T factor)
{
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) m[col][row] *= factor;
    return *this;
}

template<int N, int M, typename T>
bool QGenericMatrix<N, M, T>::operator==(const QGenericMatrix<N, M, T> &other) const
{
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) {
            if (m[col][row] != other.m[col][row]) return false;
        }
    return true;
}

template<int N, int M, typename T>
bool QGenericMatrix<N, M, T>::operator!=(const QGenericMatrix<N, M, T> &other) const
{
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) {
            if (m[col][row] != other.m[col][row]) return true;
        }
    return false;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> &QGenericMatrix<N, M, T>::operator/=(T divisor)
{
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) m[col][row] /= divisor;
    return *this;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> operator+(const QGenericMatrix<N, M, T> &m1, const QGenericMatrix<N, M, T> &m2)
{
    QGenericMatrix<N, M, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[col][row] = m1.m[col][row] + m2.m[col][row];
    return result;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> operator-(const QGenericMatrix<N, M, T> &m1, const QGenericMatrix<N, M, T> &m2)
{
    QGenericMatrix<N, M, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[col][row] = m1.m[col][row] - m2.m[col][row];
    return result;
}

template<int N, int M1, int M2, typename T>
QGenericMatrix<M1, M2, T> operator*(const QGenericMatrix<N, M2, T> &m1, const QGenericMatrix<M1, N, T> &m2)
{
    QGenericMatrix<M1, M2, T> result(Qt::Uninitialized);
    for (int row = 0; row < M2; ++row) {
        for (int col = 0; col < M1; ++col) {
            T sum(0.0f);
            for (int j = 0; j < N; ++j) sum += m1.m[j][row] * m2.m[col][j];
            result.m[col][row] = sum;
        }
    }
    return result;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> operator-(const QGenericMatrix<N, M, T> &matrix)
{
    QGenericMatrix<N, M, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[col][row] = -matrix.m[col][row];
    return result;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> operator*(T factor, const QGenericMatrix<N, M, T> &matrix)
{
    QGenericMatrix<N, M, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[col][row] = matrix.m[col][row] * factor;
    return result;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> operator*(const QGenericMatrix<N, M, T> &matrix, T factor)
{
    QGenericMatrix<N, M, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[col][row] = matrix.m[col][row] * factor;
    return result;
}

template<int N, int M, typename T>
QGenericMatrix<N, M, T> operator/(const QGenericMatrix<N, M, T> &matrix, T divisor)
{
    QGenericMatrix<N, M, T> result(Qt::Uninitialized);
    for (int row = 0; row < M; ++row)
        for (int col = 0; col < N; ++col) result.m[col][row] = matrix.m[col][row] / divisor;
    return result;
}

template<int N, int M, typename T>
void QGenericMatrix<N, M, T>::copyDataTo(T *values) const
{
    for (int col = 0; col < N; ++col)
        for (int row = 0; row < M; ++row) values[row * N + col] = T(m[col][row]);
}

// Define aliases for the useful variants of QGenericMatrix.
typedef QGenericMatrix<2, 2, float> QMatrix2x2;
typedef QGenericMatrix<2, 3, float> QMatrix2x3;
typedef QGenericMatrix<2, 4, float> QMatrix2x4;
typedef QGenericMatrix<3, 2, float> QMatrix3x2;
typedef QGenericMatrix<3, 3, float> QMatrix3x3;
typedef QGenericMatrix<3, 4, float> QMatrix3x4;
typedef QGenericMatrix<4, 2, float> QMatrix4x2;
typedef QGenericMatrix<4, 3, float> QMatrix4x3;

#endif
