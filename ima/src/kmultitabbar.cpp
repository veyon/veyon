/***************************************************************************
                          kmultitabbar.cpp -  description
                             -------------------
    begin                :  2001
    copyright            : (C) 2001,2002,2003 by Joseph Wenninger <jowenn@kde.org>
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 ***************************************************************************/

#include "kmultitabbar.h"
#include "tool_button.h"
#include "kmultitabbar.moc"

#include <QtGui/QActionEvent>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionButton>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <math.h>

/**
 A bit of an overview about what's going on here:
 There are two sorts of things that can be in the tab bar: tabs and regular 
 buttons. The former are managed by KMultiTabBarInternal, the later 
 by the KMultiTabBar itself. 
*/

class KMultiTabBarPrivate
{
public:
    class KMultiTabBarInternal *m_internal;
    QBoxLayout *m_l;
    QFrame *m_btnTabSep;
    QList<KMultiTabBarButton*> m_buttons;
    KMultiTabBar::KMultiTabBarPosition m_position;
};


KMultiTabBarInternal::KMultiTabBarInternal(QWidget *parent, KMultiTabBar::KMultiTabBarPosition pos):QFrame(parent)
{
	m_position = pos;
	if (pos == KMultiTabBar::Left || pos == KMultiTabBar::Right)
		mainLayout=new QVBoxLayout(this);
	else
		mainLayout=new QHBoxLayout(this);
	mainLayout->setMargin(0);
	mainLayout->setSpacing(0);
	mainLayout->addStretch();
	setFrameStyle(NoFrame);
	setBackgroundRole(QPalette::Background);
}

KMultiTabBarInternal::~KMultiTabBarInternal()
{
	qDeleteAll(m_tabs);
	m_tabs.clear();
}

void KMultiTabBarInternal::setStyle(enum KMultiTabBar::KMultiTabBarStyle style)
{
	m_style=style;
	for (int i=0;i<m_tabs.count();i++)
		m_tabs.at(i)->setStyle(m_style);

	updateGeometry();
}

void KMultiTabBarInternal::contentsMousePressEvent(QMouseEvent *ev)
{
	ev->ignore();
}

void KMultiTabBarInternal::mousePressEvent(QMouseEvent *ev)
{
	ev->ignore();
}

KMultiTabBarTab* KMultiTabBarInternal::tab(int id) const
{
	QListIterator<KMultiTabBarTab*> it(m_tabs);
	while (it.hasNext()) {
		KMultiTabBarTab *tab = it.next();
		if (tab->id()==id) return tab;
	}
	return 0;
}

int KMultiTabBarInternal::appendTab(const QPixmap &pic, int id, const QString& text, const QString & desc)
{
	KMultiTabBarTab  *tab;
	m_tabs.append(tab= new KMultiTabBarTab(pic,text,desc,id,this,m_position,m_style));

	// Insert before the stretch..
	mainLayout->insertWidget(m_tabs.size()-1, tab);
	tab->show();
	return 0;
}

void KMultiTabBarInternal::removeTab(int id)
{
	for (int pos=0;pos<m_tabs.count();pos++)
	{
		if (m_tabs.at(pos)->id()==id)
		{
			// remove & delete the tab
			delete m_tabs.takeAt(pos);
			break;
		}
	}
}

void KMultiTabBarInternal::setPosition(enum KMultiTabBar::KMultiTabBarPosition pos)
{
	m_position=pos;
	for (int i=0;i<m_tabs.count();i++)
		m_tabs.at(i)->setPosition(m_position);
	updateGeometry();
}

// KMultiTabBarButton
///////////////////////////////////////////////////////////////////////////////

KMultiTabBarButton::KMultiTabBarButton(const QPixmap& pic, const QString& text,
					const QString & desc,
                                       int id, QWidget *parent)
	: QPushButton(QIcon(pic), text, parent), m_id(id), d(0), m_origIcon( pic ), m_description( desc )
{
	connect(this,SIGNAL(clicked()),this,SLOT(slotClicked()));
	setFlat(true);
}

KMultiTabBarButton::~KMultiTabBarButton()
{
}

void KMultiTabBarButton::setText(const QString &text)
{
	QPushButton::setText(text);
}

void KMultiTabBarButton::slotClicked()
{
	updateGeometry();
	emit clicked(m_id);
}

int KMultiTabBarButton::id() const 
{
	return m_id;
}

void KMultiTabBarButton::hideEvent( QHideEvent* he) {
	QPushButton::hideEvent(he);
	KMultiTabBar *tb=dynamic_cast<KMultiTabBar*>(parentWidget());
	if (tb) tb->updateSeparator();
}

void KMultiTabBarButton::showEvent( QShowEvent* he) {
	QPushButton::showEvent(he);
	KMultiTabBar *tb=dynamic_cast<KMultiTabBar*>(parentWidget());
	if (tb) tb->updateSeparator();
}

void KMultiTabBarButton::enterEvent( QEvent * _e )
{
	QPoint p = mapToGlobal( QPoint( 0, 0 ) );
	int scr = QApplication::desktop()->isVirtualDesktop() ?
			QApplication::desktop()->screenNumber( p ) :
			QApplication::desktop()->screenNumber( this );

	QPushButton::enterEvent( _e );
	if( toolButton::toolTipsDisabled() )
	{
		return;
	}

#ifdef Q_WS_MAC
	QRect screen = QApplication::desktop()->availableGeometry( scr );
#else
	QRect screen = QApplication::desktop()->screenGeometry( scr );
#endif

	toolButtonTip * tbt = new toolButtonTip( m_origIcon, text(),
						m_description,
				QApplication::desktop()->screen( scr ) );
	connect( this, SIGNAL( mouseLeftButton() ), tbt, SLOT( close() ) );

	if (p.x() + tbt->width() + width() > screen.x() + screen.width())
		p.rx() -= 4 + tbt->width() + width();
	if (p.y() + tbt->height() > screen.y() + screen.height())
		p.ry() -= 24 + tbt->height();
	if (p.y() < screen.y())
		p.setY(screen.y());
	if (p.x() + tbt->width() + width() > screen.x() + screen.width() )
		p.setX(screen.x() + screen.width() - tbt->width()-width());
	if (p.x() < screen.x())
		p.setX(screen.x());
	if (p.y() + tbt->height() > screen.y() + screen.height())
		p.setY(screen.y() + screen.height() - tbt->height());
	tbt->move( p += QPoint( width()+2, 0 ) );
	tbt->show();
}




void KMultiTabBarButton::leaveEvent( QEvent * _e )
{
	emit mouseLeftButton();

	QPushButton::enterEvent( _e );
}




// KMultiTabBarTab
///////////////////////////////////////////////////////////////////////////////

KMultiTabBarTab::KMultiTabBarTab(const QPixmap& pic, const QString& text,
					const QString & desc,
                                       int id, QWidget *parent,
                                       KMultiTabBar::KMultiTabBarPosition pos,
                                       KMultiTabBar::KMultiTabBarStyle style)
	: KMultiTabBarButton(pic, text, desc, id, parent), m_style(style), d(0)
{
	m_position=pos;
//	setToolTip(text);	
	setCheckable(true);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	 // shrink down to icon only, but prefer to show text if it's there
}

KMultiTabBarTab::~KMultiTabBarTab()
{
}

void KMultiTabBarTab::setPosition(KMultiTabBar::KMultiTabBarPosition pos)
{
	m_position=pos;
	updateGeometry();
}

void KMultiTabBarTab::setStyle(KMultiTabBar::KMultiTabBarStyle style)
{
	m_style=style;
	updateGeometry();
}

QPixmap KMultiTabBarTab::iconPixmap() const
{
	int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
	return icon().pixmap(iconSize);
}

void KMultiTabBarTab::initStyleOption(QStyleOptionToolButton* opt) const
{
	opt->initFrom(this);
	
	// Setup icon..
	if (!icon().isNull()) {
		opt->iconSize = iconPixmap().size();
		opt->icon     = icon();
	}
		
	// Should we draw text?
	if (shouldDrawText())
		opt->text = text();
		
	if (underMouse())
		opt->state |= QStyle::State_AutoRaise | QStyle::State_MouseOver | QStyle::State_Raised;
		
	if (isChecked())
		opt->state |= QStyle::State_Sunken | QStyle::State_On;
	
	opt->font = font();
	opt->toolButtonStyle = shouldDrawText() ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly;
	opt->subControls = QStyle::SC_ToolButton;
}

QSize KMultiTabBarTab::sizeHint() const
{
	return computeSizeHint(shouldDrawText());
}

QSize KMultiTabBarTab::minimumSizeHint() const
{
	return computeSizeHint(false);
}

void KMultiTabBarTab::computeMargins(int* hMargin, int* vMargin) const
{
	// Unfortunately, QStyle does not give us enough information to figure out 
	// where to place things, so we try to reverse-engineer it
	QStyleOptionToolButton opt;
	initStyleOption(&opt);

	QPixmap iconPix = iconPixmap();
	QSize trialSize = iconPix.size();
	QSize expandSize = style()->sizeFromContents(QStyle::CT_ToolButton, &opt, trialSize, this);
	
	*hMargin = (expandSize.width()  - trialSize.width())/2;
	*vMargin = (expandSize.height() - trialSize.height())/2;
}

QSize KMultiTabBarTab::computeSizeHint(bool withText) const
{
	// Compute as horizontal first, then flip around if need be.	
	QStyleOptionToolButton opt;
	initStyleOption(&opt);
	
	int hMargin, vMargin;
	computeMargins(&hMargin, &vMargin);
	
	// Compute interior size, starting from pixmap..
	QPixmap iconPix = iconPixmap();
	QSize size = iconPix.size();
	
	// Always include text height in computation, to avoid resizing the minor direction
	// when expanding text..
	QSize textSize = fontMetrics().size(Qt::TextShowMnemonic, text());
	size.setHeight(qMax(size.height(), textSize.height()));
	
	// Pick margins for major/minor direction, depending on orientation
	int majorMargin = isVertical() ? vMargin : hMargin;
	int minorMargin = isVertical() ? hMargin : vMargin;
	
	size.setWidth (size.width()  + 2*majorMargin);
	size.setHeight(size.height() + 2*minorMargin);

	if (withText)
		// Add enough room for the text, and an extra major margin.
		size.setWidth(size.width() + textSize.width() + majorMargin);

	// flip time?
	if (isVertical())
		return QSize(size.height(), size.width());
	else
		return size;
}

void KMultiTabBarTab::setState(bool newState)
{
	setChecked(newState);
	updateGeometry();
}

void KMultiTabBarTab::setIcon(const QString& icon)
{
//	QPixmap pic=SmallIcon(icon);
	setIcon(QPixmap(icon));
}

void KMultiTabBarTab::setIcon(const QPixmap& icon)
{
	QPushButton::setIcon(icon);
}

bool KMultiTabBarTab::shouldDrawText() const
{
	return (m_style==KMultiTabBar::KDEV3ICON) || isChecked();
}

bool KMultiTabBarTab::isVertical() const
{
	return m_position==KMultiTabBar::Right || m_position==KMultiTabBar::Left;
}

void KMultiTabBarTab::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	
	QStyleOptionToolButton opt;
	initStyleOption(&opt);
	
	// Paint bevel..
	if (underMouse() || isChecked())
		style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &painter);
		
	int hMargin, vMargin;
	computeMargins(&hMargin, &vMargin);
		
	// We first figure out how much room we have for the text, based on 
	// icon size and margin, try to fit in by eliding, and perhaps
	// give up on drawing the text entirely if we're too short on room
	QPixmap icon = iconPixmap();
	int textRoom = 0;
	int iconRoom = 0;
	
	QString t;
	if (shouldDrawText()) {
		if (isVertical()) {
			iconRoom = icon.height() + 2*vMargin;
			textRoom = height() - iconRoom - vMargin;
		} else {
			iconRoom = icon.width() + 2*hMargin;
			textRoom = width() - iconRoom - hMargin;
		}
		
		t = painter.fontMetrics().elidedText(text(), Qt::ElideRight, textRoom);
		
		// See whether anything is left. Qt will return either 
		// ... or the ellipsis unicode character, 0x2026
		if (t == QLatin1String("...") || t == QChar(0x2026))
			t.clear();
	}
	
	opt.text = t;
	
	// Label time.... Simple case: no text, so just plop down the icon right in the center
	// We only do this when the button never draws the text, to avoid jumps in icon position
	// when resizing
 	if (!shouldDrawText()) {
 		style()->drawItemPixmap(&painter, rect(), Qt::AlignCenter | Qt::AlignVCenter, icon);
 		return;
 	}

	// Now where the icon/text goes depends on text direction and tab position	
	QRect iconArea;
	QRect labelArea;
	
	bool bottomIcon = false;
	bool rtl = layoutDirection() == Qt::RightToLeft;
	if (isVertical()) {
		if (m_position == KMultiTabBar::Left && !rtl)
			bottomIcon = true;
		if (m_position == KMultiTabBar::Right && rtl)
			bottomIcon = true;
	}
	//alignFlags = Qt::AlignLeading | Qt::AlignVCenter;

	if (isVertical()) {
		if (bottomIcon) {
			labelArea = QRect(0, vMargin, width(), textRoom);
			iconArea  = QRect(0, vMargin + textRoom, width(), iconRoom);
		} else {
			labelArea = QRect(0, iconRoom, width(), textRoom);
			iconArea  = QRect(0, 0, width(), iconRoom);
		}
	} else {
		// Pretty simple --- depends only on RTL/LTR
		if (rtl) {
			labelArea = QRect(hMargin, 0, textRoom, height());
			iconArea  = QRect(hMargin + textRoom, 0, iconRoom, height());
		} else {
			labelArea = QRect(iconRoom, 0, textRoom, height());
			iconArea  = QRect(0, 0, iconRoom, height());
		}
	}
	
	style()->drawItemPixmap(&painter, iconArea, Qt::AlignCenter | Qt::AlignVCenter, icon);
	
	if (t.isEmpty())
		return;

	QRect labelPaintArea = labelArea;

	if (isVertical()) {
		// If we're vertical, we paint to a simple 0,0 origin rect, 
		// and get the transformations to get us in the right place
		labelPaintArea = QRect(0, 0, labelArea.height(), labelArea.width());

		QTransform tr;
		
		if (bottomIcon) {
			tr.translate(labelArea.x(), labelPaintArea.width() + labelArea.y());
			tr.rotate(-90);
		} else {
			tr.translate(labelPaintArea.height() + labelArea.x(), labelArea.y());
			tr.rotate(90);
		}
		painter.setTransform(tr);
	}
	
	style()->drawItemText(&painter, labelPaintArea, Qt::AlignLeading | Qt::AlignVCenter,
	                      palette(), true, t, QPalette::ButtonText);
}

// KMultiTabBar
///////////////////////////////////////////////////////////////////////////////

KMultiTabBar::KMultiTabBar(KMultiTabBarPosition pos, QWidget *parent)
    : QWidget(parent),
    d(new KMultiTabBarPrivate)
{
	if (pos == Left || pos == Right)
	{
		d->m_l=new QVBoxLayout(this);
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding/*, true*/);
	}
	else
	{
		d->m_l=new QHBoxLayout(this);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed/*, true*/);
	}
	d->m_l->setMargin(0);
	d->m_l->setSpacing(0);

	d->m_internal=new KMultiTabBarInternal(this, pos);
	setPosition(pos);
	setStyle(VSNET);
	d->m_l->insertWidget(0,d->m_internal);
	d->m_l->insertWidget(0,d->m_btnTabSep=new QFrame(this));
	d->m_btnTabSep->setFixedHeight(4);
	d->m_btnTabSep->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	d->m_btnTabSep->setLineWidth(2);
	d->m_btnTabSep->hide();

	updateGeometry();
}

KMultiTabBar::~KMultiTabBar()
{
	qDeleteAll( d->m_buttons );
	d->m_buttons.clear();
	delete d;
}

int KMultiTabBar::appendButton(const QPixmap &pic, int id, QMenu *popup, const QString&)
{
	KMultiTabBarButton *btn = new KMultiTabBarButton(pic, QString(), QString(), id, this);
	btn->setMenu(popup);
	d->m_buttons.append(btn);
	d->m_l->insertWidget(0,btn);
	btn->show();
	d->m_btnTabSep->show();
	return 0;
}

void KMultiTabBar::updateSeparator() {
	bool hideSep=true;
	QListIterator<KMultiTabBarButton*> it(d->m_buttons);
	while (it.hasNext()){
		if (it.next()->isVisibleTo(this)) {
			hideSep=false;
			break;
		}
	}
	if (hideSep)
		d->m_btnTabSep->hide();
	else 
		d->m_btnTabSep->show();
}

int KMultiTabBar::appendTab(const QPixmap &pic, int id, const QString& text, const QString & desc)
{
	d->m_internal->appendTab(pic,id,text,desc);
	return 0;
}

KMultiTabBarButton* KMultiTabBar::button(int id) const
{
	QListIterator<KMultiTabBarButton*> it(d->m_buttons);
	while ( it.hasNext() ) {
		KMultiTabBarButton *button = it.next();
		if ( button->id() == id )
			return button;
	}
	
	return 0;
}

KMultiTabBarTab* KMultiTabBar::tab(int id) const
{
    return d->m_internal->tab(id);
}

void KMultiTabBar::removeButton(int id)
{
	for (int pos=0;pos<d->m_buttons.count();pos++)
	{
		if (d->m_buttons.at(pos)->id()==id)
		{
			d->m_buttons.takeAt(pos)->deleteLater();
			break;
		}
	}
	
	if (d->m_buttons.count()==0)
		d->m_btnTabSep->hide();
}

void KMultiTabBar::removeTab(int id)
{
	d->m_internal->removeTab(id);
}

void KMultiTabBar::setTab(int id,bool state)
{
	KMultiTabBarTab *ttab=tab(id);
	if (ttab)
		ttab->setState(state);
}

bool KMultiTabBar::isTabRaised(int id) const
{
	KMultiTabBarTab *ttab=tab(id);
	if (ttab)
		return ttab->isChecked();

	return false;
}

void KMultiTabBar::setStyle(KMultiTabBarStyle style)
{
	d->m_internal->setStyle(style);
}

KMultiTabBar::KMultiTabBarStyle KMultiTabBar::tabStyle() const
{
	return d->m_internal->m_style;
}

void KMultiTabBar::setPosition(KMultiTabBarPosition pos)
{
	d->m_position=pos;
	d->m_internal->setPosition(pos);
}

KMultiTabBar::KMultiTabBarPosition KMultiTabBar::position() const
{
	return d->m_position;
}

void KMultiTabBar::fontChange(const QFont& /* oldFont */)
{
	updateGeometry();
}

// vim: ts=4 sw=4 noet
// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
