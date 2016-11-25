/*
 * PageAccessControl.h - header for the PageAccessControl class
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef PAGE_ACCESS_CONTROL_H
#define PAGE_ACCESS_CONTROL_H

#include "ConfigurationPage.h"

namespace Ui {
class PageAccessControl;
}

class PageAccessControl : public ConfigurationPage
{
	Q_OBJECT

public:
	PageAccessControl();
	~PageAccessControl();

	virtual void resetWidgets();
	virtual void connectWidgetsToProperties();

private slots:
	void addGroup();
	void removeGroup();
	void addAuthorizationRule();
	void removeAuthorizationRule();
	void editAuthorizationRule();
	void moveAuthorizationRuleDown();
	void moveAuthorizationRuleUp();

private:
	Ui::PageAccessControl *ui;
};

#endif // PAGE_ACCESS_CONTROL_H
