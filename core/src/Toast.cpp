/*
 * File based on https://github.com/niklashenning/qt-toast/blob/master/src/Toast.cpp
 *
 * MIT License

 * Copyright (c) 2024 Niklas Henning

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QStyleHints>
#include <QTimer>

#include "Toast.h"
#include "VeyonCore.h"

// Static
int Toast::s_maximumOnScreen = 3;
int Toast::s_spacing = 10;
int Toast::s_offsetX = 20;
int Toast::s_offsetY = 45;
bool Toast::s_alwaysOnMainScreen = false;
QScreen* Toast::s_fixedScreen = nullptr;
Toast::Position Toast::s_position = Toast::Position::BottomRight;
std::deque<Toast*> Toast::s_currentlyShown = std::deque<Toast*>();
std::deque<Toast*> Toast::s_queue = std::deque<Toast*>();

// Static constants
const int Toast::sc_updatePositionDuration = 200;
const int Toast::sc_durationBarUpdateInterval = 5;
const int Toast::sc_dropShadowSize = 5;
const QColor Toast::sc_successAccentColor = QColor("#3E9141");
const QColor Toast::sc_warningAccentColor = QColor("#E8B849");
const QColor Toast::sc_errorAccentColor = QColor("#BA2626");
const QColor Toast::sc_informationAccentColor = QColor("#007FFF");
const QColor Toast::sc_defaultAccentColor = QColor("#5C5C5C");
const QColor Toast::sc_defaultBackgroundColor = QColor("#E7F4F9");
const QColor Toast::sc_defaultTitleColor = QColor("#000000");
const QColor Toast::sc_defaultTextColor = QColor("#5C5C5C");
const QColor Toast::sc_defaultIconSeparatorColor = QColor("#D9D9D9");
const QColor Toast::sc_defaultCloseButtonIconColor = QColor("#000000");
const QColor Toast::sc_defaultBackgroundColorDark = QColor("#292929");
const QColor Toast::sc_defaultTitleColorDark = QColor("#FFFFFF");
const QColor Toast::sc_defaultTextColorDark = QColor("#D0D0D0");
const QColor Toast::sc_defaultIconSeparatorColorDark = QColor("#585858");
const QColor Toast::sc_defaultCloseButtonIconColorDark = QColor("#C9C9C9");


Toast::Toast(QWidget* parent)
	: QDialog(parent)
{
	// Init attributes
	m_duration = 5000;
	m_showDurationBar = true;
	m_icon = getIconFromEnum(Icon::Information);
	m_showIcon = false;
	m_iconSize = QSize(18, 18);
	m_showIconSeparator = true;
	m_iconSeparatorWidth = 2;
	m_closeButtonIcon = getIconFromEnum(Icon::Close);
	m_showCloseButton = true;
	m_closeButtonIconSize = QSize(10, 10);
	m_closeButtonSize = QSize(24, 24);
	m_closeButtonAlignment = ButtonAlignment::Top;
	m_fadeInDuration = 250;
	m_fadeOutDuration = 250;
	m_resetDurationOnHover = true;
	m_stayOnTop = true;
	m_borderRadius = 0;
	m_backgroundColor = sc_defaultBackgroundColor;
	m_titleColor = sc_defaultTitleColor;
	m_textColor = sc_defaultTextColor;
	m_iconColor = sc_defaultAccentColor;
	m_iconSeparatorColor = sc_defaultIconSeparatorColor;
	m_closeButtonIconColor = sc_defaultCloseButtonIconColor;
	m_durationBarColor = sc_defaultAccentColor;
	m_titleFont = font();
	m_titleFont.setBold(true);
	m_textFont = font();
	m_margins = QMargins(20, 18, 10, 18);
	m_iconMargins = QMargins(0, 0, 15, 0);
	m_iconSectionMargins = QMargins(0, 0, 15, 0);
	m_textSectionMargins = QMargins(0, 0, 15, 0);
	m_closeButtonMargins = QMargins(0, -8, 0, -8);
	m_textSectionSpacing = 8;

	m_elapsedTime = 0;
	m_fadingOut = false;
	m_used = false;
	m_parent = parent;

	// Window settings
	setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground);
	setFocusPolicy(Qt::FocusPolicy::NoFocus);

	// Notification widget (QLabel because QWidget has weird behaviour with stylesheets)
	m_notification = new QLabel(this);

	// Drop shadow (has to be drawn manually since only one graphics effect can be applied)
	m_dropShadowLayer1 = new QWidget(this);
	m_dropShadowLayer2 = new QWidget(this);
	m_dropShadowLayer3 = new QWidget(this);
	m_dropShadowLayer4 = new QWidget(this);
	m_dropShadowLayer5 = new QWidget(this);
	m_dropShadowLayer1->setObjectName(QStringLiteral("toast-drop-shadow-layer-1"));
	m_dropShadowLayer2->setObjectName(QStringLiteral("toast-drop-shadow-layer-2"));
	m_dropShadowLayer3->setObjectName(QStringLiteral("toast-drop-shadow-layer-3"));
	m_dropShadowLayer4->setObjectName(QStringLiteral("toast-drop-shadow-layer-4"));
	m_dropShadowLayer5->setObjectName(QStringLiteral("toast-drop-shadow-layer-5"));

	// Opacity effect for fading animations
	m_opacityEffect = new QGraphicsOpacityEffect();
	m_opacityEffect->setOpacity(1);
	setGraphicsEffect(m_opacityEffect);

	// Close button
	m_closeButton = new QPushButton(m_notification);
	m_closeButton->setCursor(Qt::CursorShape::PointingHandCursor);
	m_closeButton->setObjectName(QStringLiteral("toast-close-button"));
	connect(m_closeButton, &QPushButton::clicked, this, &Toast::hide);

	// Text and title labels
	m_titleLabel = new QLabel(m_notification);
	m_textLabel = new QLabel(m_notification);

	// Icon (QPushButton instead of QLabel to get better icon quality)
	m_iconWidget = new QPushButton(m_notification);
	m_iconWidget->setObjectName(QStringLiteral("toast-icon-widget"));

	// Icon separator
	m_iconSeparator = new QWidget(m_notification);
	m_iconSeparator->setFixedWidth(2);

	// Duration bar container (used to make border radius possible on 4 px high widget)
	m_durationBarContainer = new QWidget(m_notification);
	m_durationBarContainer->setFixedHeight(4);
	m_durationBarContainer->setStyleSheet(QStringLiteral("background: transparent;"));

	// Duration bar
	m_durationBar = new QWidget(m_durationBarContainer);
	m_durationBar->setFixedHeight(20);
	m_durationBar->move(0, -16);

	// Duration bar chunk
	m_durationBarChunk = new QWidget(m_durationBarContainer);
	m_durationBarChunk->setFixedHeight(20);
	m_durationBarChunk->move(0, -16);

	// Set defaults
	setIcon(m_icon);
	setIconSize(m_iconSize);
	setIconColor(m_iconColor);
	setIconSeparatorWidth(m_iconSeparatorWidth);
	setCloseButtonIcon(m_closeButtonIcon);
	setCloseButtonIconSize(m_closeButtonIconSize);
	setCloseButtonSize(m_closeButtonSize);
	setCloseButtonAlignment(m_closeButtonAlignment);
	setStayOnTop(m_stayOnTop);
	setBackgroundColor(m_backgroundColor);
	setTitleColor(m_titleColor);
	setTextColor(m_textColor);
	setBorderRadius(m_borderRadius);
	setIconSeparatorColor(m_iconSeparatorColor);
	setCloseButtonIconColor(m_closeButtonIconColor);
	setDurationBarColor(m_durationBarColor);
	setTitleFont(m_titleFont);
	setTextFont(m_textFont);

	// Timer for hiding the notification after set duration
	m_durationTimer = new QTimer(this);
	m_durationTimer->setSingleShot(true);
	connect(m_durationTimer, &QTimer::timeout, this, &Toast::fadeOut);

	// Timer for updating the duration bar
	m_durationBarTimer = new QTimer(this);
	connect(m_durationBarTimer, &QTimer::timeout, this, &Toast::updateDurationBar);

	// Apply stylesheet
	setStyleSheet(QStringLiteral(
					  "#toast-drop-shadow-layer-1 { background: rgba(0, 0, 0, 3); border-radius: 8px; }"
					  "#toast-drop-shadow-layer-2 { background: rgba(0, 0, 0, 5); border-radius: 8px; }"
					  "#toast-drop-shadow-layer-3 { background: rgba(0, 0, 0, 6); border-radius: 8px; }"
					  "#toast-drop-shadow-layer-4 { background: rgba(0, 0, 0, 9); border-radius: 8px; }"
					  "#toast-drop-shadow-layer-5 { background: rgba(0, 0, 0, 10); border-radius: 8px; }"
					  "#toast-close-button { background: transparent; }"
					  "#toast-icon-widget { background: transparent; }"
					  ));
}

Toast::~Toast()
{
}

int Toast::getMaximumOnScreen()
{
	return s_maximumOnScreen;
}

int Toast::getSpacing()
{
	return s_spacing;
}

QPoint Toast::getOffset()
{
	return QPoint(s_offsetX, s_offsetY);
}

int Toast::getOffsetX()
{
	return s_offsetX;
}

int Toast::getOffsetY()
{
	return s_offsetY;
}

bool Toast::isAlwaysOnMainScreen()
{
	return s_alwaysOnMainScreen;
}

QScreen* Toast::getFixedScreen()
{
	return s_fixedScreen;
}

Toast::Position Toast::getPosition()
{
	return s_position;
}

int Toast::getCount()
{
	return s_currentlyShown.size() + s_queue.size();
}

int Toast::getVisibleCount()
{
	return s_currentlyShown.size();
}

int Toast::getQueuedCount()
{
	return s_queue.size();
}

int Toast::getDuration()
{
	return m_duration;
}

bool Toast::isShowDurationBar()
{
	return m_showDurationBar;
}

QString Toast::getTitle()
{
	return m_title;
}

QString Toast::getText()
{
	return m_text;
}

QPixmap Toast::getIcon()
{
	return m_icon;
}

bool Toast::isShowIcon()
{
	return m_showIcon;
}

QSize Toast::getIconSize()
{
	return m_iconSize;
}

bool Toast::isShowIconSeparator()
{
	return m_showIconSeparator;
}

int Toast::getIconSeparatorWidth()
{
	return m_iconSeparatorWidth;
}

QPixmap Toast::getCloseButtonIcon()
{
	return m_closeButtonIcon;
}

bool Toast::isShowCloseButton()
{
	return m_showCloseButton;
}

QSize Toast::getCloseButtonIconSize()
{
	return m_closeButtonIconSize;
}

int Toast::getCloseButtonWidth()
{
	return m_closeButtonSize.width();
}

int Toast::getCloseButtonHeight()
{
	return m_closeButtonSize.height();
}

QSize Toast::getCloseButtonSize()
{
	return m_closeButtonSize;
}

Toast::ButtonAlignment Toast::getCloseButtonAlignment()
{
	return m_closeButtonAlignment;
}

int Toast::getFadeInDuration()
{
	return m_fadeInDuration;
}

int Toast::getFadeOutDuration()
{
	return m_fadeOutDuration;
}

bool Toast::isResetDurationOnHover()
{
	return m_resetDurationOnHover;
}

bool Toast::isStayOnTop()
{
	return m_stayOnTop;
}

int Toast::getBorderRadius()
{
	return m_borderRadius;
}

QColor Toast::getBackgroundColor()
{
	return m_backgroundColor;
}

QColor Toast::getTitleColor()
{
	return m_titleColor;
}

QColor Toast::getTextColor()
{
	return m_textColor;
}

QColor Toast::getIconColor()
{
	return m_iconColor;
}

QColor Toast::getIconSeparatorColor()
{
	return m_iconSeparatorColor;
}

QColor Toast::getCloseButtonIconColor()
{
	return m_closeButtonIconColor;
}

QColor Toast::getDurationBarColor()
{
	return m_durationBarColor;
}

QFont Toast::getTitleFont()
{
	return m_titleFont;
}

QFont Toast::getTextFont()
{
	return m_textFont;
}

QMargins Toast::getMargins()
{
	return m_margins;
}

QMargins Toast::getIconMargins()
{
	return m_iconMargins;
}

QMargins Toast::getIconSectionMargins()
{
	return m_iconSectionMargins;
}

QMargins Toast::getTextSectionMargins()
{
	return m_textSectionMargins;
}

QMargins Toast::getCloseButtonMargins()
{
	return m_closeButtonMargins;
}

int Toast::getTextSectionSpacing()
{
	return m_textSectionSpacing;
}

void Toast::setMaximumOnScreen(int maximum)
{
	int freedSpaces = maximum - s_maximumOnScreen;
	s_maximumOnScreen = maximum;

	if (freedSpaces > 0)
	{
		for (int i = 0; i < freedSpaces; i++)
		{
			showNextInQueue();
		}
	}
}

void Toast::setSpacing(int spacing)
{
	s_spacing = spacing;
	updateCurrentlyShowingPositionY();
}

void Toast::setOffset(int x, int y)
{
	s_offsetX = x;
	s_offsetY = y;
	updateCurrentlyShowingPositionXY();
}

void Toast::setOffsetX(int offsetX)
{
	s_offsetX = offsetX;
	updateCurrentlyShowingPositionX();
}

void Toast::setOffsetY(int offsetY)
{
	s_offsetY = offsetY;
	updateCurrentlyShowingPositionY();
}

void Toast::setAlwaysOnMainScreen(bool enabled)
{
	s_alwaysOnMainScreen = enabled;
	updateCurrentlyShowingPositionXY();
}

void Toast::setFixedScreen(QScreen* screen)
{
	s_fixedScreen = screen;
	updateCurrentlyShowingPositionXY();
}

void Toast::setPosition(Position position)
{
	s_position = position;
	updateCurrentlyShowingPositionXY();
}

void Toast::reset()
{
	// Reset static attributes
	s_maximumOnScreen = 3;
	s_spacing = 10;
	s_offsetX = 20;
	s_offsetY = 45;
	s_alwaysOnMainScreen = false;
	s_fixedScreen = nullptr;
	s_position = Position::BottomRight;

	// Hide currently showing toasts and clear queue
	for (Toast* toast : s_currentlyShown)
	{
		toast->setVisible(false);
		toast->deleteLater();
	}

	s_currentlyShown.clear();
	s_queue.clear();
}

void Toast::setDuration(int duration)
{
	if (m_used)
	{
		return;
	}
	m_duration = duration;
}

void Toast::setShowDurationBar(bool enabled)
{
	if (m_used)
	{
		return;
	}
	m_showDurationBar = enabled;
}

void Toast::setTitle(QString title)
{
	if (m_used)
	{
		return;
	}
	m_title = title;
	m_titleLabel->setText(title);
}

void Toast::setText(QString text)
{
	if (m_used)
	{
		return;
	}
	m_text = text;
	m_textLabel->setText(text);
}

void Toast::setIcon(QPixmap icon)
{
	if (m_used)
	{
		return;
	}
	m_icon = icon;
	m_iconWidget->setIcon(QIcon(icon));
	setIconColor(m_iconColor);
}

void Toast::setIcon(Icon icon)
{
	if (m_used)
	{
		return;
	}
	m_icon = getIconFromEnum(icon);
	m_iconWidget->setIcon(QIcon(m_icon));
	setIconColor(m_iconColor);
}

void Toast::setShowIcon(bool enabled)
{
	if (m_used)
	{
		return;
	}
	m_showIcon = enabled;
}

void Toast::setIconSize(QSize size)
{
	if (m_used)
	{
		return;
	}
	m_iconSize = size;
	m_iconWidget->setFixedSize(size);
	m_iconWidget->setIconSize(size);
	setIcon(m_icon);
}

void Toast::setShowIconSeparator(bool enabled)
{
	if (m_used)
	{
		return;
	}
	m_showIconSeparator = enabled;

	if (enabled)
	{
		m_iconSeparator->setFixedWidth(m_iconSeparatorWidth);
	}
	else {
		m_iconSeparator->setFixedWidth(0);
	}
}

void Toast::setIconSeparatorWidth(int width)
{
	if (m_used)
	{
		return;
	}
	m_iconSeparatorWidth = width;

	if (m_showIconSeparator)
	{
		m_iconSeparator->setFixedWidth(width);
	}
}

void Toast::setCloseButtonIcon(QPixmap icon)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonIcon = icon;
	m_closeButton->setIcon(QIcon(icon));
	setCloseButtonIconColor(m_closeButtonIconColor);
}

void Toast::setCloseButtonIcon(Icon icon)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonIcon = getIconFromEnum(icon);
	m_closeButton->setIcon(QIcon(m_closeButtonIcon));
	setCloseButtonIconColor(m_closeButtonIconColor);
}

void Toast::setShowCloseButton(bool enabled)
{
	if (m_used)
	{
		return;
	}
	m_showCloseButton = enabled;
}

void Toast::setCloseButtonIconSize(QSize size)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonIconSize = size;
	m_closeButton->setIconSize(size);
	setCloseButtonIcon(m_closeButtonIcon);
}

void Toast::setCloseButtonSize(QSize size)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonSize = size;
	m_closeButton->setFixedSize(size);
}

void Toast::setCloseButtonWidth(int width)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonSize.setWidth(width);
	m_closeButton->setFixedSize(m_closeButtonSize);
}

void Toast::setCloseButtonHeight(int height)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonSize.setHeight(height);
	m_closeButton->setFixedSize(m_closeButtonSize);
}

void Toast::setCloseButtonAlignment(ButtonAlignment alignment)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonAlignment = alignment;
}

void Toast::setFadeInDuration(int duration)
{
	if (m_used)
	{
		return;
	}
	m_fadeInDuration = duration;
}

void Toast::setFadeOutDuration(int duration)
{
	if (m_used)
	{
		return;
	}
	m_fadeOutDuration = duration;
}

void Toast::setResetDurationOnHover(bool enabled)
{
	if (m_used)
	{
		return;
	}
	m_resetDurationOnHover = enabled;
}

void Toast::setStayOnTop(bool enabled)
{
	if (m_used)
	{
		return;
	}
	m_stayOnTop = enabled;

	if (enabled)
	{
		setWindowFlags(Qt::WindowType::Tool
					   | Qt::WindowType::CustomizeWindowHint
					   | Qt::WindowType::FramelessWindowHint
					   | Qt::WindowType::WindowStaysOnTopHint);
	}
	else
	{
		setWindowFlags(Qt::WindowType::Tool
					   | Qt::WindowType::CustomizeWindowHint
					   | Qt::WindowType::FramelessWindowHint);
	}
}

void Toast::setBorderRadius(int borderRadius)
{
	if (m_used)
	{
		return;
	}
	m_borderRadius = borderRadius;
}

void Toast::setBackgroundColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_backgroundColor = color;
}

void Toast::setTitleColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_titleColor = color;
}

void Toast::setTextColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_textColor = color;
}

void Toast::setIconColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_iconColor = color;

	QImage recoloredImage = recolorImage(m_iconWidget->icon()
										 .pixmap(m_iconWidget->iconSize()).toImage(), color);
	m_iconWidget->setIcon(QIcon(QPixmap::fromImage(recoloredImage)));
}

void Toast::setIconSeparatorColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_iconSeparatorColor = color;
}

void Toast::setCloseButtonIconColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonIconColor = color;

	QImage recoloredImage = recolorImage(m_closeButton->icon()
										 .pixmap(m_closeButton->iconSize()).toImage(), color);
	m_closeButton->setIcon(QIcon(QPixmap::fromImage(recoloredImage)));
}

void Toast::setDurationBarColor(QColor color)
{
	if (m_used)
	{
		return;
	}
	m_durationBarColor = color;
}

void Toast::setTitleFont(QFont font)
{
	if (m_used)
	{
		return;
	}
	m_titleFont = font;
	m_titleLabel->setFont(font);
}

void Toast::setTextFont(QFont font)
{
	if (m_used)
	{
		return;
	}
	m_textFont = font;
	m_textLabel->setFont(font);
}

void Toast::setMargins(QMargins margins)
{
	if (m_used)
	{
		return;
	}
	m_margins = margins;
}

void Toast::setMarginLeft(int margin)
{
	if (m_used)
	{
		return;
	}
	m_margins.setLeft(margin);
}

void Toast::setMarginTop(int margin)
{
	if (m_used)
	{
		return;
	}
	m_margins.setTop(margin);
}

void Toast::setMarginRight(int margin)
{
	if (m_used)
	{
		return;
	}
	m_margins.setRight(margin);
}


void Toast::setMarginBottom(int margin)
{
	if (m_used)
	{
		return;
	}
	m_margins.setBottom(margin);
}


void Toast::setIconMargins(QMargins margins)
{
	if (m_used)
	{
		return;
	}
	m_iconMargins = margins;
}


void Toast::setIconMarginLeft(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconMargins.setLeft(margin);
}


void Toast::setIconMarginTop(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconMargins.setTop(margin);
}


void Toast::setIconMarginRight(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconMargins.setRight(margin);
}


void Toast::setIconMarginBottom(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconMargins.setBottom(margin);
}


void Toast::setIconSectionMargins(QMargins margins)
{
	if (m_used)
	{
		return;
	}
	m_iconSectionMargins = margins;
}


void Toast::setIconSectionMarginLeft(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconSectionMargins.setLeft(margin);
}


void Toast::setIconSectionMarginTop(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconSectionMargins.setTop(margin);
}


void Toast::setIconSectionMarginRight(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconSectionMargins.setRight(margin);
}


void Toast::setIconSectionMarginBottom(int margin)
{
	if (m_used)
	{
		return;
	}
	m_iconSectionMargins.setBottom(margin);
}


void Toast::setTextSectionMargins(QMargins margins)
{
	if (m_used)
	{
		return;
	}
	m_textSectionMargins = margins;
}


void Toast::setTextSectionMarginLeft(int margin)
{
	if (m_used)
	{
		return;
	}
	m_textSectionMargins.setLeft(margin);
}


void Toast::setTextSectionMarginTop(int margin)
{
	if (m_used)
	{
		return;
	}
	m_textSectionMargins.setTop(margin);
}


void Toast::setTextSectionMarginRight(int margin)
{
	if (m_used)
	{
		return;
	}
	m_textSectionMargins.setRight(margin);
}


void Toast::setTextSectionMarginBottom(int margin)
{
	if (m_used)
	{
		return;
	}
	m_textSectionMargins.setBottom(margin);
}


void Toast::setCloseButtonMargins(QMargins margins)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonMargins = margins;
}


void Toast::setCloseButtonMarginLeft(int margin)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonMargins.setLeft(margin);
}


void Toast::setCloseButtonMarginTop(int margin)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonMargins.setTop(margin);
}


void Toast::setCloseButtonMarginRight(int margin)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonMargins.setRight(margin);
}


void Toast::setCloseButtonMarginBottom(int margin)
{
	if (m_used)
	{
		return;
	}
	m_closeButtonMargins.setBottom(margin);
}


void Toast::setTextSectionSpacing(int spacing)
{
	if (m_used)
	{
		return;
	}
	m_textSectionSpacing = spacing;
}


void Toast::setFixedSize(QSize size)
{
	setMinimumSize(size);
	setMaximumSize(size);
}


void Toast::setFixedSize(int width, int height)
{
	setMinimumSize(width, height);
	setMaximumSize(width, height);
}


void Toast::setFixedWidth(int width)
{
	setMinimumWidth(width);
	setMaximumWidth(width);
}

void Toast::setFixedHeight(int height)
{
	setMinimumHeight(height);
	setMaximumHeight(height);
}

void Toast::applyPreset(Preset preset)
{
	if (m_used)
	{
		return;
	}

	if (preset == Preset::Success)
	{
		setIcon(Icon::Success);
		setIconColor(sc_successAccentColor);
		setDurationBarColor(sc_successAccentColor);
	}
	else if (preset == Preset::Warning)
	{
		setIcon(Icon::Warning);
		setIconColor(sc_warningAccentColor);
		setDurationBarColor(sc_warningAccentColor);
	}
	else if (preset == Preset::Error)
	{
		setIcon(Icon::Error);
		setIconColor(sc_errorAccentColor);
		setDurationBarColor(sc_errorAccentColor);
	}
	else if (preset == Preset::Information)
	{
		setIcon(Icon::Information);
		setIconColor(sc_informationAccentColor);
		setDurationBarColor(sc_informationAccentColor);
	}

	if (
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
		(QGuiApplication::styleHints() &&
		 QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) ||
#endif
		VeyonCore::useDarkMode())
	{
		setBackgroundColor(sc_defaultBackgroundColorDark);
		setCloseButtonIconColor(sc_defaultCloseButtonIconColorDark);
		setIconSeparatorColor(sc_defaultIconSeparatorColorDark);
		setTitleColor(sc_defaultTitleColorDark);
		setTextColor(sc_defaultTextColorDark);
	}
	else
	{
		setBackgroundColor(sc_defaultBackgroundColor);
		setCloseButtonIconColor(sc_defaultCloseButtonIconColor);
		setIconSeparatorColor(sc_defaultIconSeparatorColor);
		setTitleColor(sc_defaultTitleColor);
		setTextColor(sc_defaultTextColor);
	}

	setShowDurationBar(true);
	setShowIcon(true);
	setShowIconSeparator(true);
	setIconSeparatorWidth(2);
}


void Toast::show()
{
	// Check if already used
	if (m_used)
	{
		return;
	}

	// If max notifications on screen not reached, show notification
	if (s_maximumOnScreen > int(s_currentlyShown.size()))
	{
		m_used = true;
		s_currentlyShown.push_back(this);

		// Setup UI
		setupUI();

		// Start duration timer
		if (m_duration != 0)
		{
			m_durationTimer->start(m_duration);
		}

		// Start duration bar update timer
		if (m_duration != 0 && m_showDurationBar)
		{
			m_durationBarTimer->start(sc_durationBarUpdateInterval);
		}

		// Calculate position and show
		QPoint position = calculatePosition();

		// If not first toast on screen, also do a fade down / up animation
		if (s_currentlyShown.size() > 1)
		{
			// Calculate offset if predecessor toast is still in fade down / up animation
			Toast* predecessorToast = getPredecessorToast();
			QPoint predecessorTarget = predecessorToast->calculatePosition();
			int predecessorTargetDifferenceY = abs(predecessorToast->y() - predecessorTarget.y());

			// Calculate start position of fade down / up animation based on position
			if (s_position == Position::BottomRight
				|| s_position == Position::BottomLeft
				|| s_position == Position::BottomMiddle)
			{
				move(position.x(),
					 position.y() - (int)(height() / 1.5) - predecessorTargetDifferenceY);
			}
			else if (s_position == Position::TopRight
					 || s_position == Position::TopLeft
					 || s_position == Position::TopMiddle
					 || s_position == Position::Center)
			{
				move(position.x(),
					 position.y() + (int)(height() / 1.5) + predecessorTargetDifferenceY);
			}

			// Start fade down / up animation
			QPropertyAnimation* posAnimation = new QPropertyAnimation(this, "pos");
			posAnimation->setEndValue(QPoint(position.x(), position.y()));
			posAnimation->setDuration(m_fadeInDuration);
			posAnimation->start(QAbstractAnimation::DeleteWhenStopped);
		}
		else
		{
			move(position.x(), position.y());
		}

		// Fade in
		QDialog::show();
		QPropertyAnimation* fadeInAnimation = new QPropertyAnimation(m_opacityEffect, "opacity");
		fadeInAnimation->setDuration(m_fadeInDuration);
		fadeInAnimation->setStartValue(0);
		fadeInAnimation->setEndValue(1);
		fadeInAnimation->start(QAbstractAnimation::DeleteWhenStopped);

		// Make sure title bar of parent is not grayed out
		if (m_parent)
		{
			m_parent->activateWindow();
		}

		// Update every other currently shown notification
		for (Toast* toast : s_currentlyShown)
		{
			toast->updatePositionXY();
		}
	}
	else
	{
		// Add notification to queue instead
		s_queue.push_back(this);
	}
}

void Toast::hide()
{
	if (!m_fadingOut)
	{
		if (m_duration != 0)
		{
			m_durationTimer->stop();
		}
		fadeOut();
	}
}

void Toast::hide_()
{
	close();

	int index = 0;
	for (Toast* toast : s_currentlyShown)
	{
		if (toast == this)
		{
			s_currentlyShown.erase(s_currentlyShown.begin() + index);
			m_elapsedTime = 0;
			m_fadingOut = false;

			// Emit signal
			Q_EMIT closed();

			// Update every other currently shown notification
			for (Toast* toast : s_currentlyShown)
			{
				toast->updatePositionY();
			}

			// Show next item from queue after updating
			QTimer::singleShot(m_fadeInDuration, this, &Toast::deleteAndShowNextInQueue);

			break;
		};
		index++;
	}
}

void Toast::enterEvent(EnterEvent* event)
{
	Q_UNUSED(event)

	// Reset timer if hovered and resetting is enabled
	if (m_duration != 0 && m_durationTimer->isActive() && m_resetDurationOnHover)
	{
		m_durationTimer->stop();

		// Reset duration bar if enabled
		if (m_showDurationBar)
		{
			m_durationBarTimer->stop();
			m_durationBarChunk->setFixedWidth(m_notification->width());
			m_elapsedTime = 0;
		}
	}
}

void Toast::leaveEvent(QEvent* event)
{
	Q_UNUSED(event)

	// Start timer again when leaving notification and reset is enabled
	if (m_duration != 0 && !m_durationTimer->isActive() && m_resetDurationOnHover)
	{
		m_durationTimer->start(m_duration);

		// Restart duration bar animation if enabled
		if (m_showDurationBar)
		{
			m_durationBarTimer->start(sc_durationBarUpdateInterval);
		}
	}
}

void Toast::fadeOut()
{
	m_fadingOut = true;
	QPropertyAnimation* fadeOutAnimation = new QPropertyAnimation(m_opacityEffect, "opacity");
	fadeOutAnimation->setDuration(m_fadeOutDuration);
	fadeOutAnimation->setStartValue(1);
	fadeOutAnimation->setEndValue(0);
	connect(fadeOutAnimation, &QPropertyAnimation::finished, this, &Toast::hide_);
	fadeOutAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Toast::updateDurationBar()
{
	m_elapsedTime += sc_durationBarUpdateInterval;

	if (m_elapsedTime >= m_duration)
	{
		m_durationBarTimer->stop();
		return;
	}
	double newChunkWidth = floor(m_durationBarContainer->width()
								 - (double)m_elapsedTime / (double)m_duration * m_durationBarContainer->width());
	m_durationBarChunk->setFixedWidth(newChunkWidth);
}

void Toast::showNextInQueue()
{
	if (s_queue.size() > 0)
	{
		Toast* nextToast = s_queue.front();
		s_queue.pop_front();
		nextToast->show();
	}
}

void Toast::setupUI()
{
	// Update stylesheet
	updateStylesheet();

	// Calculate title and text width and height
	QFontMetrics* titleFontMetrics = new QFontMetrics(m_titleFont);
	int titleWidth = titleFontMetrics->boundingRect(m_title).width() + 1;
	int titleHeight = titleFontMetrics->boundingRect(m_title).height();
	delete titleFontMetrics;

	QFontMetrics* textFontMetrics = new QFontMetrics(m_textFont);
	int textWidth = textFontMetrics->boundingRect(m_text).width() + 1;
	int textHeight = textFontMetrics->boundingRect(m_text).height();
	delete textFontMetrics;

	int textSectionSpacing = m_title.isEmpty() || m_text.isEmpty() ? 0 : m_textSectionSpacing;

	int textSectionHeight = m_textSectionMargins.top() + titleHeight
							+ textSectionSpacing + textHeight + m_textSectionMargins.bottom();

	// Calculate duration bar height
	int durationBarHeight = m_showDurationBar ? m_durationBarContainer->height() : 0;

	// Calculate icon section width and height
	int iconSectionWidth = 0;
	int iconSectionHeight = 0;

	if (m_showIcon)
	{
		iconSectionWidth = m_iconSectionMargins.left() + m_iconMargins.left()
						   + m_iconWidget->width() + m_iconMargins.right()
						   + m_iconSeparator->width() + m_iconSectionMargins.right();
		iconSectionHeight = m_iconSectionMargins.top() + m_iconMargins.top()
							+ m_iconWidget->height() + m_iconMargins.bottom() + m_iconSectionMargins.bottom();
	}

	// Calculate close button section size
	int closeButtonWidth = m_showCloseButton ? m_closeButton->width() : 0;
	int closeButtonHeight = m_showCloseButton ? m_closeButton->height() : 0;
	QMargins closeButtonMargins = m_showCloseButton ? m_closeButtonMargins : QMargins(0, 0, 0, 0);

	int closeButtonSectionHeight = m_closeButtonMargins.top()
								   + closeButtonHeight + m_closeButtonMargins.bottom();

	// Calculate needed width and height
	int width = m_margins.left() + iconSectionWidth + m_textSectionMargins.left()
				+ std::max(titleWidth, textWidth) + m_textSectionMargins.right()
				+ closeButtonMargins.left() + closeButtonWidth
				+ closeButtonMargins.right() + m_margins.right();

	int height = m_margins.top() + std::max({iconSectionHeight, textSectionHeight,
											 closeButtonSectionHeight}) + m_margins.bottom() + durationBarHeight;

	int forcedAdditionalHeight = 0;
	int forcedReducedHeight = 0;

	// Handle width greater than maximum width
	if (width > maximumWidth())
	{
		// Enable line break for title and text and recalculate size
		int newTitleWidth = std::max(titleWidth, textWidth) - (width - maximumWidth());
		if (newTitleWidth > 0)
		{
			titleWidth = newTitleWidth;
		}

		int newTextWidth = std::max(titleWidth, textWidth) - (width - maximumWidth());
		if (newTextWidth > 0)
		{
			textWidth = newTextWidth;
		}

		m_titleLabel->setMinimumWidth(titleWidth);
		m_titleLabel->setWordWrap(true);
		if (m_title.isEmpty() == false)
		{
			titleHeight = m_titleLabel->sizeHint().height();
		}
		m_titleLabel->setFixedSize(titleWidth, titleHeight);

		m_textLabel->setMinimumWidth(textWidth);
		m_textLabel->setWordWrap(true);
		if (m_text.isEmpty() == false)
		{
			textHeight = m_textLabel->sizeHint().height();
		}
		m_textLabel->setFixedSize(textWidth, textHeight);

		// Recalculate width and height
		width = maximumWidth();

		textSectionHeight = m_textSectionMargins.top() + titleHeight
							+ textSectionSpacing + textHeight + m_textSectionMargins.bottom();

		height = m_margins.top() + std::max({iconSectionHeight, textSectionHeight,
											 closeButtonSectionHeight}) + m_margins.bottom() + durationBarHeight;
	}

	// Handle height less than minimum height
	if (height < minimumHeight())
	{
		// Enable word wrap for title and text labels
		m_titleLabel->setWordWrap(true);
		m_textLabel->setWordWrap(true);

		// Calculate height with initial label width
		titleWidth = m_titleLabel->fontMetrics().boundingRect(QRect(0, 0, 0, 0),
															  Qt::TextFlag::TextWordWrap, m_titleLabel->text()).width();

		textWidth = m_textLabel->fontMetrics().boundingRect(QRect(0, 0, 0, 0),
															Qt::TextFlag::TextWordWrap, m_textLabel->text()).width();

		int tempWidth = std::max(titleWidth, textWidth);

		titleWidth = m_titleLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
															  Qt::TextFlag::TextWordWrap, m_titleLabel->text()).width();
		if (m_title.isEmpty() == false)
		{
			titleHeight = m_titleLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
																   Qt::TextFlag::TextWordWrap, m_titleLabel->text()).height();
		}

		textWidth = m_textLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
															Qt::TextFlag::TextWordWrap, m_textLabel->text()).width();
		if (m_text.isEmpty() == false)
		{
			textHeight = m_textLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
																 Qt::TextFlag::TextWordWrap, m_textLabel->text()).height();
		}

		textSectionHeight = m_textSectionMargins.top() + titleHeight
							+ textSectionSpacing + textHeight + m_textSectionMargins.bottom();

		height = m_margins.top() + std::max({ iconSectionHeight, textSectionHeight,
											  closeButtonSectionHeight }) + m_margins.bottom() + durationBarHeight;

		while (tempWidth <= width)
		{
			// Recalculate height with different text widths to find optimal value
			int tempTitleWidth = m_titleLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
																		  Qt::TextFlag::TextWordWrap, m_titleLabel->text()).width();

			int tempTitleHeight = m_titleLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
																		   Qt::TextFlag::TextWordWrap, m_titleLabel->text()).height();

			int tempTextWidth = m_textLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
																		Qt::TextFlag::TextWordWrap, m_textLabel->text()).width();

			int tempTextHeight = m_textLabel->fontMetrics().boundingRect(QRect(0, 0, tempWidth, 0),
																		 Qt::TextFlag::TextWordWrap, m_textLabel->text()).height();

			if (m_title.isEmpty())
			{
				tempTitleHeight = 0;
			}

			if (m_text.isEmpty())
			{
				tempTextHeight = 0;
			}

			int tempTextSectionHeight = m_textSectionMargins.top() + tempTitleHeight
										+ textSectionSpacing + tempTextHeight + m_textSectionMargins.bottom();

			int tempHeight = m_margins.top() + std::max({ iconSectionHeight, tempTextSectionHeight,
														  closeButtonSectionHeight }) + m_margins.bottom() + durationBarHeight;

			// Store values if calculated height is greater than or equal to min height
			if (tempHeight >= minimumHeight())
			{
				titleWidth = tempTitleWidth;
				titleHeight = tempTitleHeight;
				textWidth = tempTextWidth;
				textHeight = tempTextHeight;
				textSectionHeight = tempTextSectionHeight;
				height = tempHeight;
				tempWidth += 1;
			}
			else
			{
				// Exit loop if calculated height is less than min height
				break;
			}
		}

		// Recalculate width
		width = m_margins.left() + iconSectionWidth + m_textSectionMargins.left()
				+ std::max(titleWidth, textWidth) + m_textSectionMargins.right()
				+ closeButtonMargins.left() + closeButtonWidth
				+ closeButtonMargins.right() + m_margins.right();

		// If min height not met, set height to min height
		if (height < minimumHeight())
		{
			forcedAdditionalHeight = minimumHeight() - height;
			height = minimumHeight();
		}
	}

	// Handle width less than minimum width
	if (width < minimumWidth())
	{
		width = minimumWidth();
	}

	// Handle height greater than maximum height
	if (height > maximumHeight())
	{
		forcedReducedHeight = height - maximumHeight();
		height = maximumHeight();
	}

	// Calculate width and height including space for drop shadow
	int totalWidth = width + sc_dropShadowSize * 2;
	int totalHeight = height + sc_dropShadowSize * 2;

	// Resize drop shadow
	m_dropShadowLayer1->resize(totalWidth, totalHeight);
	m_dropShadowLayer1->move(0, 0);
	m_dropShadowLayer2->resize(totalWidth - 2, totalHeight - 2);
	m_dropShadowLayer2->move(1, 1);
	m_dropShadowLayer3->resize(totalWidth - 4, totalHeight - 4);
	m_dropShadowLayer3->move(2, 2);
	m_dropShadowLayer4->resize(totalWidth - 6, totalHeight - 6);
	m_dropShadowLayer4->move(3, 3);
	m_dropShadowLayer5->resize(totalWidth - 8, totalHeight - 8);
	m_dropShadowLayer5->move(4, 4);

	// Resize window
	QDialog::setFixedSize(totalWidth, totalHeight);
	m_notification->setFixedSize(width, height);
	m_notification->move(sc_dropShadowSize, sc_dropShadowSize);
	m_notification->raise();

	// Calculate difference between height and height of icon section
	int heightIconSectionHeightDifference = std::max({ iconSectionHeight,
													   textSectionHeight, closeButtonSectionHeight }) - iconSectionHeight;

	if (m_showIcon)
	{
		// Move icon
		m_iconWidget->move(m_margins.left() + m_iconSectionMargins.left()+ m_iconMargins.left(),
						   m_margins.top() + m_iconSectionMargins.top() + m_iconMargins.top()
						   + ceil(heightIconSectionHeightDifference / 2) - floor(forcedReducedHeight / 2));

		// Move and resize icon separator
		m_iconSeparator->setFixedHeight(textSectionHeight);
		m_iconSeparator->move(m_margins.left() + m_iconSectionMargins.left() + m_iconMargins.left()
							  + m_iconWidget->width() + m_iconMargins.right(), m_margins.top() + m_iconSectionMargins.top()
							  + ceil(forcedAdditionalHeight / 2) - floor(forcedReducedHeight / 2));
	}
	else
	{
		// Hide icon section
		m_iconWidget->setVisible(false);
		m_iconSeparator->setVisible(false);
	}

	// Calculate difference between height and height of text section
	int heightTextSectionHeightDifference = std::max({iconSectionHeight,
													  textSectionHeight, closeButtonSectionHeight}) - textSectionHeight;

	// Resize title and text labels
	m_titleLabel->setFixedSize(std::max(titleWidth, textWidth), titleHeight);
	m_textLabel->setFixedSize(std::max(titleWidth, textWidth), textHeight);

	// Move title and text labels
	if (m_showIcon)
	{
		m_titleLabel->move(m_margins.left() + m_iconSectionMargins.left()
						   + m_iconMargins.left() + m_iconWidget->width() + m_iconMargins.right()
						   + m_iconSeparator->width() + m_iconSectionMargins.right() + m_textSectionMargins.left(),
						   m_margins.top() + m_textSectionMargins.top() + ceil(heightTextSectionHeightDifference / 2)
						   + ceil(forcedAdditionalHeight / 2) - floor(forcedReducedHeight / 2));

		m_textLabel->move(m_margins.left() + m_iconSectionMargins.left()
						  + m_iconMargins.left() + m_iconWidget->width() + m_iconMargins.right()
						  + m_iconSeparator->width() + m_iconSectionMargins.right() + m_textSectionMargins.left(),
						  m_margins.top() + m_textSectionMargins.top() + titleHeight
						  + textSectionSpacing + ceil(heightTextSectionHeightDifference / 2)
						  + ceil(forcedAdditionalHeight / 2) - floor(forcedReducedHeight / 2));
	}
	else {
		// Position is different if icon hidden
		m_titleLabel->move(m_margins.left() + m_textSectionMargins.left(),
						   m_margins.top() + m_textSectionMargins.top() + ceil(heightTextSectionHeightDifference / 2)
						   + ceil(forcedAdditionalHeight / 2) - floor(forcedReducedHeight / 2));

		m_textLabel->move(m_margins.left() + m_textSectionMargins.left(),
						  m_margins.top() + m_textSectionMargins.top() + titleHeight
						  + textSectionSpacing + ceil(heightTextSectionHeightDifference / 2)
						  + ceil(forcedAdditionalHeight / 2) - floor(forcedReducedHeight / 2));
	}

	// Adjust label position if either title or text is empty
	if (m_title.isEmpty() && m_text.isEmpty() == false)
	{
		m_textLabel->move(m_textLabel->x(),
						  (int)((height - textHeight - durationBarHeight) / 2));
	}
	else if (m_title.isEmpty() == false && m_text.isEmpty())
	{
		m_titleLabel->move(m_titleLabel->x(),
						   (int)((height - titleHeight - durationBarHeight) / 2));
	}

	// Move close button to top, middle, or bottom position
	if (m_closeButtonAlignment == ButtonAlignment::Top)
	{
		m_closeButton->move(width - closeButtonWidth - closeButtonMargins.right()
							- m_margins.right(), m_margins.top() + closeButtonMargins.top());
	}
	else if (m_closeButtonAlignment == ButtonAlignment::Middle)
	{
		m_closeButton->move(width - closeButtonWidth - closeButtonMargins.right()
							- m_margins.right(), ceil((height - closeButtonHeight - durationBarHeight) / 2));
	}
	else if (m_closeButtonAlignment == ButtonAlignment::Bottom)
	{
		m_closeButton->move(width - closeButtonWidth - closeButtonMargins.right()
							- m_margins.right(), height - closeButtonHeight - m_margins.bottom()
							- closeButtonMargins.bottom() - durationBarHeight);
	}

	// Hide close button if disabled
	if (!m_showCloseButton)
	{
		m_closeButton->setVisible(false);
	}

	// Resize, move, and show duration bar if enabled
	if (m_showDurationBar)
	{
		m_durationBarContainer->setFixedWidth(width);
		m_durationBarContainer->move(0, height - durationBarHeight);
		m_durationBar->setFixedWidth(width);
		m_durationBarChunk->setFixedWidth(width);
	}
	else {
		m_durationBarContainer->setVisible(false);
	}
}

QPoint Toast::calculatePosition()
{
	// Calculate vertical space taken up by all the currently showing notifications
	int offsetY = 0;

	for (Toast* toast : s_currentlyShown)
	{
		if (toast == this)
		{
			break;
		}
		offsetY += toast->m_notification->height() + s_spacing;
	}

	// Get screen
	QScreen* primaryScreen = QGuiApplication::primaryScreen();
	QScreen* currentScreen = nullptr;

	if (s_fixedScreen)
	{
		currentScreen = s_fixedScreen;
	}
	else if (s_alwaysOnMainScreen || !m_parent)
	{
		currentScreen = primaryScreen;
	}
	else
	{
		QList<QScreen*> screens = QGuiApplication::screens();

		for (QScreen* screen : screens)
		{
			if (m_parent->geometry().intersects(screen->geometry()))
			{
				if (!currentScreen)
				{
					currentScreen = screen;
				}
				else
				{
					currentScreen = primaryScreen;
					break;
				}
			}
		}
	}

	// Calculate x and y position of notification
	int x = 0;
	int y = 0;

	if (s_position == Position::BottomRight)
	{
		x = currentScreen->geometry().width() - m_notification->width()
			- s_offsetX + currentScreen->geometry().x();
		y = currentScreen->geometry().height() - m_notification->height()
			- s_offsetY + currentScreen->geometry().y() - offsetY;
	}
	else if (s_position == Position::BottomLeft)
	{
		x = currentScreen->geometry().x() + s_offsetX;
		y = currentScreen->geometry().height() - m_notification->height()
			- s_offsetY + currentScreen->geometry().y() - offsetY;
	}
	else if (s_position == Position::BottomMiddle)
	{
		x = (int)(currentScreen->geometry().x()
				  + currentScreen->geometry().width() / 2 - m_notification->width() / 2);
		y = currentScreen->geometry().height() - m_notification->height()
			- s_offsetY + currentScreen->geometry().y() - offsetY;
	}
	else if (s_position == Position::TopRight)
	{
		x = currentScreen->geometry().width() - m_notification->width()
			- s_offsetX + currentScreen->geometry().x();
		y = currentScreen->geometry().y() + s_offsetY + offsetY;
	}
	else if (s_position == Position::TopLeft)
	{
		x = currentScreen->geometry().x() + s_offsetX;
		y = currentScreen->geometry().y() + s_offsetY + offsetY;
	}
	else if (s_position == Position::TopMiddle)
	{
		x = (int)(currentScreen->geometry().x()
				  + currentScreen->geometry().width() / 2 - m_notification->width() / 2);
		y = currentScreen->geometry().y() + s_offsetY + offsetY;
	}
	else if (s_position == Position::Center)
	{
		x = (int)(currentScreen->geometry().x()
				  + currentScreen->geometry().width() / 2 - m_notification->width() / 2);
		y = (int)(currentScreen->geometry().y() + currentScreen->geometry().height() / 2
				  - m_notification->height() / 2 + offsetY);
	}

	x -= sc_dropShadowSize;
	y -= sc_dropShadowSize;

	return QPoint(x, y);
}

void Toast::updatePositionXY()
{
	QPoint position = calculatePosition();

	// Animate position change
	QPropertyAnimation* posAnimation = new QPropertyAnimation(this, "pos");
	posAnimation->setEndValue(position);
	posAnimation->setDuration(sc_updatePositionDuration);
	posAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Toast::updatePositionX()
{
	QPoint position = calculatePosition();

	// Animate position change
	QPropertyAnimation* posAnimation = new QPropertyAnimation(this, "pos");
	posAnimation->setEndValue(QPoint(position.x(), y()));
	posAnimation->setDuration(sc_updatePositionDuration);
	posAnimation->start();
}

void Toast::updatePositionY()
{
	QPoint position = calculatePosition();

	// Animate position change
	QPropertyAnimation* posAnimation = new QPropertyAnimation(this, "pos");
	posAnimation->setEndValue(QPoint(x(), position.y()));
	posAnimation->setDuration(sc_updatePositionDuration);
	posAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Toast::updateStylesheet()
{
	m_notification->setStyleSheet(QStringLiteral("background: %1;"
												 "border-radius: %2px; ").arg(m_backgroundColor.name()).arg(m_borderRadius));

	m_durationBar->setStyleSheet(QStringLiteral("background: rgba(%1, %2, %3, 100);"
												"border-radius: %4px;").arg(m_durationBarColor.red())
								 .arg(m_durationBarColor.green()).arg(m_durationBarColor.blue()).arg(m_borderRadius));

	m_durationBarChunk->setStyleSheet(QStringLiteral("background: rgba(%1, %2, %3, 255);"
													 "border-bottom-left-radius: %4px; border-bottom-right-radius: %5px;")
									  .arg(m_durationBarColor.red()).arg(m_durationBarColor.green())
									  .arg(m_durationBarColor.blue()).arg(m_borderRadius)
									  .arg(m_duration == 0 ? m_borderRadius : 0));

	m_iconSeparator->setStyleSheet(QStringLiteral("background: %1;").arg(m_iconSeparatorColor.name()));

	m_titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(m_titleColor.name()));
	m_textLabel->setStyleSheet(QStringLiteral("color: %1;").arg(m_textColor.name()));
}

Toast* Toast::getPredecessorToast()
{
	Toast* predecessorToast = nullptr;

	for (Toast* toast : s_currentlyShown)
	{
		if (toast == this)
		{
			return predecessorToast;
		}
		predecessorToast = toast;
	}

	return predecessorToast;
}

QImage Toast::recolorImage(QImage image, QColor color)
{
	// Loop through every pixel
	for (int x = 0; x < image.width(); x++) {
		for (int y = 0; y < image.height(); y++) {
			// Get current color of the pixel
			QColor currentColor = image.pixelColor(x, y);

			// Replace the rgb values with rgb of new color and keep alpha the same
			QColor newColor = QColor::fromRgba(qRgba(color.red(), color.green(),
													 color.blue(), currentColor.alpha()));
			image.setPixelColor(x, y, newColor);
		}
	}
	return image;
}

QPixmap Toast::getIconFromEnum(Icon enumIcon)
{
	auto base = QStringLiteral(":/core/toast-%1");

	if (enumIcon == Icon::Success)
	{
		return QPixmap(base.arg(QStringLiteral("success.png")));
	}
	else if (enumIcon == Icon::Warning)
	{
		return QPixmap(base.arg(QStringLiteral("warning.png")));
	}
	else if (enumIcon == Icon::Error)
	{
		return QPixmap(base.arg(QStringLiteral("error.png")));
	}
	else if (enumIcon == Icon::Information)
	{
		return QPixmap(base.arg(QStringLiteral("information.png")));
	}
	else if (enumIcon == Icon::Close)
	{
		return QPixmap(base.arg(QStringLiteral("close.png")));
	}
	return QPixmap();
}

void Toast::updateCurrentlyShowingPositionXY()
{
	for (Toast* toast : s_currentlyShown)
	{
		toast->updatePositionXY();
	}
}

void Toast::updateCurrentlyShowingPositionX()
{
	for (Toast* toast : s_currentlyShown)
	{
		toast->updatePositionX();
	}
}

void Toast::updateCurrentlyShowingPositionY()
{
	for (Toast* toast : s_currentlyShown)
	{
		toast->updatePositionY();
	}
}

void Toast::deleteAndShowNextInQueue()
{
	showNextInQueue();
	deleteLater();
}
