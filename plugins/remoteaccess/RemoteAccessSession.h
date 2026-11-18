// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ComputerControlInterface.h"
#include <QPointer>
class QQmlApplicationEngine;
class QWindow;
class RemoteAccessSession;
class RemoteAccessWorkspace : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList sessions READ sessions NOTIFY changed)
	Q_PROPERTY(QObject* currentSession READ currentSession NOTIFY changed)
	Q_PROPERTY(int currentIndex READ currentIndex NOTIFY changed)
	Q_PROPERTY(QStringList qmlWarnings READ qmlWarnings NOTIFY qmlWarningsChanged)
public:
	explicit RemoteAccessWorkspace(QObject* parent = nullptr);
	~RemoteAccessWorkspace() override;
	void add(RemoteAccessSession*);
	void remove(RemoteAccessSession*);
	QVariantList sessions() const;
	QObject* currentSession() const;
	int currentIndex() const { return m_current; }
	QStringList qmlWarnings() const { return m_qmlWarnings; }
	Q_INVOKABLE void activate(int index);
	Q_INVOKABLE void detachCurrent();
	Q_INVOKABLE void closeAll(bool closeWindow = true);
Q_SIGNALS:
	void changed();
	void qmlWarningsChanged();
private:
	QStringList m_qmlWarnings;
	QList<RemoteAccessSession*> m_sessions;
	int m_current{0};
	QQmlApplicationEngine* m_engine{nullptr};
	QPointer<QWindow> m_window;
	bool m_closing{false};
};
class RemoteAccessSession : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QObject* computer READ computer CONSTANT)
	Q_PROPERTY(QString title READ title NOTIFY changed)
	Q_PROPERTY(QString status READ status NOTIFY changed)
	Q_PROPERTY(bool connected READ connected NOTIFY changed)
	Q_PROPERTY(bool viewOnly READ viewOnly WRITE setViewOnly NOTIFY changed)
	Q_PROPERTY(bool canToggle READ canToggle CONSTANT)
	Q_PROPERTY(QVariantList screens READ screens NOTIFY changed)
	Q_PROPERTY(QRect viewport READ viewport NOTIFY changed)
	Q_PROPERTY(int currentScreen READ currentScreen NOTIFY changed)
public:
	RemoteAccessSession(const ComputerControlInterface::Pointer&, bool viewOnly, bool canToggle, QObject* parent = nullptr);
	~RemoteAccessSession() override;
	QObject* computer() const { return m_computer.data(); }
	ComputerControlInterface::Pointer controlInterface() const { return m_computer; }
	QString title() const;
	QString status() const;
	bool connected() const;
	bool viewOnly() const { return m_viewOnly; }
	void setViewOnly(bool);
	bool canToggle() const { return m_canToggle; }
	QVariantList screens() const;
	QRect viewport() const;
	int currentScreen() const { return m_screen; }
	Q_INVOKABLE void selectScreen(int);
	Q_INVOKABLE void screenshot();
	Q_INVOKABLE void close();
	Q_INVOKABLE void showMaster();
Q_SIGNALS:
	void changed();
private:
	friend class RemoteAccessWorkspace;
	ComputerControlInterface::Pointer m_computer;
	QPointer<RemoteAccessWorkspace> m_workspace;
	bool m_viewOnly;
	bool m_canToggle;
	int m_screen{-1};
};
