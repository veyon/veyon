// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonArray>
#include <QVector>

#include "VeyonCore.h"

template<class T>
class ObjectManager
{
public:
	explicit ObjectManager(const QJsonArray& objects) :
		m_objects(objects)
	{
	}

	const QJsonArray& objects() const
	{
		return m_objects;
	}

	const QJsonArray& add(const T& object)
	{
		m_objects.append(object.toJson());

		return m_objects;
	}

	const QJsonArray& update(const T& object, bool addIfNotFound = false)
	{
		bool found = false;

		for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
		{
			T currentObject(it->toObject());
			if (currentObject.uid() == object.uid())
			{
				*it = object.toJson();
				found = true;
				break;
			}
		}

		if (!found)
		{
			add(object);
		}

		return m_objects;
	}

	const QJsonArray& remove(typename T::Uid objectUid, bool recursive = false)
	{
		if (recursive)
		{
			removeChildren(objectUid);
		}

		for (auto it = m_objects.begin(); it != m_objects.end();)
		{
			T currentObject(it->toObject());
			if (currentObject.uid() == objectUid)
			{
				it = m_objects.erase(it);
			}
			else
			{
				++it;
			}
		}

		return m_objects;
	}

	const QJsonArray& moveUp(typename T::Uid objectUid)
	{
		int currentIndex = -1;
		T targetObject;

		for (int i = 0; i < m_objects.size(); ++i)
		{
			T currentObject(m_objects[i].toObject());
			if (currentObject.uid() == objectUid)
			{
				currentIndex = i;
				targetObject = currentObject;
				break;
			}
		}

		if (currentIndex < 0)
		{
			return m_objects;
		}

		QVector<int> matchingIndices;
		matchingIndices.reserve(m_objects.size());

		for (int i = 0; i < m_objects.size(); ++i)
		{
			T currentObject(m_objects[i].toObject());
			if (currentObject.type() == targetObject.type() && currentObject.parentUid() == targetObject.parentUid())
			{
				matchingIndices.append(i);
			}
		}

		const int position = matchingIndices.indexOf(currentIndex);
		if (position > 0)
		{
			const int previousIndex = matchingIndices[position - 1];
			const auto swapValue = m_objects.at(currentIndex);
			m_objects[currentIndex] = m_objects[previousIndex];
			m_objects[previousIndex] = swapValue;
		}

		return m_objects;
	}

	const QJsonArray& moveDown(typename T::Uid objectUid)
	{
		int currentIndex = -1;
		T targetObject;

		for (int i = 0; i < m_objects.size(); ++i)
		{
			T currentObject(m_objects[i].toObject());
			if (currentObject.uid() == objectUid)
			{
				currentIndex = i;
				targetObject = currentObject;
				break;
			}
		}

		if (currentIndex < 0)
		{
			return m_objects;
		}

		QVector<int> matchingIndices;
		matchingIndices.reserve(m_objects.size());

		for (int i = 0; i < m_objects.size(); ++i)
		{
			T currentObject(m_objects[i].toObject());
			if (currentObject.type() == targetObject.type() && currentObject.parentUid() == targetObject.parentUid())
			{
				matchingIndices.append(i);
			}
		}

		const int position = matchingIndices.indexOf(currentIndex);
		if (position >= 0 && position < matchingIndices.size() - 1)
		{
			const int nextIndex = matchingIndices[position + 1];
			const auto swapValue = m_objects.at(currentIndex);
			m_objects[currentIndex] = m_objects[nextIndex];
			m_objects[nextIndex] = swapValue;
		}

		return m_objects;
	}

	void removeChildren(typename T::Uid objectUid)
	{
		for (auto it = m_objects.begin(); it != m_objects.end();)
		{
			T currentObject(it->toObject());
			if (currentObject.parentUid() == objectUid)
			{
				removeChildren(currentObject.uid());
				it = m_objects.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	T findByUid(typename T::Uid uid) const
	{
		for (auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it)
		{
			T currentObject(it->toObject());
			if (currentObject.uid() == uid)
			{
				return currentObject;
			}
		}

		return T();
	}

	T findByName(const typename T::Name& name) const
	{
		for (auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it)
		{
			T currentObject(it->toObject());
			if (T(it->toObject()).name() == name)
			{
				return currentObject;
			}
		}

		return T();
	}

	bool hasName(const typename T::Name& name) const
	{
		for (auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it)
		{
			if (T(it->toObject()).name() == name)
			{
				return true;
			}
		}

		return false;
	}

	typename T::Name generateUniqueName(const typename T::Name& baseName) const
	{
		if (hasName(baseName) == false)
		{
			return baseName;
		}

		for (int suffix = 2;; ++suffix)
		{
			const auto candidate = QStringLiteral("%1 %2").arg(baseName).arg(suffix);
			if (hasName(candidate) == false)
			{
				return candidate;
			}
		}
	}

private:
	QJsonArray m_objects;
};
