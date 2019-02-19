/*
 * ToolButton.cpp - implementation of Veyon-tool-button
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QTimer>
#include <QAction>
#include <QApplication>
#include <QBitmap>
#include <QDesktopWidget>
#include <QLabel>
#include <QLayout>
#include <QLinearGradient>
#include <QPainter>
#include <QScreen>
#include <QToolBar>

#include "ToolButton.h"

bool ToolButton::s_toolTipsDisabled = false;
bool ToolButton::s_iconOnlyMode = false;


ToolButton::ToolButton( const QIcon& icon,
						const QString& label,
						const QString& altLabel,
						const QString& description,
						const QKeySequence& shortcut ) :
	m_pixelRatio( 1 ),
	m_icon( icon ),
	m_pixmap(),
	m_mouseOver( false ),
	m_label( label ),
	m_altLabel( altLabel ),
	m_descr( description )
{
	setShortcut( shortcut );

	setAttribute( Qt::WA_NoSystemBackground, true );

	updateSize();
}



void ToolButton::setIconOnlyMode( QWidget* mainWindow, bool enabled )
{
	s_iconOnlyMode = enabled;
	const auto toolButtons = mainWindow->findChildren<ToolButton *>();
	for( auto toolButton : toolButtons )
	{
		toolButton->updateSize();
	}
}



void ToolButton::addTo( QToolBar* toolBar )
{
	auto action = toolBar->addWidget( this );
	action->setText( m_label );
}




void ToolButton::enterEvent( QEvent* event )
{
	m_mouseOver = true;
	if( !s_toolTipsDisabled && !m_label.isEmpty() && !m_descr.isEmpty() )
	{
		auto toolTipPos = mapToGlobal( QPoint( 0, 0 ) );
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		const auto screenRect = QGuiApplication::screenAt( toolTipPos )->availableGeometry();
#else
		int screenNumber = QApplication::desktop()->isVirtualDesktop() ?
					QApplication::desktop()->screenNumber( toolTipPos ) :
					QApplication::desktop()->screenNumber( this );
		const auto screenRect = QApplication::desktop()->screenGeometry( screenNumber );
#endif

		auto toolTip = new ToolButtonTip( m_icon.pixmap( 128, 128 ), m_label, m_descr, nullptr, this );
		connect( this, &ToolButton::mouseLeftButton, toolTip, &QWidget::close );

		if( toolTipPos.x() + toolTip->width() > screenRect.x() + screenRect.width() )
		{
			toolTipPos.rx() -= 4;
		}
		if( toolTipPos.y() + toolTip->height() > screenRect.y() + screenRect.height() )
		{
			toolTipPos.ry() -= 30 + toolTip->height();
		}
		if( toolTipPos.y() < screenRect.y() )
		{
			toolTipPos.setY( screenRect.y() );
		}
		if( toolTipPos.x() + toolTip->width() > screenRect.x() + screenRect.width() )
		{
			toolTipPos.setX( screenRect.x() + screenRect.width() - toolTip->width() );
		}
		if( toolTipPos.x() < screenRect.x() )
		{
			toolTipPos.setX( screenRect.x() );
		}
		if( toolTipPos.y() + toolTip->height() > screenRect.y() + screenRect.height() )
		{
			toolTipPos.setY( screenRect.y() + screenRect.height() - toolTip->height() );
		}

		toolTip->move( toolTipPos += QPoint( -4, height() ) );
		toolTip->show();
	}

	QToolButton::enterEvent( event );
}




void ToolButton::leaveEvent( QEvent* event )
{
	if( checkForLeaveEvent() )
	{
		QToolButton::leaveEvent( event );
	}
}




void ToolButton::mousePressEvent( QMouseEvent* event )
{
	emit mouseLeftButton();
	QToolButton::mousePressEvent( event );
}




void ToolButton::paintEvent( QPaintEvent* )
{
	const bool active = isDown() || isChecked();

	QPainter painter(this);
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

	if( active )
	{
		outlinebrush.setColorAt( 0.0, shadow );
		outlinebrush.setColorAt( 1.0, highlight );
		brush.setColorAt( 0.0, sunken );
		painter.setPen(Qt::NoPen);
	}
	else
	{
		outlinebrush.setColorAt( 1.0, shadow );
		outlinebrush.setColorAt( 0.0, highlight );
		brush.setColorAt( 0.0, normal1 );
		if( m_mouseOver == false )
		{
			brush.setColorAt( 1.0, normal2 );
		}
		painter.setPen(QPen(outlinebrush, 1));
	}

	painter.setBrush(brush);

	painter.drawRoundedRect( rect(), roundness(), roundness() );

	const int delta = active ? 1 : 0;
	QPoint pixmapPos( ( width() - m_pixmap.width() ) / 2 + delta, margin() / 2 + delta );
	if( s_iconOnlyMode )
	{
		pixmapPos.setY( ( height() - m_pixmap.height() ) / 2 - 1 + delta );
	}
	painter.drawPixmap( pixmapPos, m_pixmap );

	if( s_iconOnlyMode == false )
	{
		const auto label = ( active && m_altLabel.isEmpty() == false ) ? m_altLabel : m_label;
		const int labelX = 1 + ( width() - painter.fontMetrics().width( label ) ) / 2;
		const int deltaNormal = delta - 1;
		const int deltaShadow = deltaNormal + 1;

		painter.setPen( Qt::black );
		painter.drawText( labelX + deltaShadow, height() - margin() / 2 + deltaShadow, label );

		painter.setPen( Qt::white );
		painter.drawText( labelX + deltaNormal, height() - margin() / 2 + deltaNormal, label );
	}
}




bool ToolButton::checkForLeaveEvent()
{
	if( QRect( mapToGlobal( QPoint( 0, 0 ) ), size() ).
			contains( QCursor::pos() ) )
	{
		QTimer::singleShot( 20, this, &ToolButton::checkForLeaveEvent );
	}
	else
	{
		emit mouseLeftButton();
		m_mouseOver = false;

		return true;
	}
	return false;
}




void ToolButton::updateSize()
{
	auto f = QApplication::font();
	f.setPointSizeF( qMax( 7.5, f.pointSizeF() * 0.9 ) );
	setFont( f );

	m_pixelRatio = fontInfo().pixelSize() / fontInfo().pointSizeF();

	const auto metrics = fontMetrics();

	m_pixmap = m_icon.pixmap( static_cast<int>( iconSize() ) );

	if( s_iconOnlyMode )
	{
		setFixedSize( margin() + iconSize(), margin() + iconSize() );
	}
	else
	{
		const int textWidth = ( qMax( metrics.width( m_label ), metrics.width( m_altLabel ) ) / stepSize() + 1 ) * stepSize();
		const int width = qMax( textWidth, iconSize() * 3 / 2 );
		const int height = iconSize() + fontInfo().pixelSize();
		setFixedSize( width + margin(), height + margin() );
	}
}







ToolButtonTip::ToolButtonTip( const QIcon& icon, const QString &title,
							  const QString & _description,
							  QWidget * _parent, QWidget * _tool_btn ) :
	QWidget( _parent, Qt::ToolTip ),
	m_pixelRatio( fontInfo().pixelSize() / fontInfo().pointSizeF() ),
	m_pixmap( icon.pixmap( static_cast<int>( 64 * m_pixelRatio ) ) ),
	m_title( title ),
	m_description( _description ),
	m_toolButton( _tool_btn )
{
	setAttribute( Qt::WA_DeleteOnClose, true );
	setAttribute( Qt::WA_NoSystemBackground, true );

	QTimer::singleShot( 0, this, [this]() { resize( sizeHint() ); } );

	updateMask();
}




QSize ToolButtonTip::sizeHint() const
{
	auto f = font();
	f.setBold( true );

	const auto titleWidth = QFontMetrics( f ).width( m_title );
	const auto descriptionRect = fontMetrics().boundingRect( QRect( 0, 0, 250, 100 ), Qt::TextWordWrap, m_description );

	return { margin() + m_pixmap.width() + margin() + qMax( titleWidth, descriptionRect.width() ) + margin(),
				margin() + qMax( m_pixmap.height(), fontMetrics().height() + margin() + descriptionRect.height() ) + margin() };
}




void ToolButtonTip::paintEvent( QPaintEvent* )
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
		p.setRenderHint( QPainter::Antialiasing, false );
		p.drawLine( pt.x() - x(), 0,
					pt.x() + m_toolButton->width() - x() - 2, 0 );
		const int dx = pt.x() - x();
		p.setRenderHint( QPainter::Antialiasing, true );
		if( dx < 10 && dx >= 0 )
		{
			p.setPen( pen );
			p.drawImage( dx+1, 0, m_bg.copy( 20, 0, 10-dx, 10 ) );
			p.drawImage( dx, 0, m_bg.copy( 0, 10, 1, 10-dx*2 ) );
		}
	}
	p.setPen( Qt::black );

	p.drawPixmap( margin(), margin(), m_pixmap );
	QFont f = p.font();
	f.setBold( true );
	p.setFont( f );
	const auto titleX = margin() + m_pixmap.width() + margin();
	const auto titleY = margin() + fontMetrics().height() - 2;
	p.drawText( titleX, titleY, m_title );

	f.setBold( false );
	p.setFont( f );
	p.drawText( QRect( titleX,
					   titleY + margin(),
					   width() - margin() - titleX,
					   height() - margin() - titleY ),
				Qt::TextWordWrap, m_description );

	updateMask();
	QWidget::resizeEvent( _re );
}




void ToolButtonTip::updateMask()
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
