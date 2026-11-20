// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QUuid>

#include "Configuration/Password.h"
#include "Configuration/SecretReference.h"
#include "VeyonCore.h"

namespace Configuration {

class Object;
class Proxy;

// clang-format off
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
// clang-format on


class VEYON_CORE_EXPORT Property : public QObject
{
	Q_OBJECT
public:
	enum class Flag {
		Standard = 0x01,
		Advanced = 0x02,
		Hidden = 0x04,
		Legacy = 0x08
	};
	Q_DECLARE_FLAGS(Flags, Flag)
	Q_FLAG(Flags)

	Property(Object* object, const QString& key, const QString& parentKey,
			 const QVariant& defaultValue, Flags flags);

	Property(Proxy* proxy, const QString& key, const QString& parentKey,
			 const QVariant& defaultValue, Flags flags);

	QObject* lambdaContext() const;

	QVariant variantValue() const;

	void setVariantValue(const QVariant& value) const;

	static Property* find(QObject* parent, const QString& key, const QString& parentKey);

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
		if (m_parentKey.isEmpty())
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

	// true for TypedProperty<SecretReference>, i.e. the value is a reference to a SecretStore key
	bool isSecretReference() const
	{
		return m_isSecretReference;
	}

protected:
	// not checked against the SecretPropertyRegistry here: plugins create their configuration
	// objects before the registry is complete; the value is resolved via the registry instead
	void markAsSecretReference()
	{
		m_isSecretReference = true;
	}

private:
	Object* m_object;
	Proxy* m_proxy;
	const QString m_key;
	const QString m_parentKey;
	const QVariant m_defaultValue;
	const Flags m_flags;
	bool m_isSecretReference = false;
};


template<class T>
class TypedProperty : public Property
{
public:
	using Type = typename CheapestType<T>::Type;

	TypedProperty(Object* object, const QString& key, const QString& parentKey,
				  const QVariant& defaultValue, Flags flags) :
		Property(object, key, parentKey, defaultValue, flags)
	{
		if constexpr (std::is_same_v<T, SecretReference>)
		{
			markAsSecretReference();
		}
	}

	TypedProperty(Proxy* proxy, const QString& key, const QString& parentKey,
				  const QVariant& defaultValue, Flags flags) :
		Property(proxy, key, parentKey, defaultValue, flags)
	{
		if constexpr (std::is_same_v<T, SecretReference>)
		{
			markAsSecretReference();
		}
	}

	T value() const
	{
		return variantValue().template value<T>();
	}

	void setValue(Type value) const
	{
		setVariantValue(QVariant::fromValue(value));
	}
};

// read-only legacy type, see LegacySecretMigration
template<>
VEYON_CORE_EXPORT Password TypedProperty<Password>::value() const;

// always the property's own reference (its key in the SecretPropertyRegistry, which must match its
// default value), or an invalid reference if the property is not registered or the configuration
// holds a legacy value or a reference to a different key
template<>
VEYON_CORE_EXPORT SecretReference TypedProperty<SecretReference>::value() const;

template<>
VEYON_CORE_EXPORT void TypedProperty<SecretReference>::setValue(const SecretReference& value) const;


#define DECLARE_CONFIG_PROPERTY(className, config, type, name, setter, key, parentKey, defaultValue, flags) \
private: \
	const Configuration::TypedProperty<type>* m_##name{new Configuration::TypedProperty<type>(this, QStringLiteral(key), QStringLiteral(parentKey), defaultValue, flags)}; \
public: \
	type name() const \
	{ \
		return m_##name->value(); \
	} \
	const Configuration::TypedProperty<type>& name##Property() const \
	{ \
		return *m_##name; \
	} \
	void setter(Configuration::TypedProperty<type>::Type value) const \
	{ \
		m_##name->setValue(value); \
	}

}
