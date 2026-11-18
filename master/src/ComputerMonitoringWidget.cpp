// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QCheckBox>
#include <QPainter>
#include <QSignalBlocker>
#include <QStringList>
#include <QMenu>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>

#include "ComputerControlListModel.h"
#include "ComputerItemDelegate.h"
#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringWidget.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"


namespace
{
class ComputerListHeader : public QWidget
{
public:
	ComputerListHeader(const QStringList& labels, QWidget* parent) : QWidget(parent), m_labels(labels)
	{
		setObjectName(QStringLiteral("computerListHeader"));
		setFixedHeight(30);
		setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	}

protected:
	void paintEvent(QPaintEvent*) override
	{
		QPainter painter(this);
		painter.fillRect(rect(), palette().color(QPalette::Window));
		painter.setPen(QPen(palette().color(QPalette::Mid), 2));
		painter.drawLine(rect().bottomLeft(), rect().bottomRight());
		auto labelFont = font();
		labelFont.setPixelSize(11);
		labelFont.setWeight(QFont::DemiBold);
		labelFont.setLetterSpacing(QFont::AbsoluteSpacing, .6);
		painter.setFont(labelFont);
		painter.setPen(palette().color(QPalette::PlaceholderText));
		const QFontMetrics metrics(labelFont);
		const auto available = width() - 24;
		const auto factor = qMin(1.0, qMax(.45, available / 850.0));
		const int checkWidth = qRound(36 * factor);
		const int nameWidth = qRound(110 * factor);
		const int statusWidth = qRound(190 * factor);
		const int addressWidth = qRound(130 * factor);
		const int osWidth = qRound(120 * factor);
		const int userWidth = qMax(0, available - checkWidth - nameWidth - statusWidth - addressWidth - osWidth);
		const int widths[] = {nameWidth, userWidth, statusWidth, addressWidth, osWidth};
		int column = 12 + checkWidth;
		for (int i = 0; i < m_labels.size(); ++i)
		{
			const auto textWidth = qMax(0, widths[i] - 8);
			painter.drawText(QRect(column, 0, textWidth, height()), Qt::AlignLeft | Qt::AlignVCenter,
				metrics.elidedText(m_labels[i].toUpper(), Qt::ElideRight, textWidth));
			column += widths[i];
		}
	}

private:
	const QStringList m_labels;
};
}

ComputerMonitoringWidget::ComputerMonitoringWidget( QWidget *parent ) :
	FlexibleListView( parent ),
	m_featureMenu(new QMenu(this)),
	m_itemsLayoutDebounceTimer(new QTimer(this))
{
	setContextMenuPolicy( Qt::CustomContextMenu );
	setAcceptDrops( true );
	setDragEnabled( true );
	setDragDropMode( QAbstractItemView::DropOnly );
	setDefaultDropAction( Qt::MoveAction );
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	setFlow( QListView::LeftToRight );
	setWrapping( true );
	setResizeMode( QListView::Adjust );
	setSpacing(6);
	setViewportMargins(8, 8, 8, 8);
	setViewMode( QListView::IconMode );
	setUniformItemSizes( true );
	setSelectionRectVisible( true );
	setMouseTracking(true);
	setFrameShape(QFrame::NoFrame);

	const auto fullLengthLabels = VeyonCore::config().computerMonitoringGridSizingMode() ==
		ComputerListModel::GridSizingMode::FullLengthLabels;
	setTextElideMode(fullLengthLabels ? Qt::ElideNone : Qt::ElideRight);
	setItemDelegate(new ComputerItemDelegate(fullLengthLabels, this));

	setUidRole(ComputerControlListModel::UidRole);

	connect(this, &QListView::doubleClicked, this, &ComputerMonitoringWidget::runDoubleClickFeature);
	connect(this, &QListView::customContextMenuRequested,
			this, [this](QPoint pos) { showContextMenu(mapToGlobal(pos)); });

	connect(dataModel(), &ComputerMonitoringModel::dataChanged,
			this, &ComputerMonitoringWidget::handleSizeHintChanges);

	m_itemsLayoutDebounceTimer->setInterval(ItemsLayoutDebounceTimeout);
	m_itemsLayoutDebounceTimer->setSingleShot(true);
	connect(m_itemsLayoutDebounceTimer, &QTimer::timeout, this, &ComputerMonitoringWidget::doItemsLayout);

	initializeView(this);

	setModel(dataModel());
	m_listHeader = new ComputerListHeader({tr("Computer"), tr("User"), tr("Status"), tr("Address"), tr("OS")}, this);
	m_listHeader->hide();
	m_selectAllComputers = new QCheckBox(m_listHeader);
	m_selectAllComputers->setObjectName(QStringLiteral("selectAllComputers"));
	m_selectAllComputers->setAccessibleName(tr("Select all computers"));
	m_selectAllComputers->setToolTip(tr("Select all computers"));
	m_selectAllComputers->setTristate(true);
	m_selectAllComputers->setGeometry(12, 7, 16, 16);
	connect(m_selectAllComputers, &QCheckBox::clicked, this, [this](bool checked) {
		if (checked) selectAll();
		else clearSelection();
		updateListHeaderSelection();
	});
	connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ComputerMonitoringWidget::updateListHeaderSelection);
	connect(model(), &QAbstractItemModel::modelReset, this, &ComputerMonitoringWidget::updateListHeaderSelection);
	connect(model(), &QAbstractItemModel::rowsInserted, this, &ComputerMonitoringWidget::updateListHeaderSelection);
	connect(model(), &QAbstractItemModel::rowsRemoved, this, &ComputerMonitoringWidget::updateListHeaderSelection);
	updateListHeaderSelection();
	updateCardLayout();
}



ComputerControlInterfaceList ComputerMonitoringWidget::selectedComputerControlInterfaces() const
{
	ComputerControlInterfaceList computerControlInterfaces;

	const auto selectedIndices = selectionModel()->selectedIndexes(); // clazy:exclude=inefficient-qlist
	computerControlInterfaces.reserve(selectedIndices.size());

	for (const auto& index : selectedIndices)
	{
		computerControlInterfaces.append(model()->data(index, ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>());
	}

	return computerControlInterfaces;
}



bool ComputerMonitoringWidget::performIconSizeAutoAdjust()
{
	if (property("listMode").toBool())
	{
		return false;
	}

	if (isVisible() == false)
	{
		initiateIconSizeAutoAdjust();
		return false;
	}

	if (ComputerMonitoringView::performIconSizeAutoAdjust() == false)
	{
		return false;
	}

	m_ignoreResizeEvent = true;

	auto size = iconSize().width();

	setCardMinimumWidth(size);
	QApplication::processEvents();

	while (verticalScrollBar()->isVisible() == false &&
		   horizontalScrollBar()->isVisible() == false &&
		   size < MaximumComputerScreenSize)
	{
		size += IncreaseIconSizeStepSize;
		setCardMinimumWidth(size);
		QApplication::processEvents();
	}

	while ((verticalScrollBar()->isVisible() ||
			horizontalScrollBar()->isVisible()) &&
		   size > MinimumComputerScreenSize)
	{
		const auto scrollMax = std::max(verticalScrollBar()->maximum(), horizontalScrollBar()->maximum());
		size -= std::max(DecreaseIconSizeMinimumStepSize, std::min(size / 10, scrollMax / 5));
		setCardMinimumWidth(size);
		QApplication::processEvents();
	}

	Q_EMIT computerScreenSizeAdjusted(size);

	m_ignoreResizeEvent = false;

	return true;
}



void ComputerMonitoringWidget::handleSizeHintChanges(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	Q_UNUSED(topLeft);
	Q_UNUSED(bottomRight);

	if (roles.contains(Qt::SizeHintRole))
	{
		m_itemsLayoutDebounceTimer->start();
	}
}



void ComputerMonitoringWidget::setUseCustomComputerPositions(bool enabled)
{
	setFlexible(enabled);
	initiateIconSizeAutoAdjust();
}



void ComputerMonitoringWidget::alignComputers()
{
	alignToGrid();
}



void ComputerMonitoringWidget::showContextMenu(QPoint globalPos)
{
	populateFeatureMenu(selectedComputerControlInterfaces());

	m_featureMenu->exec(globalPos);
}



void ComputerMonitoringWidget::setIconSize(const QSize& size)
{
	QAbstractItemView::setIconSize(size);
}



void ComputerMonitoringWidget::setColors(const QColor& backgroundColor, const QColor& textColor)
{
	auto pal = palette();
	if (backgroundColor == QColor(Qt::white) && textColor == QColor(Qt::black))
	{
		pal.setColor(QPalette::Base, qApp->palette().color(QPalette::Base));
		pal.setColor(QPalette::Text, qApp->palette().color(QPalette::Text));
	}
	else if (VeyonCore::useDarkMode())
	{
		pal.setColor(QPalette::Base, textColor);
		pal.setColor(QPalette::Text, backgroundColor);
	}
	else
	{
		pal.setColor(QPalette::Base, backgroundColor);
		pal.setColor(QPalette::Text, textColor);
	}
	setPalette(pal);
}



QJsonArray ComputerMonitoringWidget::saveComputerPositions()
{
	return savePositions();
}



bool ComputerMonitoringWidget::useCustomComputerPositions()
{
	return flexible();
}



void ComputerMonitoringWidget::loadComputerPositions(const QJsonArray& positions)
{
	loadPositions(positions);
}



void ComputerMonitoringWidget::populateFeatureMenu(const ComputerControlInterfaceList& computerControlInterfaces)
{
	m_featureMenuTargets = computerControlInterfaces;
	Plugin::Uid previousPluginUid;

	m_featureMenu->clear();

	for (const auto& feature : master()->features())
	{
		if (feature.testFlag(Feature::Flag::Meta))
		{
			continue;
		}

		Plugin::Uid pluginUid = VeyonCore::featureManager().pluginUid(feature.uid());

		if (previousPluginUid.isNull() == false &&
			pluginUid != previousPluginUid &&
			feature.testFlag(Feature::Flag::Mode) == false)
		{
			m_featureMenu->addSeparator();
		}

		previousPluginUid = pluginUid;

		if (!feature.displayNameActive().isEmpty() && !computerControlInterfaces.isEmpty() &&
			std::all_of(computerControlInterfaces.cbegin(), computerControlInterfaces.cend(), [this, &feature](const auto& computer) {
				return isFeatureOrRelatedFeatureActive({computer}, feature.uid());
			}))
		{
			addFeatureToMenu(feature, feature.displayNameActive());
		}
		else
		{
			const auto subFeatures = master()->subFeatures(feature.uid());
			if (subFeatures.isEmpty())
			{
				addFeatureToMenu(feature, feature.displayName());
			}
			else
			{
				addSubFeaturesToMenu(feature, subFeatures, feature.displayName());
			}
		}
	}
}



void ComputerMonitoringWidget::addFeatureToMenu(const Feature& feature, const QString& label)
{
	const auto targets = m_featureMenuTargets;
	m_featureMenu->addAction(QIcon(feature.iconUrl()),
							 label,
							 this, [this, feature, targets]() { runFeatureOnComputers(feature, targets); });
}



void ComputerMonitoringWidget::addSubFeaturesToMenu(const Feature& parentFeature, const FeatureList& subFeatures, const QString& label)
{
	const auto targets = m_featureMenuTargets;
	auto menu = m_featureMenu->addMenu( QIcon( parentFeature.iconUrl() ), label );

	for (const auto& subFeature : subFeatures)
	{
		menu->addAction(QIcon(subFeature.iconUrl()), subFeature.displayName(),
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
						subFeature.shortcut(),
#endif
						 this, [this, subFeature, targets]() { runFeatureOnComputers(subFeature, targets, false); }
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
						,
						subFeature.shortcut()
#endif
		);
	}
}



void ComputerMonitoringWidget::runDoubleClickFeature(const QModelIndex& index)
{
	const Feature& feature = VeyonCore::featureManager().feature(VeyonCore::config().computerDoubleClickFeature());

	if (index.isValid() && feature.isValid())
	{
		selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
		runFeature(feature);
	}
}



void ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents()
{
	m_ignoreNumberOfMouseEvents = IgnoredNumberOfMouseEventsWhileHold;
}



void ComputerMonitoringWidget::runMousePressAndHoldFeature()
{
	m_mousePressAndHold.stop();
	const auto selectedInterfaces = selectedComputerControlInterfaces();
	if (!m_ignoreMousePressAndHoldEvent &&
		selectedInterfaces.count() > 0 &&
		selectedInterfaces.count() < 2 &&
		selectedInterfaces.first()->state() == ComputerControlInterface::State::Connected &&
		selectedInterfaces.first()->hasValidFramebuffer())
	{
		m_ignoreMousePressAndHoldEvent = true;
		resetIgnoreNumberOfMouseEvents();
		delete m_computerZoomWidget;
		m_computerZoomWidget = new ComputerZoomWidget(selectedInterfaces.first());
		connect(m_computerZoomWidget, &ComputerZoomWidget::keypressInComputerZoomWidget, this, &ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents);
	}
}



void ComputerMonitoringWidget::stopMousePressAndHoldFeature()
{
	m_ignoreMousePressAndHoldEvent = false;
	m_ignoreNumberOfMouseEvents = 0;
	if (m_computerZoomWidget)
	{
		disconnect(m_computerZoomWidget, &ComputerZoomWidget::keypressInComputerZoomWidget, this, &ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents);
		m_computerZoomWidget->close();
		delete m_computerZoomWidget;
		m_computerZoomWidget = nullptr;
	}
}



void ComputerMonitoringWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() == Qt::LeftButton && indexAt(event->pos()).isValid())
	{
		if (!m_ignoreMousePressAndHoldEvent)
		{
			m_mousePressAndHold.setInterval(500);
			m_mousePressAndHold.start();
			connect(&m_mousePressAndHold, &QTimer::timeout, this, &ComputerMonitoringWidget::runMousePressAndHoldFeature, Qt::UniqueConnection);
		}
	}
	QListView::mousePressEvent(event);
}



void ComputerMonitoringWidget::mouseReleaseEvent(QMouseEvent* event)
{
	m_mousePressAndHold.stop();
	if (m_ignoreMousePressAndHoldEvent)
	{
		stopMousePressAndHoldFeature();
	}
	QListView::mouseReleaseEvent(event);
}



void ComputerMonitoringWidget::mouseMoveEvent(QMouseEvent* event)
{
	m_mousePressAndHold.stop();
	if (m_ignoreNumberOfMouseEvents <= 0)
	{
		if (m_ignoreMousePressAndHoldEvent)
		{
			stopMousePressAndHoldFeature();
		}

		QListView::mouseMoveEvent(event);
	}
	else
	{
		m_ignoreNumberOfMouseEvents--;
		event->accept();
	}
}



void ComputerMonitoringWidget::resizeEvent(QResizeEvent* event)
{
	FlexibleListView::resizeEvent(event);
	updateCardLayout();

	if (m_ignoreResizeEvent == false)
	{
		initiateIconSizeAutoAdjust();
	}
}



void ComputerMonitoringWidget::showEvent(QShowEvent* event)
{
	if (event->spontaneous() == false)
	{
		initiateIconSizeAutoAdjust();
	}

	FlexibleListView::showEvent(event);
}



void ComputerMonitoringWidget::wheelEvent(QWheelEvent* event)
{
	if (m_ignoreWheelEvent == false &&
		event->modifiers().testFlag(Qt::ControlModifier))
	{
		setCardMinimumWidth(m_cardMinimumWidth + event->angleDelta().y() / 8);

		Q_EMIT computerScreenSizeAdjusted(computerScreenSize());

		event->accept();
	}
	else
	{
		QListView::wheelEvent(event);
	}
}

void ComputerMonitoringWidget::setListMode(bool enabled)
{
	setProperty("listMode", enabled);
	setViewportMargins(8, enabled ? 38 : 8, 8, 8);
	m_listHeader->setVisible(enabled);
	setViewMode(enabled ? QListView::ListMode : QListView::IconMode);
	setFlow(enabled ? QListView::TopToBottom : QListView::LeftToRight);
	setWrapping(!enabled);
	setSpacing(enabled ? 0 : 6);
	updateCardLayout();
}

void ComputerMonitoringWidget::setCardMinimumWidth(int width)
{
	m_cardMinimumWidth = qBound(150, width, 1000);
	setComputerScreenSize(m_cardMinimumWidth);
	updateCardLayout();
}

void ComputerMonitoringWidget::updateCardLayout()
{
	if (!model()) return;
	if (m_listHeader)
	{
		m_listHeader->setGeometry(viewport()->geometry().left(), 8, viewport()->width(), 30);
		m_listHeader->raise();
	}
	if (property("listMode").toBool())
	{
		setProperty("cardWidth", qMax(140, viewport()->width()));
		QStyleOptionViewItem option;
		option.initFrom(this);
		option.font = font();
		option.fontMetrics = fontMetrics();
		option.widget = this;
		setGridSize(QSize(viewport()->width(), qMax(30, itemDelegate()->sizeHint(option, model()->index(0, 0)).height())));
	}
	else if (!flexible())
	{
		const int available = qMax(150, viewport()->width() - 13);
		const int columns = qMax(1, (available + 12) / (m_cardMinimumWidth + 12));
		const int width = (available - (columns - 1) * 12) / columns;
		setProperty("cardWidth", width);
		QStyleOptionViewItem option;
		option.initFrom(this);
		option.font = font();
		option.fontMetrics = fontMetrics();
		option.widget = this;
		setGridSize(QSize(width + 12, itemDelegate()->sizeHint(option, model()->index(0, 0)).height() + 12));
	}
	else
	{
		setProperty("cardWidth", iconSize().width());
		setGridSize({});
	}
	doItemsLayout();
	viewport()->update();
}

void ComputerMonitoringWidget::updateListHeaderSelection()
{
	if (!m_selectAllComputers || !selectionModel() || !model()) return;
	const auto count = model()->rowCount();
	const auto selected = selectionModel()->selectedRows().size();
	const QSignalBlocker blocker(m_selectAllComputers);
	m_selectAllComputers->setEnabled(count > 0);
	m_selectAllComputers->setCheckState(selected == 0 ? Qt::Unchecked : selected == count ? Qt::Checked : Qt::PartiallyChecked);
}
