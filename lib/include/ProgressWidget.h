/*
 *  ProgressWidget.h - widget with animated progress-indicator
 *
 *  Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 *  This file is part of iTALC - http://italc.sourceforge.net
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */


#ifndef _PROGRESS_WIDGET_H
#define _PROGRESS_WIDGET_H

#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>


class ProgressWidget : public QWidget
{
	Q_OBJECT
public:
	ProgressWidget( const QString & _txt,
			const QString & _anim, int _frames,
			QWidget * _parent = 0 );
	virtual ~ProgressWidget();


private slots:
	void nextAnim( void );


private:
	virtual void paintEvent( QPaintEvent * );

	QString m_txt;
	QString m_anim;
	int m_frames;
	int m_curFrame;

	QVector<QPixmap> m_pixmaps;

} ;


#endif

