/*
 * QtCompat.h - functions and templates for compatibility with older Qt versions
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef QT_COMPAT_H
#define QT_COMPAT_H

#include <QtGlobal>
#include <QSet>

template<class A, class B>
static inline bool intersects( const QSet<A>& a, const QSet<B>& b )
{
#if QT_VERSION < 0x050600
	return QSet<A>( a ).intersect( b ).isEmpty() == false;
#else
	return a.intersects( b );
#endif
}

#if QT_VERSION >= 0x050600
#include <QVersionNumber>
#else

// taken from qtbase/src/corelib/tools/qversionnumber.h

class QVersionNumber
{
    /*
     * QVersionNumber stores small values inline, without memory allocation.
     * We do that by setting the LSB in the pointer that would otherwise hold
     * the longer form of the segments.
     * The constants below help us deal with the permutations for 32- and 64-bit,
     * little- and big-endian architectures.
     */
    enum {
        // in little-endian, inline_segments[0] is shared with the pointer's LSB, while
        // in big-endian, it's inline_segments[7]
        InlineSegmentMarker = Q_BYTE_ORDER == Q_LITTLE_ENDIAN ? 0 : sizeof(void*) - 1,
        InlineSegmentStartIdx = !InlineSegmentMarker, // 0 for BE, 1 for LE
        InlineSegmentCount = sizeof(void*) - 1
    };
    Q_STATIC_ASSERT(InlineSegmentCount >= 3);   // at least major, minor, micro

    struct SegmentStorage {
        // Note: we alias the use of dummy and inline_segments in the use of the
        // union below. This is undefined behavior in C++98, but most compilers implement
        // the C++11 behavior. The one known exception is older versions of Sun Studio.
        union {
            quintptr dummy;
            qint8 inline_segments[sizeof(void*)];
            QVector<int> *pointer_segments;
        };

        // set the InlineSegmentMarker and set length to zero
        SegmentStorage() Q_DECL_NOTHROW : dummy(1) {}

        SegmentStorage(const QVector<int> &seg)
        {
            if (dataFitsInline(seg.begin(), seg.size()))
                setInlineData(seg.begin(), seg.size());
            else
                pointer_segments = new QVector<int>(seg);
        }

        SegmentStorage(const SegmentStorage &other)
        {
            if (other.isUsingPointer())
                pointer_segments = new QVector<int>(*other.pointer_segments);
            else
                dummy = other.dummy;
        }

        SegmentStorage &operator=(const SegmentStorage &other)
        {
            if (isUsingPointer() && other.isUsingPointer()) {
                *pointer_segments = *other.pointer_segments;
            } else if (other.isUsingPointer()) {
                pointer_segments = new QVector<int>(*other.pointer_segments);
            } else {
                if (isUsingPointer())
                    delete pointer_segments;
                dummy = other.dummy;
            }
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        SegmentStorage(SegmentStorage &&other) Q_DECL_NOTHROW
            : dummy(other.dummy)
        {
            other.dummy = 1;
        }

        SegmentStorage &operator=(SegmentStorage &&other) Q_DECL_NOTHROW
        {
            qSwap(dummy, other.dummy);
            return *this;
        }

        explicit SegmentStorage(QVector<int> &&seg)
        {
            if (dataFitsInline(seg.begin(), seg.size()))
                setInlineData(seg.begin(), seg.size());
            else
                pointer_segments = new QVector<int>(std::move(seg));
        }
#endif
#ifdef Q_COMPILER_INITIALIZER_LISTS
        SegmentStorage(std::initializer_list<int> args)
        {
            if (dataFitsInline(args.begin(), int(args.size()))) {
                setInlineData(args.begin(), int(args.size()));
            } else {
                pointer_segments = new QVector<int>(args);
            }
        }
#endif

        ~SegmentStorage() { if (isUsingPointer()) delete pointer_segments; }

        bool isUsingPointer() const Q_DECL_NOTHROW
        { return (inline_segments[InlineSegmentMarker] & 1) == 0; }

        int size() const Q_DECL_NOTHROW
        { return isUsingPointer() ? pointer_segments->size() : (inline_segments[InlineSegmentMarker] >> 1); }

        void setInlineSize(int len)
        { inline_segments[InlineSegmentMarker] = 1 + 2 * len; }

        void resize(int len)
        {
            if (isUsingPointer())
                pointer_segments->resize(len);
            else
                setInlineSize(len);
        }

        int at(int index) const
        {
            return isUsingPointer() ?
                        pointer_segments->at(index) :
                        inline_segments[InlineSegmentStartIdx + index];
        }

        void setSegments(int len, int maj, int min = 0, int mic = 0)
        {
            if (maj == qint8(maj) && min == qint8(min) && mic == qint8(mic)) {
                int data[] = { maj, min, mic };
                setInlineData(data, len);
            } else {
                setVector(len, maj, min, mic);
            }
        }

    private:
        static bool dataFitsInline(const int *data, int len)
        {
            if (len > InlineSegmentCount)
                return false;
            for (int i = 0; i < len; ++i)
                if (data[i] != qint8(data[i]))
                    return false;
            return true;
        }
        void setInlineData(const int *data, int len)
        {
            dummy = 1 + len * 2;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            for (int i = 0; i < len; ++i)
                dummy |= quintptr(data[i] & 0xFF) << (8 * (i + 1));
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
            for (int i = 0; i < len; ++i)
                dummy |= quintptr(data[i] & 0xFF) << (8 * (sizeof(void *) - i - 1));
#else
            // the code above is equivalent to:
            setInlineSize(len);
            for (int i = 0; i < len; ++i)
                inline_segments[InlineSegmentStartIdx + i] = data[i] & 0xFF;
#endif
        }

        Q_CORE_EXPORT void setVector(int len, int maj, int min, int mic);
    } m_segments;

public:
    inline QVersionNumber() Q_DECL_NOTHROW
        : m_segments()
    {}
    inline explicit QVersionNumber(const QVector<int> &seg)
        : m_segments(seg)
    {}

    // compiler-generated copy/move ctor/assignment operators and the destructor are ok

#ifdef Q_COMPILER_RVALUE_REFS
    explicit QVersionNumber(QVector<int> &&seg)
        : m_segments(std::move(seg))
    {}
#endif

#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QVersionNumber(std::initializer_list<int> args)
        : m_segments(args)
    {}
#endif

    inline explicit QVersionNumber(int maj)
    { m_segments.setSegments(1, maj); }

    inline explicit QVersionNumber(int maj, int min)
    { m_segments.setSegments(2, maj, min); }

    inline explicit QVersionNumber(int maj, int min, int mic)
    { m_segments.setSegments(3, maj, min, mic); }

    Q_REQUIRED_RESULT inline bool isNull() const Q_DECL_NOTHROW
    { return segmentCount() == 0; }

    Q_REQUIRED_RESULT inline bool isNormalized() const Q_DECL_NOTHROW
    { return isNull() || segmentAt(segmentCount() - 1) != 0; }

    Q_REQUIRED_RESULT inline int majorVersion() const Q_DECL_NOTHROW
    { return segmentAt(0); }

    Q_REQUIRED_RESULT inline int minorVersion() const Q_DECL_NOTHROW
    { return segmentAt(1); }

    Q_REQUIRED_RESULT inline int microVersion() const Q_DECL_NOTHROW
    { return segmentAt(2); }

    Q_REQUIRED_RESULT Q_CORE_EXPORT QVersionNumber normalized() const;

    Q_REQUIRED_RESULT Q_CORE_EXPORT QVector<int> segments() const;

    Q_REQUIRED_RESULT inline int segmentAt(int index) const Q_DECL_NOTHROW
    { return (m_segments.size() > index) ? m_segments.at(index) : 0; }

    Q_REQUIRED_RESULT inline int segmentCount() const Q_DECL_NOTHROW
    { return m_segments.size(); }

    Q_REQUIRED_RESULT Q_CORE_EXPORT bool isPrefixOf(const QVersionNumber &other) const Q_DECL_NOTHROW;

    Q_REQUIRED_RESULT Q_CORE_EXPORT static int compare(const QVersionNumber &v1, const QVersionNumber &v2) Q_DECL_NOTHROW;

    Q_REQUIRED_RESULT Q_CORE_EXPORT static Q_DECL_PURE_FUNCTION QVersionNumber commonPrefix(const QVersionNumber &v1, const QVersionNumber &v2);

    Q_REQUIRED_RESULT Q_CORE_EXPORT QString toString() const;
    Q_REQUIRED_RESULT Q_CORE_EXPORT static Q_DECL_PURE_FUNCTION QVersionNumber fromString(const QString &string, int *suffixIndex = Q_NULLPTR);

};

Q_DECLARE_TYPEINFO(QVersionNumber, Q_MOVABLE_TYPE);

Q_REQUIRED_RESULT inline bool operator> (const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) > 0; }

Q_REQUIRED_RESULT inline bool operator>=(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) >= 0; }

Q_REQUIRED_RESULT inline bool operator< (const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) < 0; }

Q_REQUIRED_RESULT inline bool operator<=(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) <= 0; }

Q_REQUIRED_RESULT inline bool operator==(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) == 0; }

Q_REQUIRED_RESULT inline bool operator!=(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) != 0; }

Q_DECLARE_METATYPE(QVersionNumber)
#endif

#if QT_VERSION < 0x050700
template <typename T> struct QAddConst { typedef const T Type; };
template <typename T> constexpr typename QAddConst<T>::Type &qAsConst(T &t) { return t; }
template <typename T> void qAsConst(const T &&) = delete;

template <typename... Args>
struct QNonConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};

template <typename... Args>
struct QConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
	using QConstOverload<Args...>::of;
	using QConstOverload<Args...>::operator();
	using QNonConstOverload<Args...>::of;
	using QNonConstOverload<Args...>::operator();

	template <typename R>
	Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R>
	static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};
#endif

#endif
