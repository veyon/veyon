// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>
#include <QObject>

#include "NetworkObject.h"

#include <functional>
#include <optional>

class QTimer;

class VEYON_CORE_EXPORT NetworkObjectDirectory : public QObject
{
	Q_OBJECT
public:
	enum {
		MinimumUpdateInterval = 10,
		DefaultUpdateInterval = 60,
		MaximumUpdateInterval = 3600
	};

	explicit NetworkObjectDirectory(QObject* parent);

	void setUpdateInterval(int interval);

	const NetworkObjectList& objects(const NetworkObject& parent) const;

	const NetworkObject& object(NetworkObject::ModelId parent, NetworkObject::ModelId object) const;
	int index(NetworkObject::ModelId parent, NetworkObject::ModelId child) const;
	int childCount(NetworkObject::ModelId parent) const;
	NetworkObject::ModelId childId(NetworkObject::ModelId parent, int index) const;
	NetworkObject::ModelId parentId(NetworkObject::ModelId child) const;

	NetworkObject::ModelId rootId() const;

	const NetworkObject& rootObject() const
	{
		return m_rootObject;
	}

	virtual QVariant queryObjectAttribute(NetworkObject::Uid objectUid, NetworkObject::Attribute attribute);
	virtual NetworkObjectList queryObjects(NetworkObject::Type type,
										   NetworkObject::Attribute attribute, const QVariant& value);
	virtual NetworkObjectList queryParents(const NetworkObject& child);

	// like queryObjects()/queryParents(), but return std::nullopt if the directory could not be
	// queried (e.g. directory or broker not available) so that access control can fail closed
	virtual std::optional<NetworkObjectList> tryQueryObjects(NetworkObject::Type type,
															 NetworkObject::Attribute attribute, const QVariant& value);
	virtual std::optional<NetworkObjectList> tryQueryParents(const NetworkObject& child);

	virtual void update() = 0;
	virtual void fetchObjects(const NetworkObject& object);

	// whether the last update() or fetchObjects() could not query the directory completely, i.e.
	// the objects are incomplete and must not be presented as the current state
	bool hasUpdateFailed() const
	{
		return m_updateFailed;
	}

protected:
	void setUpdateFailed(bool failed)
	{
		m_updateFailed = failed;
	}

	using NetworkObjectFilter = std::function<bool(const NetworkObject&)>;

	bool hasObjects() const;
	void addOrUpdateObject(const NetworkObject& networkObject, const NetworkObject& parent);
	void removeObjects(const NetworkObject& parent, const NetworkObjectFilter& removeObjectFilter);
	void replaceObjects(const NetworkObjectList& objects, const NetworkObject& parent);
	void setObjectPopulated(const NetworkObject& networkObject);
	void propagateChildObjectChange(NetworkObject::ModelId objectId, int depth = 0);
	void propagateChildObjectChanges();

private:
	static constexpr auto ObjectChangePropagationTimeout = 100;

	QTimer* m_updateTimer;
	QTimer* m_propagateChangedObjectsTimer;
	QHash<NetworkObject::ModelId, NetworkObjectList> m_objects;
	NetworkObject m_invalidObject;
	NetworkObject m_rootObject;
	NetworkObjectList m_defaultObjectList;
	QList<NetworkObject::ModelId> m_changedObjectIds;
	bool m_updateFailed = false;

Q_SIGNALS:
	void objectsAboutToBeInserted(NetworkObject::ModelId parentId, int index, int count);
	void objectsInserted();
	void objectsAboutToBeRemoved(NetworkObject::ModelId parentId, int index, int count);
	void objectsRemoved();
	void objectChanged(NetworkObject::ModelId parentId, int index);
};
