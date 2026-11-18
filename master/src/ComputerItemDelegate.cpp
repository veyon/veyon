// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ComputerItemDelegate.h"
#include "ComputerControlListModel.h"
#include "DesktopIcons.h"
#include "FeatureManager.h"

#include <QAbstractProxyModel>
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QWidget>


namespace
{
qreal fontScale(const QStyleOptionViewItem& option)
{
	return qMax(1.0, (option.fontMetrics.height() - 4) / 16.0);
}

QFont itemFont(const QStyleOptionViewItem& option, int pixels, bool bold = false)
{
	auto font = option.font;
	font.setPixelSize(qRound(pixels * fontScale(option)));
	font.setBold(bold);
	return font;
}

int cardWidth(const QStyleOptionViewItem& option)
{
	const auto width = option.widget ? option.widget->property("cardWidth").toInt() : 0;
	return qMax(140, width > 0 ? width : option.decorationSize.width());
}

bool listMode(const QStyleOptionViewItem& option)
{
	return option.widget && option.widget->property("listMode").toBool();
}

void drawIcon(QPainter* painter, const QRect& rect, const QString& name, const QColor& color)
{
	auto pixmap = DesktopIcons::icon(name).pixmap(rect.size(), painter->device()->devicePixelRatioF());
	if (pixmap.isNull()) return;
	QPainter tint(&pixmap);
	tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
	tint.fillRect(pixmap.rect(), color);
	tint.end();
	painter->drawPixmap(rect, pixmap);
}

int titleHeight(const QStyleOptionViewItem& option, const QModelIndex& index, bool fullLength)
{
	const auto metrics = QFontMetrics(itemFont(option, 13, true));
	auto height = metrics.height();
	if (fullLength && index.model())
	{
		const auto width = qMax(40, cardWidth(option) / 2 - 22);
		for (int row = 0; row < index.model()->rowCount(); ++row)
		{
			const auto item = index.model()->index(row, 0);
			const auto control = item.data(ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
			const auto name = control ? control->computerName() : item.data().toString();
			height = qMax(height, metrics.boundingRect(QRect(0, 0, width, 0), Qt::TextWordWrap, name).height());
		}
	}
	return height;
}

void drawText(QPainter* painter, const QRect& rect, const QString& text, const QFont& font, const QColor& color)
{
	if (rect.width() <= 0) return;
	painter->setFont(font);
	painter->setPen(color);
	painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter,
		QFontMetrics(font).elidedText(text, Qt::ElideRight, rect.width()));
}

void drawCheck(QPainter* painter, const QRect& rect, bool selected, const QPalette& palette)
{
	painter->setPen(QPen(selected ? palette.color(QPalette::Highlight) : palette.color(QPalette::Mid), 1.5));
	painter->setBrush(palette.color(selected ? QPalette::Highlight : QPalette::Base));
	painter->drawRect(QRectF(rect).adjusted(.75, .75, -.75, -.75));
	if (selected) drawIcon(painter, rect.adjusted(2, 2, -2, -2), QStringLiteral("check"), palette.color(QPalette::HighlightedText));
}

bool screenVisible(const QAbstractItemModel* model)
{
	while (const auto proxy = qobject_cast<const QAbstractProxyModel*>(model)) model = proxy->sourceModel();
	const auto computerModel = qobject_cast<const ComputerListModel*>(model);
	return computerModel && computerModel->visibilityMode() != ComputerListModel::VisibilityMode::Hidden;
}

struct ComputerStatus
{
	QString text;
	QString icon;
	QColor color;
	bool online = false;
};

ComputerStatus computerStatus(const ComputerControlInterface::Pointer& control, const QPalette& palette)
{
	const auto muted = palette.color(QPalette::PlaceholderText);
	if (!control) return { ComputerItemDelegate::tr("Disconnected"), QStringLiteral("monitor"), muted };
	using State = ComputerControlInterface::State;
	switch (control->state())
	{
	case State::Connected:
		for (const auto& uid : control->activeFeatures())
		{
			const auto name = VeyonCore::featureManager().feature(uid).name();
			if (name.contains(QLatin1String("Lock")))
				return { ComputerItemDelegate::tr("Locked"), QStringLiteral("lock"), palette.color(QPalette::Text) };
		}
		for (const auto& uid : control->activeFeatures())
		{
			const auto name = VeyonCore::featureManager().feature(uid).name();
			if (name.contains(QLatin1String("Demo")) || name.contains(QLatin1String("Share")))
				return { ComputerItemDelegate::tr("Watching demo"), QStringLiteral("cast"), palette.color(QPalette::Text) };
		}
		return { ComputerItemDelegate::tr("Online"), QStringLiteral("circle"), QColor(QStringLiteral("#438358")), true };
	case State::Connecting:
		return { ComputerItemDelegate::tr("Connecting…"), QStringLiteral("rotate-ccw"), muted };
	case State::HostOffline:
		return { ComputerItemDelegate::tr("Offline"), QStringLiteral("power-off"), muted };
	case State::HostNameResolutionFailed:
		return { ComputerItemDelegate::tr("Unknown host"), QStringLiteral("globe"), muted };
	case State::ServerNotRunning:
		return { ComputerItemDelegate::tr("Service unavailable"), QStringLiteral("monitor"), muted };
	case State::AuthenticationFailed:
	case State::AccessControlFailed:
		return { ComputerItemDelegate::tr("Access denied"), QStringLiteral("key-round"), palette.color(QPalette::Highlight) };
	default:
		return { ComputerItemDelegate::tr("Disconnected"), QStringLiteral("monitor"), muted };
	}
}

void drawStatus(QPainter* painter, const QRect& rect, const ComputerStatus& status, const QFont& font)
{
	const auto iconSize = qMax(12, QFontMetrics(font).height() - 2);
	const QRect symbol(rect.left(), rect.center().y() - iconSize / 2, iconSize, iconSize);
	if (status.online)
	{
		painter->setPen(Qt::NoPen);
		painter->setBrush(status.color);
		painter->drawEllipse(QRectF(symbol).adjusted(2.5, 2.5, -2.5, -2.5));
	}
	else drawIcon(painter, symbol, status.icon, status.color);
	drawText(painter, rect.adjusted(iconSize + 4, 0, 0, 0), status.text, font, status.color);
}
}

ComputerItemDelegate::ComputerItemDelegate(bool fullLengthLabels, QObject* parent) :
	QStyledItemDelegate(parent), m_fullLengthLabels(fullLengthLabels)
{
}

void ComputerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid()) return;
	const auto control = index.data(ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
	const auto selected = option.state.testFlag(QStyle::State_Selected);
	const auto hovered = option.state.testFlag(QStyle::State_MouseOver);
	const auto status = computerStatus(control, option.palette);
	const auto text = option.palette.color(QPalette::Text);
	const auto muted = option.palette.color(QPalette::PlaceholderText);
	const auto accent = option.palette.color(QPalette::Highlight);
	const auto titleFont = itemFont(option, 13, true);
	const auto userFont = itemFont(option, 12);
	const auto statusFont = itemFont(option, 11);
	const auto name = control ? control->computerName() : index.data().toString();
	const auto user = control && !control->userFullName().isEmpty() ? control->userFullName() :
		control && !control->userLoginName().isEmpty() ? control->userLoginName() : QStringLiteral("—");
	painter->save();
	painter->setClipRect(option.rect);
	painter->setRenderHint(QPainter::Antialiasing);

	if (listMode(option))
	{
		const auto row = option.rect;
		auto background = option.palette.color(QPalette::Base);
		if (selected || hovered)
		{
			auto overlay = selected ? accent : text;
			overlay.setAlpha(selected ? 26 : 13);
			painter->fillRect(row, background);
			painter->fillRect(row, overlay);
	}
		else painter->fillRect(row, background);
		painter->setPen(option.palette.color(QPalette::Mid));
		painter->drawLine(row.bottomLeft(), row.bottomRight());
		const auto x = row.left() + 12;
		const auto width = row.width() - 24;
		const auto factor = qMin(1.0, qMax(.45, width / 850.0));
		const int checkWidth = qRound(36 * factor);
		const int nameWidth = qRound(110 * factor);
		const int statusWidth = qRound(190 * factor);
		const int addressWidth = qRound(130 * factor);
		const int osWidth = qRound(120 * factor);
		const int userWidth = qMax(0, width - checkWidth - nameWidth - statusWidth - addressWidth - osWidth);
		drawCheck(painter, QRect(x, row.center().y() - 7, 15, 15), selected, option.palette);
		int column = x + checkWidth;
		drawText(painter, QRect(column, row.top(), nameWidth - 8, row.height()), name, titleFont, text);
		column += nameWidth;
		drawText(painter, QRect(column, row.top(), userWidth - 8, row.height()), user, itemFont(option, 13), text);
		column += userWidth;
		drawStatus(painter, QRect(column, row.top(), statusWidth - 8, row.height()), status, itemFont(option, 13));
		column += statusWidth;
		auto mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		mono.setPixelSize(userFont.pixelSize());
		drawText(painter, QRect(column, row.top(), addressWidth - 8, row.height()),
			control ? control->computer().hostName() : QStringLiteral("—"), mono, muted);
		column += addressWidth;
		drawText(painter, QRect(column, row.top(), osWidth, row.height()), QStringLiteral("—"), userFont, muted);
	}
	else
	{
		const auto card = QRect(option.rect.topLeft() + QPoint(1, 1),
			QSize(qMin(cardWidth(option), option.rect.width()) - 2, option.rect.height() - 2));
		const auto imageHeight = card.width() * 10 / 16;
		const QRect imageRect(card.left(), card.top(), card.width(), imageHeight);
		painter->fillRect(card, option.palette.color(QPalette::AlternateBase));
		painter->fillRect(imageRect, option.palette.color(QPalette::Window));
		const auto connected = control && control->state() == ComputerControlInterface::State::Connected;
		if (connected && control->hasValidFramebuffer() && screenVisible(index.model()))
		{
			const auto image = index.data(Qt::DecorationRole).value<QImage>();
			if (!image.isNull())
			{
				const auto size = image.size().scaled(imageRect.size(), Qt::KeepAspectRatio);
				painter->drawImage(QRect(imageRect.center() - QPoint(size.width() / 2, size.height() / 2), size), image);
			}
		}
		else
		{
			const auto overlayIcon = connected ? QStringLiteral("monitor") : status.icon;
			const auto center = imageRect.center();
			drawIcon(painter, QRect(center.x() - 10, center.y() - 22, 20, 20), overlayIcon, status.color);
			painter->setFont(statusFont);
			painter->setPen(status.color);
			painter->drawText(QRect(imageRect.left() + 8, center.y() + 3, imageRect.width() - 16, QFontMetrics(statusFont).height()),
				Qt::AlignHCenter | Qt::AlignVCenter, connected ? (screenVisible(index.model()) ? tr("Screen unavailable") : tr("Screen hidden")) : status.text);
		}
		if (connected && !status.online)
		{
			const auto overlayWidth = qMin(imageRect.width() - 16, QFontMetrics(statusFont).horizontalAdvance(status.text) + 20);
			const auto overlayHeight = 36 + QFontMetrics(statusFont).height();
			const QRect overlay(imageRect.center() - QPoint(overlayWidth / 2, overlayHeight / 2), QSize(overlayWidth, overlayHeight));
			const auto locked = status.icon == QLatin1String("lock");
			const auto foreground = option.palette.color(locked ? QPalette::Base : QPalette::Text);
			painter->fillRect(overlay, option.palette.color(locked ? QPalette::Text : QPalette::Base));
			drawIcon(painter, QRect(overlay.center().x() - 10, overlay.top() + 6, 20, 20), status.icon, foreground);
			painter->setFont(statusFont);
			painter->setPen(foreground);
			painter->drawText(overlay.adjusted(6, 30, -6, -4), Qt::AlignCenter,
				QFontMetrics(statusFont).elidedText(status.text, Qt::ElideRight, overlay.width() - 12));
		}
		painter->setPen(option.palette.color(QPalette::Mid));
		painter->drawLine(imageRect.bottomLeft(), imageRect.bottomRight());
		drawCheck(painter, QRect(card.left() + 6, card.top() + 6, 16, 16), selected, option.palette);
		if (connected)
		{
			int featureX = imageRect.right() - 22;
			QStringList shownIcons;
			for (const auto& uid : control->activeFeatures())
			{
				if (featureX < imageRect.left() + 30) break;
				const auto activeFeature = VeyonCore::featureManager().feature(uid);
				const auto feature = activeFeature.name();
				if (!activeFeature.isValid() || !activeFeature.testFlag(Feature::Flag::Master) ||
					activeFeature.testFlag(Feature::Flag::Meta) || activeFeature.iconUrl().isEmpty() ||
					feature == QStringLiteral("MonitoringMode") || feature == QStringLiteral("Monitoring")) continue;
				const auto icon = feature.contains(QLatin1String("Lock")) ? QStringLiteral("lock") :
					feature.contains(QLatin1String("Demo")) || feature.contains(QLatin1String("Share")) ? QStringLiteral("cast") :
					feature.contains(QLatin1String("Remote")) ? QStringLiteral("mouse-pointer-2") :
					feature.contains(QLatin1String("File")) ? QStringLiteral("files") : QStringLiteral("circle-check");
				if (shownIcons.contains(icon)) continue;
				shownIcons.append(icon);
				painter->fillRect(QRect(featureX - 2, imageRect.top() + 4, 20, 20), option.palette.color(QPalette::Base));
				drawIcon(painter, QRect(featureX, imageRect.top() + 6, 16, 16), icon, text);
				featureX -= 24;
			}
		}
		const auto footerTop = imageRect.bottom() + 5;
		const auto lineHeight = qMax(titleHeight(option, index, m_fullLengthLabels), QFontMetrics(statusFont).height());
		const auto statusWidth = qMin(card.width() / 2, QFontMetrics(statusFont).horizontalAdvance(status.text) + 20);
		const QRect titleRect(card.left() + 8, footerTop, card.width() - statusWidth - 22, lineHeight);
		if (m_fullLengthLabels)
		{
			painter->setFont(titleFont);
			painter->setPen(text);
			painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, name);
		}
		else drawText(painter, titleRect, name, titleFont, text);
		drawStatus(painter, QRect(card.right() - statusWidth - 7, footerTop, statusWidth, lineHeight), status, statusFont);
		drawText(painter, QRect(card.left() + 8, footerTop + lineHeight + 1, card.width() - 16,
			qMax(0, card.bottom() - footerTop - lineHeight - 4)), user, userFont, muted);
		painter->setPen(QPen(selected ? accent : hovered ? text : option.palette.color(QPalette::Mid), selected ? 2 : 1));
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(QRectF(card).adjusted(.5, .5, -.5, -.5));
	}
	if (option.state.testFlag(QStyle::State_HasFocus) &&
		(option.state.testFlag(QStyle::State_KeyboardFocusChange) ||
		 (option.widget && option.widget->testAttribute(Qt::WA_KeyboardFocusChange))))
	{
		painter->setPen(QPen(accent, 1));
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(option.rect.adjusted(3, 3, -3, -3));
	}
	painter->restore();
}

QSize ComputerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	const auto nameHeight = titleHeight(option, index, !listMode(option) && m_fullLengthLabels);
	const auto userHeight = QFontMetrics(itemFont(option, 12)).height();
	if (listMode(option)) return QSize(cardWidth(option), qMax(30, nameHeight + 10));
	const auto width = cardWidth(option);
	return QSize(width, width * 10 / 16 + qMax(44, nameHeight + userHeight + 12));
}
