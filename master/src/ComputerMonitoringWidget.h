// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>
#include <QWidget>

#include "ComputerMonitoringView.h"
#include "ComputerZoomWidget.h"
#include "FlexibleListView.h"

class FlexibleListView;
class QTimer;

class ComputerMonitoringWidget : public FlexibleListView, public ComputerMonitoringView
{
	Q_OBJECT
public:
	explicit ComputerMonitoringWidget(QWidget* parent = nullptr);
	~ComputerMonitoringWidget() override = default;

	ComputerControlInterfaceList selectedComputerControlInterfaces() const override;

	void setUseCustomComputerPositions(bool enabled) override;
	void alignComputers() override;

	void showContextMenu(QPoint globalPos);

	void setIconSize(const QSize& size) override;

	void setIgnoreWheelEvent(bool enabled)
	{
		m_ignoreWheelEvent = enabled;
	}

	void resetIgnoreNumberOfMouseEvents();

	QTimer m_mousePressAndHold;

private:
	void setColors(const QColor& backgroundColor, const QColor& textColor) override;
	QJsonArray saveComputerPositions() override;
	bool useCustomComputerPositions() override;
	void loadComputerPositions(const QJsonArray& positions) override;

	bool performIconSizeAutoAdjust() override;

	void handleSizeHintChanges(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

	void populateFeatureMenu(const ComputerControlInterfaceList& computerControlInterfaces);
	void addFeatureToMenu(const Feature& feature, const QString& label);
	void addSubFeaturesToMenu(const Feature& parentFeature, const FeatureList& subFeatures, const QString& label);

	void runDoubleClickFeature(const QModelIndex& index);
	void runMousePressAndHoldFeature();
	void stopMousePressAndHoldFeature();

	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	QMenu* m_featureMenu{};
	bool m_ignoreMousePressAndHoldEvent{false};
	bool m_ignoreWheelEvent{false};
	bool m_ignoreResizeEvent{false};
	int m_ignoreNumberOfMouseEvents = 0;

	static constexpr auto IgnoredNumberOfMouseEventsWhileHold = 3;
	static constexpr auto ItemsLayoutDebounceTimeout = 250;

	QPointer<ComputerZoomWidget> m_computerZoomWidget;

	QTimer* m_itemsLayoutDebounceTimer;

Q_SIGNALS:
	void computerScreenSizeAdjusted(int size);
};
