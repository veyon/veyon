/*
 * Configuration/Property.h - Configuration::Property class
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include <QUuid>

#include "QtCompat.h"
#include "VeyonCore.h"
#include "Configuration/Password.h"

namespace Configuration
{

class Object;
class Proxy;

template<typename T, typename = void> struct CheapestType          { using Type = const T &; };
template<typename T> struct CheapestType<T, typename std::enable_if<std::is_enum<T>::value >::type> { using Type = T; };
template<>           struct CheapestType<bool>    { using Type = bool; };
template<>           struct CheapestType<quint8>  { using Type = quint8; };
template<>           struct CheapestType<quint16> { using Type = quint16; };
template<>           struct CheapestType<quint32> { using Type = quint32; };
template<>           struct CheapestType<quint64> { using Type = quint64; };
template<>           struct CheapestType<qint8>   { using Type = qint8; };
template<>           struct CheapestType<qint16>  { using Type = qint16; };
template<>           struct CheapestType<qint32>  { using Type = qint32; };
template<>           struct CheapestType<qint64>  { using Type = qint64; };
template<>           struct CheapestType<float>   { using Type = float; };
template<>           struct CheapestType<double>  { using Type = double; };
template<>           struct CheapestType<QUuid>   { using Type = QUuid; };
template<typename T> struct CheapestType<T *>     { using Type = T *; };


class VEYON_CORE_EXPORT Property : public QObject
{
	Q_OBJECT
public:
	enum class Flag
	{
		Standard = 0x01,
		Advanced = 0x02,
		Hidden = 0x04,
		Legacy = 0x08
	};
	Q_DECLARE_FLAGS(Flags, Flag)

	// work around QTBUG-47652 where Q_FLAG() is broken for enum classes when using Qt < 5.12
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
	Q_FLAG(Flags)
#endif

	Property( Object* object, const QString& key, const QString& parentKey,
			  const QVariant& defaultValue, Flags flags );

	Property( Proxy* proxy, const QString& key, const QString& parentKey,
			  const QVariant& defaultValue, Flags flags );

	QObject* lambdaContext() const;

	QVariant variantValue() const;

	void setVariantValue( const QVariant& value ) const;

	static Property* find( QObject* parent, const QString& key, const QString& parentKey );

	const QString& key() const
	{
		return m_key;
	}

	const QString& parentKey() const
	{
		return m_parentKey;
	}

	QString absoluteKey() const
	{
		if( m_parentKey.isEmpty() )
		{
			return m_key;
		}

		return m_parentKey + QLatin1Char('/') + m_key;
	}

	const QVariant& defaultValue() const
	{
		return m_defaultValue;
	}

	Flags flags() const
	{
		return m_flags;
	}

private:
	Object* m_object;
	Proxy* m_proxy;
	const QString m_key;
	const QString m_parentKey;
	const QVariant m_defaultValue;
	const Flags m_flags;

};


template<class T>
class VEYON_CORE_EXPORT TypedProperty : public Property {
public:
	using Type = typename CheapestType<T>::Type;

	TypedProperty( Object* object, const QString& key, const QString& parentKey,
				   const QVariant& defaultValue, Flags flags ) :
		Property( object, key, parentKey, defaultValue, flags )
	{
	}

	TypedProperty( Proxy* proxy, const QString& key, const QString& parentKey,
				   const QVariant& defaultValue, Flags flags ) :
		Property( proxy, key, parentKey, defaultValue, flags )
	{
	}

	T value() const
	{
		return QVariantHelper<T>::value( variantValue() );
	}

	void setValue( Type value ) const
	{
		setVariantValue( QVariant::fromValue( value ) );
	}
};

template<>
VEYON_CORE_EXPORT Password TypedProperty<Password>::value() const;

template<>
VEYON_CORE_EXPORT void TypedProperty<Password>::setValue( const Password& value ) const;


#define DECLARE_CONFIG_PROPERTY(className,config,type, name, setter, key, parentKey, defaultValue, flags) \
	private: \
		const Configuration::TypedProperty<type>* m_##name{ new Configuration::TypedProperty<type>( this, QStringLiteral(key), QStringLiteral(parentKey), defaultValue, flags ) }; \
	public: \
		type name() const \
		{											\
			return m_##name->value();			\
		} \
		const Configuration::TypedProperty<type>& name##Property() const \
		{ \
			return *m_##name; \
		} \
		void setter( Configuration::TypedProperty<type>::Type value ) const \
		{ \
			m_##name->setValue( value ); \
		}

}
