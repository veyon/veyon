// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QSet>
#include "Configuration/Object.h"
#include "Configuration/Property.h"

namespace Configuration
{

class VEYON_CORE_EXPORT PropertyModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(QString pageId READ pageId WRITE setPageId NOTIFY filterChanged)
	Q_PROPERTY(QString groupId READ groupId WRITE setGroupId NOTIFY filterChanged)
	Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged)
	Q_PROPERTY(bool advancedMode READ advancedMode WRITE setAdvancedMode NOTIFY filterChanged)
public:
	enum Role
	{
		KeyRole = Qt::UserRole + 1, LabelRole, ToolTipRole, EditorRole, ValueRole,
		DefaultValueRole, MinimumRole, MaximumRole, StepRole, SuffixRole, ChoicesRole,
		EnabledRole, VisibleRole, AdvancedRole, ErrorRole, PageIdRole, GroupIdRole, EffectsRole
	};
	Q_ENUM(Role)

	explicit PropertyModel(Object* object, QObject* parent = nullptr);
	int rowCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QString pageId() const { return m_pageId; }
	QString groupId() const { return m_groupId; }
	QString search() const { return m_search; }
	bool advancedMode() const { return m_advancedMode; }
	void setPageId(const QString& pageId);
	void setGroupId(const QString& groupId);
	void setSearch(const QString& search);
	void setAdvancedMode(bool advancedMode);

	Q_INVOKABLE bool setValue(const QString& key, const QVariant& value);
	Q_INVOKABLE QVariant value(const QString& key) const;
	Q_INVOKABLE QVariantMap propertyInfo(const QString& key) const;
	Q_INVOKABLE void refresh();

Q_SIGNALS:
	void filterChanged();
	void valueEdited(const QString& key);
	void editRejected(const QString& key, const QString& error);

private:
	Property* property(const QString& key) const;
	QVariant propertyData(const Property* property, int role) const;
	QPointer<Object> m_object;
	QList<Property*> m_properties;
	QList<Property*> m_rows;
	QSet<Property*> m_connected;
	QHash<QString, QString> m_errors;
	QString m_pageId;
	QString m_groupId;
	QString m_search;
	bool m_advancedMode{false};
};

}
