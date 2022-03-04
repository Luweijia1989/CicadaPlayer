#pragma once

#include "common.h"
#include <algorithm>

class QSize {
public:
    QSize();
    QSize(int w, int h);

    inline bool isNull() const;
    inline bool isEmpty() const;
    inline bool isValid() const;

    inline int width() const;
    inline int height() const;
    inline void setWidth(int w);
    inline void setHeight(int h);
    void transpose();
    inline QSize transposed() const;

    inline void scale(int w, int h, YPP::AspectRatioMode mode);
    inline void scale(const QSize &s, YPP::AspectRatioMode mode);
    QSize scaled(int w, int h, YPP::AspectRatioMode mode) const;
    QSize scaled(const QSize &s, YPP::AspectRatioMode mode) const;

    inline QSize expandedTo(const QSize &) const;
    inline QSize boundedTo(const QSize &) const;

    inline int &rwidth();
    inline int &rheight();

    inline QSize &operator+=(const QSize &);
    inline QSize &operator-=(const QSize &);
    inline QSize &operator*=(double c);
    inline QSize &operator/=(double c);

    friend inline bool operator==(const QSize &, const QSize &);
    friend inline bool operator!=(const QSize &, const QSize &);
    friend inline const QSize operator+(const QSize &, const QSize &);
    friend inline const QSize operator-(const QSize &, const QSize &);
    friend inline const QSize operator*(const QSize &, double);
    friend inline const QSize operator*(double, const QSize &);
    friend inline const QSize operator/(const QSize &, double);

private:
    int wd;
    int ht;
};

inline QSize::QSize() : wd(-1), ht(-1)
{}

inline QSize::QSize(int w, int h) : wd(w), ht(h)
{}

inline bool QSize::isNull() const
{
    return wd == 0 && ht == 0;
}

inline bool QSize::isEmpty() const
{
    return wd < 1 || ht < 1;
}

inline bool QSize::isValid() const
{
    return wd >= 0 && ht >= 0;
}

inline int QSize::width() const
{
    return wd;
}

inline int QSize::height() const
{
    return ht;
}

inline void QSize::setWidth(int w)
{
    wd = w;
}

inline void QSize::setHeight(int h)
{
    ht = h;
}

inline QSize QSize::transposed() const
{
    return QSize(ht, wd);
}

inline void QSize::scale(int w, int h, YPP::AspectRatioMode mode)
{
    scale(QSize(w, h), mode);
}

inline void QSize::scale(const QSize &s, YPP::AspectRatioMode mode)
{
    *this = scaled(s, mode);
}

inline QSize QSize::scaled(int w, int h, YPP::AspectRatioMode mode) const
{
    return scaled(QSize(w, h), mode);
}

inline int &QSize::rwidth()
{
    return wd;
}

inline int &QSize::rheight()
{
    return ht;
}

inline QSize &QSize::operator+=(const QSize &s)
{
    wd += s.wd;
    ht += s.ht;
    return *this;
}

inline QSize &QSize::operator-=(const QSize &s)
{
    wd -= s.wd;
    ht -= s.ht;
    return *this;
}

inline QSize &QSize::operator*=(double c)
{
    wd = std::round(wd * c);
    ht = std::round(ht * c);
    return *this;
}

inline bool operator==(const QSize &s1, const QSize &s2)
{
    return s1.wd == s2.wd && s1.ht == s2.ht;
}

inline bool operator!=(const QSize &s1, const QSize &s2)
{
    return s1.wd != s2.wd || s1.ht != s2.ht;
}

inline const QSize operator+(const QSize &s1, const QSize &s2)
{
    return QSize(s1.wd + s2.wd, s1.ht + s2.ht);
}

inline const QSize operator-(const QSize &s1, const QSize &s2)
{
    return QSize(s1.wd - s2.wd, s1.ht - s2.ht);
}

inline const QSize operator*(const QSize &s, double c)
{
    return QSize(std::round(s.wd * c), std::round(s.ht * c));
}

inline const QSize operator*(double c, const QSize &s)
{
    return QSize(std::round(s.wd * c), std::round(s.ht * c));
}

inline QSize &QSize::operator/=(double c)
{
    assert(!qFuzzyIsNull(c));
    wd = std::round(wd / c);
    ht = std::round(ht / c);
    return *this;
}

inline const QSize operator/(const QSize &s, double c)
{
    assert(!qFuzzyIsNull(c));
    return QSize(std::round(s.wd / c), std::round(s.ht / c));
}

inline QSize QSize::expandedTo(const QSize &otherSize) const
{
    return QSize(std::max<int>(wd, otherSize.wd), std::max<int>(ht, otherSize.ht));
}

inline QSize QSize::boundedTo(const QSize &otherSize) const
{
    return QSize(std::min<int>(wd, otherSize.wd), std::min<int>(ht, otherSize.ht));
}

class QSizeF {
public:
    QSizeF();
    QSizeF(const QSize &sz);
    QSizeF(double w, double h);

    inline bool isNull() const;
    inline bool isEmpty() const;
    inline bool isValid() const;

    inline double width() const;
    inline double height() const;
    inline void setWidth(double w);
    inline void setHeight(double h);
    void transpose();
    inline QSizeF transposed() const;

    inline void scale(double w, double h, YPP::AspectRatioMode mode);
    inline void scale(const QSizeF &s, YPP::AspectRatioMode mode);
    QSizeF scaled(double w, double h, YPP::AspectRatioMode mode) const;
    QSizeF scaled(const QSizeF &s, YPP::AspectRatioMode mode) const;

    inline QSizeF expandedTo(const QSizeF &) const;
    inline QSizeF boundedTo(const QSizeF &) const;

    inline double &rwidth();
    inline double &rheight();

    inline QSizeF &operator+=(const QSizeF &);
    inline QSizeF &operator-=(const QSizeF &);
    inline QSizeF &operator*=(double c);
    inline QSizeF &operator/=(double c);

    friend inline bool operator==(const QSizeF &, const QSizeF &);
    friend inline bool operator!=(const QSizeF &, const QSizeF &);
    friend inline const QSizeF operator+(const QSizeF &, const QSizeF &);
    friend inline const QSizeF operator-(const QSizeF &, const QSizeF &);
    friend inline const QSizeF operator*(const QSizeF &, double);
    friend inline const QSizeF operator*(double, const QSizeF &);
    friend inline const QSizeF operator/(const QSizeF &, double);

    inline QSize toSize() const;

private:
    double wd;
    double ht;
};

inline QSizeF::QSizeF() : wd(-1.), ht(-1.)
{}

inline QSizeF::QSizeF(const QSize &sz) : wd(sz.width()), ht(sz.height())
{}

inline QSizeF::QSizeF(double w, double h) : wd(w), ht(h)
{}

inline bool QSizeF::isNull() const
{
    return qIsNull(wd) && qIsNull(ht);
}

inline bool QSizeF::isEmpty() const
{
    return wd <= 0. || ht <= 0.;
}

inline bool QSizeF::isValid() const
{
    return wd >= 0. && ht >= 0.;
}

inline double QSizeF::width() const
{
    return wd;
}

inline double QSizeF::height() const
{
    return ht;
}

inline void QSizeF::setWidth(double w)
{
    wd = w;
}

inline void QSizeF::setHeight(double h)
{
    ht = h;
}

inline QSizeF QSizeF::transposed() const
{
    return QSizeF(ht, wd);
}

inline void QSizeF::scale(double w, double h, YPP::AspectRatioMode mode)
{
    scale(QSizeF(w, h), mode);
}

inline void QSizeF::scale(const QSizeF &s, YPP::AspectRatioMode mode)
{
    *this = scaled(s, mode);
}

inline QSizeF QSizeF::scaled(double w, double h, YPP::AspectRatioMode mode) const
{
    return scaled(QSizeF(w, h), mode);
}

inline double &QSizeF::rwidth()
{
    return wd;
}

inline double &QSizeF::rheight()
{
    return ht;
}

inline QSizeF &QSizeF::operator+=(const QSizeF &s)
{
    wd += s.wd;
    ht += s.ht;
    return *this;
}

inline QSizeF &QSizeF::operator-=(const QSizeF &s)
{
    wd -= s.wd;
    ht -= s.ht;
    return *this;
}

inline QSizeF &QSizeF::operator*=(double c)
{
    wd *= c;
    ht *= c;
    return *this;
}

inline bool operator==(const QSizeF &s1, const QSizeF &s2)
{
    return qFuzzyCompare(s1.wd, s2.wd) && qFuzzyCompare(s1.ht, s2.ht);
}

inline bool operator!=(const QSizeF &s1, const QSizeF &s2)
{
    return !qFuzzyCompare(s1.wd, s2.wd) || !qFuzzyCompare(s1.ht, s2.ht);
}

inline const QSizeF operator+(const QSizeF &s1, const QSizeF &s2)
{
    return QSizeF(s1.wd + s2.wd, s1.ht + s2.ht);
}

inline const QSizeF operator-(const QSizeF &s1, const QSizeF &s2)
{
    return QSizeF(s1.wd - s2.wd, s1.ht - s2.ht);
}

inline const QSizeF operator*(const QSizeF &s, double c)
{
    return QSizeF(s.wd * c, s.ht * c);
}

inline const QSizeF operator*(double c, const QSizeF &s)
{
    return QSizeF(s.wd * c, s.ht * c);
}

inline QSizeF &QSizeF::operator/=(double c)
{
    assert(!qFuzzyIsNull(c));
    wd = wd / c;
    ht = ht / c;
    return *this;
}

inline const QSizeF operator/(const QSizeF &s, double c)
{
    assert(!qFuzzyIsNull(c));
    return QSizeF(s.wd / c, s.ht / c);
}

inline QSizeF QSizeF::expandedTo(const QSizeF &otherSize) const
{
    return QSizeF(std::max<double>(wd, otherSize.wd), std::max<double>(ht, otherSize.ht));
}

inline QSizeF QSizeF::boundedTo(const QSizeF &otherSize) const
{
    return QSizeF(std::min<double>(wd, otherSize.wd), std::min<double>(ht, otherSize.ht));
}

inline QSize QSizeF::toSize() const
{
    return QSize(std::round(wd), std::round(ht));
}
