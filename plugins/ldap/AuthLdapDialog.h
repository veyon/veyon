// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

#include <QDialog>

class LdapConfiguration;

namespace Ui { class AuthLdapDialog; }

class AuthLdapDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AuthLdapDialog( const LdapConfiguration& config, QWidget *parent );
	~AuthLdapDialog() override;

	QString username() const;
	CryptoCore::PlaintextPassword password() const;

	void accept() override;

private Q_SLOTS:
	void updateOkButton();

private:
	Ui::AuthLdapDialog *ui;
	const LdapConfiguration& m_config;

} ;
