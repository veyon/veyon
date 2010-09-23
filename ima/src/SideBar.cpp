/*
 * SideBar.cpp - side bar in MainWindow
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QToolButton>

#include "SideBar.h"
#include "SideBarWidget.h"


// internal helper class allowing to create QToolButtons with
// vertical orientation
class SideBarButton : public QToolButton
{
public:
	SideBarButton( Qt::Orientation _orientation, QWidget * _parent ) :
		QToolButton( _parent ),
		m_orientation( _orientation )
	{
	}
	virtual ~SideBarButton()
	{
	}

	Qt::Orientation orientation() const
	{
		return m_orientation;
	}

	virtual QSize sizeHint() const
	{
		QSize s = QToolButton::sizeHint();
		s.setWidth( s.width() + 12 );
		s.setHeight( 24 );
		if( orientation() == Qt::Horizontal )
		{
			return s;
		}
		return QSize( s.height(), s.width() );
	}


protected:
	virtual void paintEvent( QPaintEvent * )
	{
		QPainter painter( this );
		QRect r = rect();
		if( orientation() == Qt::Vertical )
		{
			const QSize s = sizeHint();
			painter.rotate( 270 );
			painter.translate( -s.height(), 0 );
			r = QRect( 0, 0, s.height(), s.width() );
		}

        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

		QLinearGradient outlinebrush(0, 0, 0, height());
		QLinearGradient brush(0, 0, 0, height());

		brush.setSpread(QLinearGradient::PadSpread);
		QColor highlight(255, 255, 255, 70);
		QColor shadow(0, 0, 0, 70);
		QColor sunken(220, 220, 220, 30);
		QColor normal1(255, 255, 245, 60);
		QColor normal2(255, 255, 235, 10);

		if( isDown() || isChecked() )
		{
			outlinebrush.setColorAt(0.0f, shadow);
			outlinebrush.setColorAt(1.0f, highlight);
			brush.setColorAt(0.0f, sunken);
			painter.setPen(Qt::NoPen);
		}
		else
		{
			outlinebrush.setColorAt(1.0f, shadow);
			outlinebrush.setColorAt(0.0f, highlight);
			brush.setColorAt(0.0f, normal1);
			if( !underMouse() )
			{
				brush.setColorAt(1.0f, normal2);
			}
			painter.setPen(QPen(outlinebrush, 1));
		}
		painter.setBrush(brush);

		painter.drawRoundedRect(0, 0, r.width(), r.height(), 5, 5);

		painter.drawPixmap( 10, 5, icon().pixmap( iconSize() ) );

		if( isChecked() )
		{
			const QString l = text();
			painter.setPen( Qt::black );
			painter.drawText( 18 + iconSize().width() + 1, 19, l );
			painter.setPen( Qt::white );
			painter.drawText( 18 + iconSize().width(), 18, l );
		}
	}


private:
	Qt::Orientation m_orientation;

} ;




SideBar::SideBar( QWidget * _parent ) :
	QToolBar( _parent ),
	m_btnGroup( this )
{
	m_btnGroup.setExclusive( false );
	connect( &m_btnGroup, SIGNAL( buttonClicked( QAbstractButton * ) ),
				this, SLOT( toggleButton( QAbstractButton * ) ) );

	QPalette pal = palette();
	pal.setBrush( QPalette::Window, QPixmap( ":/resources/toolbar-background.png" ) );
	setPalette( pal );
}




SideBar::~SideBar()
{
}




void SideBar::appendTab( SideBarWidget * _sbw )
{
	SideBarButton * btn = new SideBarButton( orientation(), this );
	btn->setText( _sbw->title() );
	btn->setIcon( _sbw->icon() );
	btn->setCheckable( true );
	m_widgets[btn] = _sbw;
	m_btnGroup.addButton( btn );
	addWidget( btn );

	_sbw->hide();
	_sbw->setMinimumWidth( 200 );
}




int SideBar::activeTab() const
{
	int i = 0;
	for( ButtonMap::ConstIterator it = m_widgets.begin();
							it != m_widgets.end(); ++it, ++i )
	{
		if( it.key()->isChecked() )
		{
			return i;
		}
	}
	return 0;
}




void SideBar::toggleButton( QAction * _a )
{
	foreach( QAbstractButton * btn, tabs() )
	{
		if( btn->text() == _a->text() )
		{
			if( btn->isVisible() )
			{
				btn->setVisible( false );
			}
			else
			{
				btn->setVisible( false );
			}
			break;
		}
	}
}




void SideBar::toggleButton( QAbstractButton * _btn )
{
	QToolButton * toolButton = NULL;
	QWidget * activeWidget = NULL;
	for( ButtonMap::Iterator it = m_widgets.begin();
							it != m_widgets.end(); ++it )
	{
		QToolButton * curBtn = it.key();
		if( curBtn != _btn )
		{
			curBtn->setChecked( false );
			curBtn->setToolButtonStyle( Qt::ToolButtonIconOnly );
		}
		else
		{
			toolButton = it.key();
			activeWidget = it.value();
		}
		if( it.value() )
		{
			it.value()->hide();
		}
	}

	if( toolButton && activeWidget )
	{
		activeWidget->setVisible( _btn->isChecked() );
		toolButton->setToolButtonStyle( _btn->isChecked() ?
				Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly );
	}
}




void SideBar::contextMenuEvent( QContextMenuEvent * _e )
{
	QMenu m( this );
	foreach( QAbstractButton * btn, tabs() )
	{
		QAction * ma = m.addAction( btn->text() );
		ma->setCheckable( true );
		ma->setChecked( btn->isVisible() );
	}
	connect( &m, SIGNAL( triggered( QAction * ) ),
				this, SLOT( toggleButton( QAction * ) ) );
	m.exec( _e->globalPos() );
}




void SideBar::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( _pe->rect(), palette().brush( QPalette::Window ) );

	p.setPen( QColor( 48, 48, 48 ) );
	if( orientation() == Qt::Vertical )
	{
		p.drawLine( 0, 0, 0, height() );
		p.drawLine( width()-1, 0, width()-1, height() );
	}
	else
	{
		p.drawLine( 0, 0, width(), 0 );
		p.drawLine( 0, height()-1, width(), height()-1 );
	}
}


