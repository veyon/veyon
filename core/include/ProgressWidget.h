/*
 *  ProgressWidget.h - widget with animated progress indicator
 *
 *  Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
 *
 *  This file is part of Veyon - http://veyon.io
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

#ifndef PROGRESS_WIDGET_H
#define PROGRESS_WIDGET_H

#include <QPixmap>
#include <QVector>
#include <QWidget>

class ProgressWidget : public QWidget
{
	Q_OBJECT
public:
	ProgressWidget( const QString& text, const QString& animationPixmapBase, int frames, QWidget* parent = nullptr );
	~ProgressWidget() override;

protected:
	void paintEvent( QPaintEvent* event ) override;

private slots:
	void nextFrame();

private:
	QString m_text;
	int m_frames;
	int m_curFrame;

	QVector<QPixmap> m_pixmaps;

} ;

#endif
