// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QUuid>
#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <limits>
#include <cmath>

#include "Configuration/Password.h"
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

	enum class Editor { Automatic, Boolean, Integer, Text, Password, Choice, Color, File, Directory, Custom };
	Q_ENUM(Editor)
	enum class ApplyEffect { None = 0, ServiceRestart = 1, ApplicationRestart = 2, Platform = 4 };
	Q_DECLARE_FLAGS(ApplyEffects, ApplyEffect)
	Q_FLAG(ApplyEffects)

	struct Choice
	{
		QVariant value;
		QString label;
	};

	struct UiMetadata
	{
		QString pageId;
		QString groupId;
		QString label;
		QString toolTip;
		QString suffix;
		Editor editor{Editor::Automatic};
		QVariant minimum;
		QVariant maximum;
		QVariant step;
		QList<Choice> choices;
		QStringList dependencyKeys;
		std::function<bool()> enabled;
		std::function<bool()> visible;
		ApplyEffects effects;
		int order{0};
	};

	Property& setUiMetadata(UiMetadata metadata);
	const UiMetadata& uiMetadata() const { return m_uiMetadata; }
	virtual QMetaType valueType() const { return m_defaultValue.metaType(); }
	virtual QVariant editorValue() const { return variantValue(); }
	virtual bool setEditorValue(const QVariant& value, QString* error = nullptr) const;
	Editor editor() const;
	bool isEnabled() const;
	bool isVisible() const;
	bool validateEditorValue(const QVariant& value, QString* error = nullptr) const;

Q_SIGNALS:
	void metadataChanged();

public:

	Property( Object* object, const QString& key, const QString& parentKey,
			  const QVariant& defaultValue, Flags flags );

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

	QString absoluteKey() const;

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
	UiMetadata m_uiMetadata;

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
	}

	TypedProperty(Proxy* proxy, const QString& key, const QString& parentKey,
				  const QVariant& defaultValue, Flags flags) :
		Property(proxy, key, parentKey, defaultValue, flags)
	{
	}

	T value() const
	{
		return variantValue().template value<T>();
	}

	void setValue(Type value) const
	{
		setVariantValue(QVariant::fromValue(value));
	}

	QMetaType valueType() const override { return QMetaType::fromType<T>(); }

	QVariant editorValue() const override
	{
		if constexpr (std::is_enum_v<T>)
		{
			return static_cast<int>(value());
		}
		else if constexpr (std::is_same_v<T, QUuid>)
		{
			return value().toString();
		}
		else
		{
			return QVariant::fromValue(value());
		}
	}

	bool setEditorValue(const QVariant& input, QString* error = nullptr) const override
	{
		if (!validateEditorValue(input, error)) return false;
		const auto fail = [error]() {
			if (error) *error = Property::tr("The value has an invalid type or is outside the supported range.");
			return false;
		};
		if constexpr (std::is_enum_v<T>)
		{
			if (input.metaType().id() == QMetaType::Bool) return fail();
			bool ok = false;
			const auto numeric = input.toLongLong(&ok);
			if (!ok || input.toDouble() != static_cast<double>(numeric) ||
				numeric < std::numeric_limits<int>::min() || numeric > std::numeric_limits<int>::max()) return fail();
			setValue(static_cast<T>(numeric));
		}
		else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
		{
			if (input.metaType().id() == QMetaType::Bool) return fail();
			bool ok = false;
			const auto numeric = input.toLongLong(&ok);
			if (!ok || input.toDouble() != static_cast<double>(numeric) ||
				static_cast<long double>(numeric) < std::numeric_limits<T>::lowest() ||
				static_cast<long double>(numeric) > std::numeric_limits<T>::max()) return fail();
			setValue(static_cast<T>(numeric));
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			if (input.metaType().id() != QMetaType::Bool) return fail();
			setValue(input.toBool());
		}
		else if constexpr (std::is_same_v<T, QString>)
		{
			if (input.metaType().id() != QMetaType::QString) return fail();
			setValue(input.toString());
		}
		else if constexpr (std::is_same_v<T, QUuid>)
		{
			const auto uuid = input.toUuid();
			if (uuid.isNull() && !input.toString().isEmpty() &&
				input.toString() != QUuid().toString() && input.toString() != QUuid().toString(QUuid::WithoutBraces)) return fail();
			setValue(uuid);
		}
		else if constexpr (std::is_same_v<T, QColor>)
		{
			const auto color = input.value<QColor>();
			if (!color.isValid()) return fail();
			setValue(color);
		}
		else
		{
			auto converted = input;
			if (!converted.convert(QMetaType::fromType<T>())) return fail();
			setValue(converted.value<T>());
		}
		return true;
	}
};

template<>
VEYON_CORE_EXPORT Password TypedProperty<Password>::value() const;

template<>
VEYON_CORE_EXPORT void TypedProperty<Password>::setValue(const Password& value) const;

template<>
VEYON_CORE_EXPORT QVariant TypedProperty<Password>::editorValue() const;

template<>
VEYON_CORE_EXPORT bool TypedProperty<Password>::setEditorValue(const QVariant& value, QString* error) const;


#define DECLARE_CONFIG_PROPERTY(className,config,type, name, setter, key, parentKey, defaultValue, flags) \
	private: \
		Configuration::TypedProperty<type>* m_##name{ new Configuration::TypedProperty<type>( this, QStringLiteral(key), QStringLiteral(parentKey), defaultValue, flags ) }; \
	public: \
	type name() const \
	{ \
		return m_##name->value(); \
	} \
		Configuration::TypedProperty<type>& name##Property() \
		{ \
			return *m_##name; \
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

Q_DECLARE_OPERATORS_FOR_FLAGS(Configuration::Property::ApplyEffects)
