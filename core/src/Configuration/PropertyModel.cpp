// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration/PropertyModel.h"
#include <QMetaEnum>
#include <algorithm>

namespace Configuration
{

PropertyModel::PropertyModel(Object* object, QObject* parent) :
	QAbstractListModel(parent), m_object(object)
{
	if (object)
	{
		connect(object, &Object::configurationChanged, this, [this]() {
			m_errors.clear();
			refresh();
		});
		connect(object, &QObject::destroyed, this, [this]() {
			beginResetModel();
			m_rows.clear();
			m_properties.clear();
			endResetModel();
		});
	}
	refresh();
}

int PropertyModel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : m_rows.size();
}

QHash<int, QByteArray> PropertyModel::roleNames() const
{
	return {{KeyRole, "key"}, {LabelRole, "label"}, {ToolTipRole, "toolTip"},
		{EditorRole, "editor"}, {ValueRole, "value"}, {DefaultValueRole, "defaultValue"},
		{MinimumRole, "minimum"}, {MaximumRole, "maximum"}, {StepRole, "step"},
		{SuffixRole, "suffix"}, {ChoicesRole, "choices"}, {EnabledRole, "enabled"},
		{VisibleRole, "visible"}, {AdvancedRole, "advanced"}, {ErrorRole, "error"},
		{PageIdRole, "pageId"}, {GroupIdRole, "groupId"}, {EffectsRole, "effects"}};
}

static QVariant exposedValue(const QVariant& value)
{
	if (value.metaType().flags().testFlag(QMetaType::IsEnumeration)) return value.toInt();
	if (value.metaType() == QMetaType::fromType<QUuid>()) return value.toUuid().toString();
	return value;
}

QVariant PropertyModel::propertyData(const Property* property, int role) const
{
	const auto& metadata = property->uiMetadata();
	switch (role)
	{
	case KeyRole: return property->absoluteKey();
	case Qt::DisplayRole:
	case LabelRole: return metadata.label;
	case ToolTipRole: return metadata.toolTip;
	case EditorRole: return QString::fromLatin1(QMetaEnum::fromType<Property::Editor>().valueToKey(static_cast<int>(property->editor()))).toLower();
	case ValueRole: return property->editorValue();
	case DefaultValueRole: return exposedValue(property->defaultValue());
	case MinimumRole: return metadata.minimum;
	case MaximumRole: return metadata.maximum;
	case StepRole: return metadata.step.isValid() ? metadata.step : QVariant(1);
	case SuffixRole: return metadata.suffix;
	case ChoicesRole:
	{
		QVariantList choices;
		for (const auto& choice : metadata.choices)
			choices.append(QVariantMap{{QStringLiteral("value"), exposedValue(choice.value)}, {QStringLiteral("label"), choice.label}});
		return choices;
	}
	case EnabledRole: return property->isEnabled();
	case VisibleRole: return property->isVisible();
	case AdvancedRole: return property->flags().testFlag(Property::Flag::Advanced);
	case ErrorRole: return m_errors.value(property->absoluteKey());
	case PageIdRole: return metadata.pageId;
	case GroupIdRole: return metadata.groupId;
	case EffectsRole: return static_cast<int>(metadata.effects);
	default: return {};
	}
}

QVariant PropertyModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) return {};
	return propertyData(m_rows.at(index.row()), role);
}

bool PropertyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size() ||
		(role != Qt::EditRole && role != ValueRole)) return false;
	return setValue(m_rows.at(index.row())->absoluteKey(), value);
}

Qt::ItemFlags PropertyModel::flags(const QModelIndex& index) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) return Qt::NoItemFlags;
	return m_rows.at(index.row())->isEnabled() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable : Qt::ItemIsSelectable;
}

Property* PropertyModel::property(const QString& key) const
{
	for (auto property : m_properties)
		if (property->absoluteKey() == key) return property;
	return nullptr;
}

QVariant PropertyModel::value(const QString& key) const
{
	const auto item = property(key);
	return item ? item->editorValue() : QVariant();
}

QVariantMap PropertyModel::propertyInfo(const QString& key) const
{
	QVariantMap result;
	if (const auto item = property(key))
	{
		const auto roles = roleNames();
		for (auto it = roles.constBegin(); it != roles.constEnd(); ++it)
			result.insert(QString::fromLatin1(it.value()), propertyData(item, it.key()));
	}
	return result;
}

bool PropertyModel::setValue(const QString& key, const QVariant& value)
{
	const auto item = property(key);
	QString error;
	if (!item || !item->isVisible() || !item->isEnabled())
		error = tr("This setting is not available for editing.");
	else
	{
		m_errors.remove(key);
		if (item->setEditorValue(value, &error))
		{
			Q_EMIT valueEdited(key);
			refresh();
			return true;
		}
	}
	m_errors.insert(key, error);
	refresh();
	Q_EMIT editRejected(key, error);
	return false;
}

void PropertyModel::refresh()
{
	if (!m_object) return;
	m_properties.clear();
	QSet<QString> keys;
	for (auto item : m_object->findChildren<Property*>())
	{
		if (!m_connected.contains(item))
		{
			m_connected.insert(item);
			connect(item, &Property::metadataChanged, this, &PropertyModel::refresh);
			connect(item, &QObject::destroyed, this, [this, item]() {
				m_connected.remove(item);
				m_properties.removeAll(item);
				if (m_rows.contains(item))
				{
					beginResetModel();
					m_rows.removeAll(item);
					endResetModel();
				}
			});
		}
		if (keys.contains(item->absoluteKey())) continue;
		keys.insert(item->absoluteKey());
		m_properties.append(item);
	}
	QList<Property*> rows;
	for (auto item : m_properties)
	{
		const auto& metadata = item->uiMetadata();
		if (!item->isVisible() || metadata.label.isEmpty() ||
			(!m_advancedMode && item->flags().testFlag(Property::Flag::Advanced)) ||
			(!m_pageId.isEmpty() && metadata.pageId != m_pageId) ||
			(!m_groupId.isEmpty() && metadata.groupId != m_groupId)) continue;
		const QString searchableText = metadata.label + QLatin1Char(' ') + metadata.toolTip +
			QLatin1Char(' ') + item->absoluteKey();
		if (!m_search.isEmpty() && !searchableText.contains(m_search, Qt::CaseInsensitive)) continue;
		rows.append(item);
	}
	std::stable_sort(rows.begin(), rows.end(), [](const Property* a, const Property* b) {
		return a->uiMetadata().order < b->uiMetadata().order;
	});
	if (rows != m_rows)
	{
		beginResetModel();
		m_rows = rows;
		endResetModel();
	}
	else if (!m_rows.isEmpty())
		Q_EMIT dataChanged(index(0), index(m_rows.size() - 1));
}

void PropertyModel::setPageId(const QString& value)
{
	if (m_pageId == value) return;
	m_pageId = value;
	refresh();
	Q_EMIT filterChanged();
}

void PropertyModel::setGroupId(const QString& value)
{
	if (m_groupId == value) return;
	m_groupId = value;
	refresh();
	Q_EMIT filterChanged();
}

void PropertyModel::setSearch(const QString& value)
{
	if (m_search == value) return;
	m_search = value;
	refresh();
	Q_EMIT filterChanged();
}

void PropertyModel::setAdvancedMode(bool value)
{
	if (m_advancedMode == value) return;
	m_advancedMode = value;
	refresh();
	Q_EMIT filterChanged();
}

}
