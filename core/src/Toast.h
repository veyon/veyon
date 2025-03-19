/*
 * File based on https://github.com/niklashenning/qt-toast/blob/master/src/Toast.h
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

#pragma once

#include <deque>
#include <QDialog>

class QGraphicsOpacityEffect;
class QLabel;

class Toast : public QDialog
{
	Q_OBJECT
public:
	enum class Preset
	{
		Success,
		Warning,
		Error,
		Information,
	};

	enum class Icon
	{
		Success,
		Warning,
		Error,
		Information,
		Close
	};

	enum class Position
	{
		BottomLeft,
		BottomMiddle,
		BottomRight,
		TopLeft,
		TopMiddle,
		TopRight,
		Center
	};

	enum class ButtonAlignment
	{
		Top,
		Middle,
		Bottom
	};

	Toast(QWidget* parent = nullptr);
	~Toast();

	static int getMaximumOnScreen();
	static int getSpacing();
	static QPoint getOffset();
	static int getOffsetX();
	static int getOffsetY();
	static bool isAlwaysOnMainScreen();
	static QScreen* getFixedScreen();
	static Position getPosition();
	static int getCount();
	static int getVisibleCount();
	static int getQueuedCount();
	int getDuration();
	bool isShowDurationBar();
	QString getTitle();
	QString getText();
	QPixmap getIcon();
	bool isShowIcon();
	QSize getIconSize();
	bool isShowIconSeparator();
	int getIconSeparatorWidth();
	QPixmap getCloseButtonIcon();
	bool isShowCloseButton();
	QSize getCloseButtonIconSize();
	int getCloseButtonWidth();
	int getCloseButtonHeight();
	QSize getCloseButtonSize();
	ButtonAlignment getCloseButtonAlignment();
	int getFadeInDuration();
	int getFadeOutDuration();
	bool isResetDurationOnHover();
	bool isStayOnTop();
	int getBorderRadius();
	QColor getBackgroundColor();
	QColor getTitleColor();
	QColor getTextColor();
	QColor getIconColor();
	QColor getIconSeparatorColor();
	QColor getCloseButtonIconColor();
	QColor getDurationBarColor();
	QFont getTitleFont();
	QFont getTextFont();
	QMargins getMargins();
	QMargins getIconMargins();
	QMargins getIconSectionMargins();
	QMargins getTextSectionMargins();
	QMargins getCloseButtonMargins();
	int getTextSectionSpacing();

	static void setMaximumOnScreen(int maximum);
	static void setSpacing(int spacing);
	static void setOffset(int x, int y);
	static void setOffsetX(int offsetX);
	static void setOffsetY(int offsetY);
	static void setAlwaysOnMainScreen(bool enabled);
	static void setFixedScreen(QScreen* screen);
	static void setPosition(Position position);
	static void reset();
	void setDuration(int duration);
	void setShowDurationBar(bool enabled);
	void setTitle(QString title);
	void setText(QString text);
	void setIcon(QPixmap icon);
	void setIcon(Icon icon);
	void setShowIcon(bool enabled);
	void setIconSize(QSize size);
	void setShowIconSeparator(bool enabled);
	void setIconSeparatorWidth(int width);
	void setCloseButtonIcon(QPixmap icon);
	void setCloseButtonIcon(Icon icon);
	void setShowCloseButton(bool enabled);
	void setCloseButtonIconSize(QSize size);
	void setCloseButtonSize(QSize size);
	void setCloseButtonWidth(int width);
	void setCloseButtonHeight(int height);
	void setCloseButtonAlignment(ButtonAlignment alignment);
	void setFadeInDuration(int duration);
	void setFadeOutDuration(int duration);
	void setResetDurationOnHover(bool enabled);
	void setStayOnTop(bool enabled);
	void setBorderRadius(int borderRadius);
	void setBackgroundColor(QColor color);
	void setTitleColor(QColor color);
	void setTextColor(QColor color);
	void setIconColor(QColor color);
	void setIconSeparatorColor(QColor color);
	void setCloseButtonIconColor(QColor color);
	void setDurationBarColor(QColor color);
	void setTitleFont(QFont font);
	void setTextFont(QFont font);
	void setMargins(QMargins margins);
	void setMarginLeft(int margin);
	void setMarginTop(int margin);
	void setMarginRight(int margin);
	void setMarginBottom(int margin);
	void setIconMargins(QMargins margins);
	void setIconMarginLeft(int margin);
	void setIconMarginTop(int margin);
	void setIconMarginRight(int margin);
	void setIconMarginBottom(int margin);
	void setIconSectionMargins(QMargins margins);
	void setIconSectionMarginLeft(int margin);
	void setIconSectionMarginTop(int margin);
	void setIconSectionMarginRight(int margin);
	void setIconSectionMarginBottom(int margin);
	void setTextSectionMargins(QMargins margins);
	void setTextSectionMarginLeft(int margin);
	void setTextSectionMarginTop(int margin);
	void setTextSectionMarginRight(int margin);
	void setTextSectionMarginBottom(int margin);
	void setCloseButtonMargins(QMargins margins);
	void setCloseButtonMarginLeft(int margin);
	void setCloseButtonMarginTop(int margin);
	void setCloseButtonMarginRight(int margin);
	void setCloseButtonMarginBottom(int margin);
	void setTextSectionSpacing(int spacing);
	void setFixedSize(QSize size);
	void setFixedSize(int width, int height);
	void setFixedWidth(int width);
	void setFixedHeight(int height);
	void applyPreset(Preset preset);


public Q_SLOTS:
	void show();
	void hide();

Q_SIGNALS:
	void closed();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	using EnterEvent = QEnterEvent;
#else
	using EnterEvent = QEvent;
#endif
	void enterEvent(EnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private Q_SLOTS:
	void hide_();
	void fadeOut();
	void updateDurationBar();
	void deleteAndShowNextInQueue();

private:
	static int s_maximumOnScreen;
	static int s_spacing;
	static int s_offsetX;
	static int s_offsetY;
	static bool s_alwaysOnMainScreen;
	static QScreen* s_fixedScreen;
	static Position s_position;
	static std::deque<Toast*> s_currentlyShown;
	static std::deque<Toast*> s_queue;
	int m_duration;
	bool m_showDurationBar;
	QString m_title;
	QString m_text;
	QPixmap m_icon;
	bool m_showIcon;
	QSize m_iconSize;
	bool m_showIconSeparator;
	int m_iconSeparatorWidth;
	QPixmap m_closeButtonIcon;
	bool m_showCloseButton;
	QSize m_closeButtonIconSize;
	QSize m_closeButtonSize;
	ButtonAlignment m_closeButtonAlignment;
	int m_fadeInDuration;
	int m_fadeOutDuration;
	bool m_resetDurationOnHover;
	bool m_stayOnTop;
	int m_borderRadius;
	QColor m_backgroundColor;
	QColor m_titleColor;
	QColor m_textColor;
	QColor m_iconColor;
	QColor m_iconSeparatorColor;
	QColor m_closeButtonIconColor;
	QColor m_durationBarColor;
	QFont m_titleFont;
	QFont m_textFont;
	QMargins m_margins;
	QMargins m_iconMargins;
	QMargins m_iconSectionMargins;
	QMargins m_textSectionMargins;
	QMargins m_closeButtonMargins;
	int m_textSectionSpacing;
	int m_elapsedTime;
	bool m_fadingOut;
	bool m_used;
	QWidget* m_parent;
	QLabel* m_notification;
	QWidget* m_dropShadowLayer1;
	QWidget* m_dropShadowLayer2;
	QWidget* m_dropShadowLayer3;
	QWidget* m_dropShadowLayer4;
	QWidget* m_dropShadowLayer5;
	QGraphicsOpacityEffect* m_opacityEffect;
	QPushButton* m_closeButton;
	QLabel* m_titleLabel;
	QLabel* m_textLabel;
	QPushButton* m_iconWidget;
	QWidget* m_iconSeparator;
	QWidget* m_durationBarContainer;
	QWidget* m_durationBar;
	QWidget* m_durationBarChunk;
	QTimer* m_durationTimer;
	QTimer* m_durationBarTimer;

	void setupUI();
	void updatePositionXY();
	void updatePositionX();
	void updatePositionY();
	void updateStylesheet();
	QPoint calculatePosition();
	Toast* getPredecessorToast();
	static QImage recolorImage(QImage image, QColor color);
	static QPixmap getIconFromEnum(Icon enumIcon);
	static void updateCurrentlyShowingPositionXY();
	static void updateCurrentlyShowingPositionX();
	static void updateCurrentlyShowingPositionY();
	static void showNextInQueue();

	static const int sc_updatePositionDuration;
	static const int sc_durationBarUpdateInterval;
	static const int sc_dropShadowSize;
	static const QColor sc_successAccentColor;
	static const QColor sc_warningAccentColor;
	static const QColor sc_errorAccentColor;
	static const QColor sc_informationAccentColor;
	static const QColor sc_defaultAccentColor;
	static const QColor sc_defaultBackgroundColor;
	static const QColor sc_defaultTitleColor;
	static const QColor sc_defaultTextColor;
	static const QColor sc_defaultIconSeparatorColor;
	static const QColor sc_defaultCloseButtonIconColor;
	static const QColor sc_defaultBackgroundColorDark;
	static const QColor sc_defaultTitleColorDark;
	static const QColor sc_defaultTextColorDark;
	static const QColor sc_defaultIconSeparatorColorDark;
	static const QColor sc_defaultCloseButtonIconColorDark;
};
