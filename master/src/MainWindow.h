// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QIdentityProxyModel>
#include <QPointer>
#include <QPointF>
#include <QSizeF>
#include <QRegularExpression>
#include <QJsonArray>
#include <QSet>
#include <QVariantList>
#include <QTimer>
#include "ComputerControlInterface.h"
class VeyonMaster;
class QQmlApplicationEngine;
class QWindow;
class MainWindow;
class MasterComputerModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	enum Roles { ComputerName = Qt::UserRole + 100, UserName, Address, Online, StateText, Thumbnail, ComputerObject, Selected, ComputerUid, PositionX, PositionY };
	MasterComputerModel(MainWindow& controller, QObject* parent);
	QHash<int,QByteArray> roleNames() const override;
	QVariant data(const QModelIndex&, int role) const override;
	void refreshSelection();
private:
	MainWindow& m_controller;
};
class MainWindow : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QAbstractItemModel* computers READ computers CONSTANT)
	Q_PROPERTY(bool shortcutContextActive READ shortcutContextActive NOTIFY shortcutContextActiveChanged)
	Q_PROPERTY(QVariantList rooms READ rooms NOTIFY roomsChanged)
	Q_PROPERTY(QString currentRoom READ currentRoom NOTIFY roomsChanged)
	Q_PROPERTY(bool canAddRooms READ canAddRooms CONSTANT)
	Q_PROPERTY(bool expandRooms READ expandRooms CONSTANT)
	Q_PROPERTY(QVariantList features READ features NOTIFY featuresChanged)
	Q_PROPERTY(int selectedCount READ selectedCount NOTIFY changed)
	Q_PROPERTY(int totalCount READ totalCount NOTIFY changed)
	Q_PROPERTY(int visibleCount READ visibleCount NOTIFY changed)
	Q_PROPERTY(int filterIndex READ filterIndex NOTIFY changed)
	Q_PROPERTY(int onlineCount READ onlineCount NOTIFY changed)
	Q_PROPERTY(QVariantMap details READ details NOTIFY changed)
	Q_PROPERTY(bool selectedScope READ selectedScope WRITE setSelectedScope NOTIFY changed)
	Q_PROPERTY(bool customPositions READ customPositions WRITE setCustomPositions NOTIFY changed)
	Q_PROPERTY(int cardWidth READ cardWidth WRITE setCardWidth NOTIFY changed)
	Q_PROPERTY(bool autoFit READ autoFit WRITE setAutoFit NOTIFY changed)
	Q_PROPERTY(QVariantList screenshots READ screenshots NOTIFY screenshotsChanged)
	Q_PROPERTY(QVariantList spotlight READ spotlight NOTIFY spotlightChanged)
	Q_PROPERTY(bool spotlightRealtime READ spotlightRealtime WRITE setSpotlightRealtime NOTIFY spotlightChanged)
	Q_PROPERTY(QObject* slideshowComputer READ slideshowComputer NOTIFY slideshowChanged)
	Q_PROPERTY(bool slideshowRunning READ slideshowRunning WRITE setSlideshowRunning NOTIFY slideshowChanged)
	Q_PROPERTY(int slideshowDuration READ slideshowDuration WRITE setSlideshowDuration NOTIFY slideshowChanged)
	Q_PROPERTY(QString activeSummary READ activeSummary NOTIFY changed)
public:
	explicit MainWindow(VeyonMaster&);
	~MainWindow() override;
	static bool initAuthentication();
	static bool initAccessControl();
	QWindow* window() const;
	bool shortcutContextActive() const;
	QStringList qmlWarnings() const { return m_qmlWarnings; }
	void show();
	void reloadSubFeatures();
	ComputerControlInterfaceList selectedComputerControlInterfaces() const;
	QAbstractItemModel* computers() const;
	QVariantList rooms() const { return m_rooms; }
	QString currentRoom() const { return m_currentRoom; }
	QVariantList features() const;
	int selectedCount() const { return m_selected.size(); }
	int totalCount() const;
	int visibleCount() const { return m_model->rowCount(); }
	int filterIndex() const;
	int onlineCount() const;
	QVariantMap details() const;
	bool selectedScope() const { return m_selectedScope; }
	void setSelectedScope(bool);
	bool customPositions() const;
	void setCustomPositions(bool);
	int cardWidth() const;
	void setCardWidth(int);
	bool autoFit() const;
	void setAutoFit(bool);
	Q_INVOKABLE void fitCards(int width, int height);
	bool isSelected(const QString& uid) const { return m_selected.contains(uid); }
	QPoint position(const QString& uid, int row) const;
	QVariantList screenshots() const;
	QVariantList spotlight() const;
	bool spotlightRealtime() const;
	void setSpotlightRealtime(bool);
	QObject* slideshowComputer() const;
	bool slideshowRunning() const { return m_slideshowTimer.isActive(); }
	void setSlideshowRunning(bool);
	int slideshowDuration() const;
	void setSlideshowDuration(int);
	QString activeSummary() const;
	Q_INVOKABLE void select(const QString& uid, bool additive = false, bool range = false);
	Q_INVOKABLE void selectAll(bool selected);
	Q_INVOKABLE QString computerIdAt(int row) const;
	Q_INVOKABLE void activateRoom(const QString& uid);
	Q_INVOKABLE void setRoomChecked(const QString& uid, bool checked);
	Q_INVOKABLE void setComputerChecked(const QString& uid, bool checked);
	Q_INVOKABLE void addRoom();
	Q_INVOKABLE void removeRoom();
	Q_INVOKABLE void reloadComputers();
	bool canAddRooms() const;
	bool expandRooms() const;
	Q_INVOKABLE void setSearch(const QString&);
	Q_INVOKABLE void setFilter(int);
	Q_INVOKABLE void trigger(const QString& featureId, bool selectedOnly = false);
	Q_INVOKABLE int captureTargets(bool selectedOnly = false);
	Q_INVOKABLE void releaseTargets(int token);
	Q_INVOKABLE void triggerSnapshot(const QString& featureId, int token);
	void runFeatureOnComputers(const Feature&, const ComputerControlInterfaceList&, const ComputerControlInterfaceList& sources = {}, bool toggle = true);
	Q_INVOKABLE void doubleClick(const QString& uid);
	Q_INVOKABLE void removePreset(const QString& uid);
	Q_INVOKABLE void stopFeatures();
	Q_INVOKABLE void setPosition(const QString& uid, int x, int y);
	Q_INVOKABLE void alignPositions();
	Q_INVOKABLE void positionsFile(bool save);
	Q_INVOKABLE void exportComputers();
	Q_INVOKABLE void deleteScreenshots(const QStringList& names);
	Q_INVOKABLE void addSpotlight();
	Q_INVOKABLE void removeSpotlight(int index);
	Q_INVOKABLE void nextSlide(int delta);
	Q_INVOKABLE bool prepareClose();
	Q_INVOKABLE void about();
	Q_INVOKABLE void documentation();
	Q_INVOKABLE QVariantMap feature(const QString& name) const;
Q_SIGNALS:
	void shortcutContextActiveChanged();
	void changed();
	void qmlWarning(const QString& warning);
	void roomsChanged();
	void featuresChanged();
	void screenshotsChanged();
	void spotlightChanged();
	void slideshowChanged();
private:
	void rebuildRooms();
	void refresh();
	void savePositions();
	bool decodePositions(const QJsonArray&, const QString& format, QHash<QString,QPointF>& positions) const;
	QSizeF positionGridSize() const;
	ComputerControlInterface::Pointer slideshowTarget(int row) const;
	QString uid(const ComputerControlInterface::Pointer&) const;
	ComputerControlInterfaceList actionTargets(bool selectedOnly) const;
	VeyonMaster& m_master;
	MasterComputerModel* m_model;
	QQmlApplicationEngine* m_engine;
	QPointer<QWindow> m_window;
	QStringList m_qmlWarnings;
	QVariantList m_rooms;
	QString m_currentRoom;
	QString m_currentRoomUid;
	QRegularExpression m_directorySearch;
	QSet<QString> m_selected;
	QString m_anchor;
	bool m_selectedScope{false};
	bool m_dispatching{false};
	bool m_rebuildingRooms{false};
	ComputerControlInterfaceList m_dispatchSelection;
	ComputerControlInterfaceList m_spotlight;
	QHash<QString,QPointF> m_positions;
	bool m_positionsLoaded{false};
	bool m_positionsFormatKnown{false};
	struct Invocation { ComputerControlInterfaceList targets; ComputerControlInterfaceList sources; };
	QHash<int, Invocation> m_invocations;
	int m_nextInvocation{0};
	QTimer m_slideshowTimer;
	int m_slide{-1};
};
