// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractListModel>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT ComputerListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum {
		UidRole = Qt::UserRole,
		StateRole,
		UserLoginNameRole,
		ImageIdRole,
		GroupsRole,
		FramebufferRole,
		ControlInterfaceRole
	};

	enum class DisplayRoleContent {
		UserAndComputerName,
		UserName,
		ComputerName,
		ComputerAndUserName,
	};
	Q_ENUM(DisplayRoleContent)

	enum class UidRoleContent {
		NetworkObjectUid,
		SessionMetaDataHash,
	};
	Q_ENUM(UidRoleContent)

	enum class GridSizingMode {
		FullLengthLabels,
		MaximizedThumbnails,
	};
	Q_ENUM(GridSizingMode)

	enum class VisibilityMode {
		Normal,
		Blurred,
		Hidden,
	};
	Q_ENUM(VisibilityMode)

	enum class SortOrder {
		ComputerAndUserName,
		UserName,
		ComputerName,
		LastPartOfUserName,
	};
	Q_ENUM(SortOrder)

	enum class AspectRatio {
		Auto,
		AR16_9,
		AR16_10,
		AR3_2,
		AR4_3
	};
	Q_ENUM(AspectRatio)

	explicit ComputerListModel(QObject* parent = nullptr);

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;

	DisplayRoleContent displayRoleContent() const
	{
		return m_displayRoleContent;
	}

	UidRoleContent uidRoleContent() const
	{
		return m_uidRoleContent;
	}

	GridSizingMode gridSizingMode() const
	{
		return m_gridSizingMode;
	}

	VisibilityMode visibilityMode() const
	{
		return m_visibilityMode;
	}

	SortOrder sortOrder() const
	{
		return m_sortOrder;
	}

	AspectRatio aspectRatio() const
	{
		return m_aspectRatio;
	}

private:
	DisplayRoleContent m_displayRoleContent;
	UidRoleContent m_uidRoleContent;
	GridSizingMode m_gridSizingMode;
	VisibilityMode m_visibilityMode;
	SortOrder m_sortOrder;
	AspectRatio m_aspectRatio;
};
