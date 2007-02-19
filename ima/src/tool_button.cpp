/*
 * tool_button.cpp - implementation of iTALC-tool-button
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
 

#include <QtCore/QTimer>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QDesktopWidget>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>

#include "tool_button.h"
#include "fast_qimage.h"



const int MARGIN = 10;
const int ROUNDED = 3000;

bool toolButton::s_toolTipsDisabled = FALSE;


toolButtonTip::toolButtonTip( const QPixmap & _pixmap, const QString & _title,
				const QString & _description,
							QWidget * _parent ) :
	QWidget( _parent, Qt::ToolTip ),
	m_icon( fastQImage( _pixmap ).scaled( 72, 72 ) ),
	m_title( _title ),
	m_description( _description ),
	m_dissolveSize( 24 )
{
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
	setAttribute( Qt::WA_NoSystemBackground, TRUE );

	resize( sizeHint() );
	updateMask();
}




QSize toolButtonTip::sizeHint( void ) const
{
	QFont f = font();
	f.setBold( TRUE );
	int title_w = QFontMetrics( f ).width( m_title );
	QRect desc_rect = fontMetrics().boundingRect( QRect( 0, 0, 250, 100 ),
					Qt::TextWordWrap, m_description );

	return QSize( MARGIN + m_icon.width() + MARGIN +
				qMax( title_w, desc_rect.width() ) + MARGIN,
			MARGIN + qMax( m_icon.height(), fontMetrics().height() +
						MARGIN + desc_rect.height() ) +
								MARGIN );
}




void toolButtonTip::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.drawPixmap( 0, 0, m_bg );
}




void toolButtonTip::resizeEvent( QResizeEvent * _re )
{
	m_bg = QPixmap( size() );
	QPainter p( &m_bg );
	p.setRenderHint( QPainter::Antialiasing );
	p.setPen( QColor( 0, 0, 0 ) );
	QLinearGradient grad( 0, 0, 0, height() );
	grad.setColorAt( 0, palette().color( QPalette::Active,
						QPalette::Window ).
							light( 120 ) );
	grad.setColorAt( 1, palette().color( QPalette::Active,
						QPalette::Window ).
							light( 80 ) );
	p.setBrush( grad );
	p.drawRoundRect( 0, 0, width() - 1, height() - 1,
					ROUNDED / width(), ROUNDED / height() );

	p.drawImage( MARGIN, MARGIN, m_icon );
	QFont f = p.font();
	f.setBold( TRUE );
	p.setFont( f );
	const int title_x = MARGIN + m_icon.width() + MARGIN;
	const int title_y = MARGIN + fontMetrics().height() - 2;
	p.drawText( title_x, title_y, m_title );

	f.setBold( FALSE );
	p.setFont( f );
	p.drawText( QRect( title_x, title_y + MARGIN,
					width() - MARGIN - title_x,
					height() - MARGIN - title_y ),
					Qt::TextWordWrap, m_description );

	QWidget::resizeEvent( _re );
}




void toolButtonTip::updateMask( void )
{
	// as this widget has not a rectangular shape AND is a top
	// level widget (which doesn't allow painting only particular
	// regions), we have to set a mask for it
	QBitmap b( size() );
	b.clear();

	QPainter p( &b );
	p.setBrush( Qt::color1 );
	p.setPen( Qt::color1 );
	p.drawRoundRect( 0, 0, width() - 1, height() - 1,
					ROUNDED / width(), ROUNDED / height() );

	p.setBrush( Qt::color0 );
	p.setPen( Qt::color0 );

	if( m_dissolveSize > 0 )
	{
		const int size = 16;
		for( Q_UINT16 y = 0; y < height() + size; y += size )
		{
			Q_INT16 s = m_dissolveSize * width() / 128;
			for( Q_INT16 x = width(); x > -size; x -= size, s -= 2 )
			{
				if( s < 0 )
				{
					s = 0;
				}
				p.drawEllipse( x - s / 2, y - s / 2, s, s );
			}
		}
	}

	setMask( b );

	if( --m_dissolveSize >= 0 )
	{
		QTimer::singleShot( 20, this, SLOT( updateMask() ) );
	}
}




toolButton::toolButton( const QPixmap & _pixmap, const QString & _title,
			const QString & _description,
			QObject * _receiver, const char * _slot,
			QWidget * _parent ) :
	QToolButton( _parent ),
	m_pixmap( _pixmap ),
	m_title( _title ),
	m_description( _description ),
	m_mouseOver( FALSE )
{
	setText( m_title );
	if( _receiver != NULL && _slot != NULL )
	{
		connect( this, SIGNAL( clicked() ), _receiver, _slot );
	}
	setFixedSize( 47, 47 );
}




toolButton::~toolButton()
{
}





void toolButton::enterEvent( QEvent * _e )
{
	if( !s_toolTipsDisabled )
	{
		QPoint p = mapToGlobal( QPoint( 0, 0 ) );
		int scr = QApplication::desktop()->isVirtualDesktop() ?
				QApplication::desktop()->screenNumber( p ) :
				QApplication::desktop()->screenNumber( this );

#ifdef Q_WS_MAC
		QRect screen = QApplication::desktop()->availableGeometry(
									scr );
#else
		QRect screen = QApplication::desktop()->screenGeometry( scr );
#endif

		toolButtonTip * tbt = new toolButtonTip( m_pixmap, m_title,
							m_description,
				QApplication::desktop()->screen( scr ) );
		connect( this, SIGNAL( mouseLeftButton() ),
							tbt, SLOT( close() ) );

		if( p.x() + tbt->width() > screen.x() + screen.width() )
			p.rx() -= 4;// + tbt->width();
		if( p.y() + tbt->height() > screen.y() + screen.height() )
			p.ry() -= 24 + tbt->height();
		if( p.y() < screen.y() )
			p.setY( screen.y() );
		if( p.x() + tbt->width() > screen.x() + screen.width() )
			p.setX( screen.x() + screen.width() - tbt->width() );
		if( p.x() < screen.x() )
			p.setX( screen.x() );
		if( p.y() + tbt->height() > screen.y() + screen.height() )
			p.setY( screen.y() + screen.height() - tbt->height() );
		tbt->move( p += QPoint( 0, 48 ) );
		tbt->show();
	}

	m_mouseOver = TRUE;

	QToolButton::enterEvent( _e );
}




void toolButton::leaveEvent( QEvent * _e )
{
	emit mouseLeftButton();

	m_mouseOver = FALSE;

	QToolButton::enterEvent( _e );
}




void toolButton::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );
	if( m_mouseOver || isDown() || isChecked() )
	{
		p.setPen( QColor( 64, 64, 64 ) );
		QLinearGradient grad( 0, 0, 0, height() );
		const int a1 = isChecked() ? -30 : ( isDown() ? -30 : 0 );
		const int a2 = isDown() ? -30 : 0;
		grad.setColorAt( 0, palette().
				color( QPalette::Active, QPalette::Window ).
						light( 110 + a1 + a2 ) );
		grad.setColorAt( 1, palette().
				color( QPalette::Active, QPalette::Window ).
						light( 90 - a1 + a2 ) );
		p.setBrush( grad );
	}
	else
	{
		p.setPen( palette().color( QPalette::Active,
							QPalette::Window ) );
		p.setBrush( p.pen().color() );
	}

	p.drawRoundRect( 0, 0, width() - 1, height() - 1, 20, 20 );
	p.drawImage( 3, 3, fastQImage( m_pixmap ).scaled(
						width() - 7, height() - 7 ) );
}



#if 0
void toolButton::resizeEvent( QResizeEvent * _re )
{
	QToolButton::resizeEvent( _re );
	setIcon( m_pixmap.scaled( width() - 5, height() - 5,
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation ) );
/*	QPainter p( this );
	p.drawPixmap( 3, 3, m_pixmap.scaled( width() - 6, height() - 6,
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation ) );*/
}
#endif




#include "tool_button.moc"
