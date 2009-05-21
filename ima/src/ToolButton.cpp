/*
 * ToolButton.cpp - implementation of iTALC-tool-button
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QToolBar>

#include "ToolButton.h"


const int MARGIN = 10;
const int ROUNDED = 2000;

#ifndef ITALC3
bool ToolButton::s_toolTipsDisabled = FALSE;
bool ToolButton::s_iconOnlyMode = FALSE;
#endif


ToolButton::ToolButton( const QPixmap & _pixmap, const QString & _label,
				const QString & _alt_label,
				const QString & _title, 
				const QString & _desc, QObject * _receiver, 
				const char * _slot, QWidget * _parent ) :
	QToolButton( _parent ),
	m_pixmap( _pixmap ),
	m_img( FastQImage( _pixmap.toImage() ).scaled( 32, 32 ) ),
	m_colorizeLevel( 0 ),
	m_fadeBack( FALSE ),
	m_label( _label ),
	m_altLabel( _alt_label ),
	m_title( _title ),
	m_descr( _desc )
{
	setAttribute( Qt::WA_NoSystemBackground, true );
	setText( m_title );

	updateSize();

	if( _receiver != NULL && _slot != NULL )
	{
		connect( this, SIGNAL( clicked() ), _receiver, _slot );
	}

}




ToolButton::ToolButton( QAction * _a, const QString & _label,
				const QString & _alt_label,
				const QString & _desc, QObject * _receiver, 
				const char * _slot, QWidget * _parent ) :
	QToolButton( _parent ),
	m_pixmap( _a->icon().pixmap( 128, 128 ) ),
	m_img( FastQImage( m_pixmap.toImage() ).scaled( 32, 32 ) ),
	m_colorizeLevel( 0 ),
	m_fadeBack( FALSE ),
	m_label( _label ),
	m_altLabel( _alt_label ),
	m_title( _a->text() ),
	m_descr( _desc )
{
	setAttribute( Qt::WA_NoSystemBackground, true );
	setText( m_title );

	updateSize();

	if( _receiver != NULL && _slot != NULL )
	{
		connect( this, SIGNAL( clicked() ), _receiver, _slot );
		connect( _a, SIGNAL( triggered( bool ) ), _receiver, _slot );
	}

}




ToolButton::~ToolButton()
{
}




void ToolButton::addTo( QToolBar * _tb )
{
	QAction * a = _tb->addWidget( this );
	a->setText( text() );
}




void ToolButton::enterEvent( QEvent * _e )
{
	m_fadeBack = FALSE;
	if( m_colorizeLevel == 0 )
	{
		updateColorLevel();
	}
	if( !s_toolTipsDisabled && !m_title.isEmpty() && !m_descr.isEmpty() )
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

		ToolButtonTip * tbt = new ToolButtonTip( m_pixmap, m_title,
							m_descr,
				QApplication::desktop()->screen( scr ), this );
		connect( this, SIGNAL( mouseLeftButton() ),
							tbt, SLOT( close() ) );

		if( p.x() + tbt->width() > screen.x() + screen.width() )
			p.rx() -= 4;// + tbt->width();
		if( p.y() + tbt->height() > screen.y() + screen.height() )
			p.ry() -= 30 + tbt->height();
		if( p.y() < screen.y() )
			p.setY( screen.y() );
		if( p.x() + tbt->width() > screen.x() + screen.width() )
			p.setX( screen.x() + screen.width() - tbt->width() );
		if( p.x() < screen.x() )
			p.setX( screen.x() );
		if( p.y() + tbt->height() > screen.y() + screen.height() )
			p.setY( screen.y() + screen.height() - tbt->height() );
		tbt->move( p += QPoint( -4, 46 ) );
		tbt->show();
	}
	QToolButton::enterEvent( _e );
}




void ToolButton::leaveEvent( QEvent * _e )
{
	if( checkForLeaveEvent() )
	{
		QToolButton::leaveEvent( _e );
	}
}




void ToolButton::mousePressEvent( QMouseEvent * _me )
{
	emit mouseLeftButton();
	QToolButton::mousePressEvent( _me );
}




void ToolButton::paintEvent( QPaintEvent * _pe )
{
	const bool active = isDown() || isChecked();
	const QColor ctbl[2][4] = {
				{
					QColor( 80, 160, 255 ),
					QColor( 32, 64, 192 ),
					QColor( 8, 16, 96 ),
					QColor( 0, 64, 224 )
				},
				{
					QColor( 255, 255, 64, m_colorizeLevel ),
					QColor( 192, 160, 32, m_colorizeLevel ),
					QColor( 96, 48, 0, m_colorizeLevel ),
					QColor( 192, 160, 0, m_colorizeLevel )
				}
				} ;

	QPainter p( this );
	p.fillRect( rect(), Qt::black );
	p.setRenderHint( QPainter::Antialiasing );
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, ctbl[0][0] );
	lingrad.setColorAt( 0.38, ctbl[0][1] );
	lingrad.setColorAt( 0.42, ctbl[0][2] );
	lingrad.setColorAt( 1, ctbl[0][3] );
	p.setBrush( lingrad );
	p.drawRoundRect( 1, 1, width()-2, height()-2, 1000 / width(),
							1000 / height() );

	if( m_colorizeLevel )
	{
		lingrad = QLinearGradient( 0, 0, 0, height() );
		lingrad.setColorAt( 0, ctbl[1][0] );
		lingrad.setColorAt( 0.38, ctbl[1][1] );
		lingrad.setColorAt( 0.42, ctbl[1][2] );
		lingrad.setColorAt( 1, ctbl[1][3] );
		p.setBrush( lingrad );
		p.drawRoundRect( 1, 1, width()-2, height()-2, 1000 / width(),
							1000 / height() );
	}
	p.setBrush( QBrush() );

	p.fillRect( rect(), QColor( 0, 0, 0, active ? 64 : 0 ) );

	QPen pen( QColor( 255, 255, 255, 96 ) );
	pen.setWidthF( 1.4f );
	p.setPen( pen );
	p.drawRoundRect( 0, 0, width()-1, height()-1, 1000 / width(),
							1000 / height() );
	QPen pen2 = pen;
	pen.setColor( QColor( 0, 0, 0, 128 ) );
	p.setPen( pen );
	p.drawRoundRect( 1, 1, width()-3, height()-3, 1000 / width(),
							1000 / height() );
	p.setPen( pen2 );
	p.drawRoundRect( 2, 2, width()-2, height()-2, 1000 / width(),
							1000 / height() );

	const int dd = isDown() ? 1 : 0;
	QPoint pt = QPoint( ( width() - m_img.width() ) / 2 + dd, 3 + dd );
	if( s_iconOnlyMode )
	{
		pt.setY( ( height() - m_img.height() ) / 2 - 1 + dd );
	}
	p.drawImage( pt, m_img );

	if( s_iconOnlyMode )
	{
		return;
	}
	const QString l = ( active && m_altLabel.isEmpty() == FALSE ) ?
							m_altLabel : m_label;
	const int w = p.fontMetrics().width( l );
	p.setPen( Qt::black );
	p.drawText( ( width() - w ) / 2 +1+dd, height() - 4+dd, l );
	p.setPen( Qt::white );
	p.drawText( ( width() - w ) / 2 +dd, height() - 5+dd, l );
}




bool ToolButton::checkForLeaveEvent( void )
{
	if( QRect( mapToGlobal( QPoint( 0, 0 ) ), size() ).
					contains( QCursor::pos() ) )
	{
		QTimer::singleShot( 20, this, SLOT( checkForLeaveEvent() ) );
	}
	else
	{
		emit mouseLeftButton();
		m_fadeBack = TRUE;
		if( m_colorizeLevel == 255 )
		{
			updateColorLevel();
		}
		return( TRUE );
	}
	return( FALSE );
}




void ToolButton::updateColorLevel( void )
{
	bool again;
	if( m_fadeBack )
	{
		m_colorizeLevel = qMax( 0, m_colorizeLevel - 10 );
		again = m_colorizeLevel > 0;
	}
	else
	{
		m_colorizeLevel = qMin( 255, m_colorizeLevel + 10 );
		again = m_colorizeLevel < 255;
	}
	update();
	if( again )
	{
		QTimer::singleShot( 10, this, SLOT( updateColorLevel() ) );
	}
}




void ToolButton::updateSize( void )
{
	QFont f = font();
	f.setPointSizeF( 7.5 );
	setFont( f );

	if( s_iconOnlyMode )
	{
		setFixedSize( 52, 48 );
	}
	else if( m_label.size() > 14 || m_altLabel.size() > 14 )
	{
		setFixedSize( 96, 48 );
	}
	else
	{
		setFixedSize( 88, 48 );
	}

	// set mask
	QBitmap b( size() );
	b.clear();

	QPainter p( &b );
	p.setBrush( Qt::color1 );
	p.setPen( Qt::color1 );
	p.drawRoundRect( 0, 0, width() - 1, height() - 1,
					1000 / width(), 1000 / height() );
	setMask( b );
}







ToolButtonTip::ToolButtonTip( const QPixmap & _pixmap, const QString & _title,
				const QString & _description,
				QWidget * _parent, QWidget * _tool_btn ) :
	QWidget( _parent, Qt::ToolTip ),
	m_icon( FastQImage( _pixmap ).scaled( 72, 72 ) ),
	m_title( _title ),
	m_description( _description ),
	m_toolButton( _tool_btn )
{
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
	setAttribute( Qt::WA_NoSystemBackground, TRUE );

	resize( sizeHint() );
	updateMask();
}




QSize ToolButtonTip::sizeHint( void ) const
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




void ToolButtonTip::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.drawImage( 0, 0, m_bg );
}




void ToolButtonTip::resizeEvent( QResizeEvent * _re )
{
	const QColor color_frame = QColor( 48, 48, 48 );
	m_bg = QImage( size(), QImage::Format_ARGB32 );
	m_bg.fill( color_frame.rgba() );
	QPainter p( &m_bg );
	p.setRenderHint( QPainter::Antialiasing );
	QPen pen( color_frame );
	pen.setWidthF( 1.5 );
	p.setPen( pen );
	QLinearGradient grad( 0, 0, 0, height() );
	const QColor color_top = palette().color( QPalette::Active,
						QPalette::Window ).light( 120 );
	grad.setColorAt( 0, color_top );
	grad.setColorAt( 1, palette().color( QPalette::Active,
						QPalette::Window ).
							light( 80 ) );
	p.setBrush( grad );
	p.drawRoundRect( 0, 0, width() - 1, height() - 1,
					ROUNDED / width(), ROUNDED / height() );
	if( m_toolButton )
	{
		QPoint pt = m_toolButton->mapToGlobal( QPoint( 0, 0 ) );
		p.setPen( color_top );
		p.setBrush( color_top );
		p.setRenderHint( QPainter::Antialiasing, FALSE );
		p.drawLine( pt.x() - x(), 0,
				pt.x() + m_toolButton->width() - x() - 2, 0 );
		const int dx = pt.x() - x();
		p.setRenderHint( QPainter::Antialiasing, TRUE );
		if( dx < 10 && dx >= 0 )
		{
			p.setPen( pen );
			p.drawImage( dx+1, 0, m_bg.copy( 20, 0, 10-dx, 10 ) );
			p.drawImage( dx, 0, m_bg.copy( 0, 10, 1, 10-dx*2 ) );
		}
	}
	p.setPen( Qt::black );

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

	updateMask();
	QWidget::resizeEvent( _re );
}




void ToolButtonTip::updateMask( void )
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

	if( m_toolButton )
	{
		QPoint pt = m_toolButton->mapToGlobal( QPoint( 0, 0 ) );
		const int dx = pt.x()-x();
		if( dx < 10 && dx >= 0 )
		{
			p.fillRect( dx, 0, 10, 10, Qt::color1 );
		}
	}

	setMask( b );
}


