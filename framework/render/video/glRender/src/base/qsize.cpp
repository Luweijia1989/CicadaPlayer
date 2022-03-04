#include "qsize.h"

void QSize::transpose()
{
    qSwap(wd, ht);
}

QSize QSize::scaled(const QSize &s, YPP::AspectRatioMode mode) const
{
    if (mode == YPP::IgnoreAspectRatio || wd == 0 || ht == 0) {
        return s;
    } else {
        bool useHeight;
        __int64 rw = __int64(s.ht) * __int64(wd) / __int64(ht);

        if (mode == YPP::KeepAspectRatio) {
            useHeight = (rw <= s.wd);
        } else {// mode == YPP::KeepAspectRatioByExpanding
            useHeight = (rw >= s.wd);
        }

        if (useHeight) {
            return QSize(rw, s.ht);
        } else {
            return QSize(s.wd, __int32(__int64(s.wd) * __int64(ht) / __int64(wd)));
        }
    }
}


void QSizeF::transpose()
{
    qSwap(wd, ht);
}

QSizeF QSizeF::scaled(const QSizeF &s, YPP::AspectRatioMode mode) const
{
    if (mode == YPP::IgnoreAspectRatio || qIsNull(wd) || qIsNull(ht)) {
        return s;
    } else {
        bool useHeight;
        double rw = s.ht * wd / ht;

        if (mode == YPP::KeepAspectRatio) {
            useHeight = (rw <= s.wd);
        } else {// mode == YPP::KeepAspectRatioByExpanding
            useHeight = (rw >= s.wd);
        }

        if (useHeight) {
            return QSizeF(rw, s.ht);
        } else {
            return QSizeF(s.wd, s.wd * ht / wd);
        }
    }
}
