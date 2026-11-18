// SPDX-License-Identifier: GPL-2.0-or-later
#include "MainWindow.h"
#include "ComputerControlListModel.h"
#include "VeyonConfiguration.h"
MasterComputerModel::MasterComputerModel(MainWindow& controller, QObject* parent) : QIdentityProxyModel(parent), m_controller(controller) {}
QHash<int,QByteArray> MasterComputerModel::roleNames() const
{
	return {{ComputerName,"computerName"},{UserName,"userName"},{Address,"address"},{Online,"online"},{StateText,"stateText"},{Thumbnail,"thumbnail"},{ComputerObject,"computerObject"},{Selected,"selected"},{ComputerUid,"computerUid"},{PositionX,"positionX"},{PositionY,"positionY"}};
}
QVariant MasterComputerModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) return role == Thumbnail ? QVariant::fromValue(QImage{}) : QVariant{};
	const auto computer = QIdentityProxyModel::data(index, ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
	if (!computer) return role == Thumbnail ? QVariant::fromValue(QImage{}) : QVariant{};
	const auto id = QIdentityProxyModel::data(index, ComputerControlListModel::UidRole).toUuid().toString();
	switch (role) {
	case ComputerName: return computer->computerName();
	case UserName: return computer->userFullName().isEmpty() ? computer->userLoginName() : computer->userFullName();
	case Address: return computer->computer().hostName();
	case Online: return computer->state() == ComputerControlInterface::State::Connected;
	case StateText:
		switch (computer->state()) {
		case ComputerControlInterface::State::Connected: return tr("Online");
		case ComputerControlInterface::State::Connecting: return tr("Connecting");
		case ComputerControlInterface::State::HostNameResolutionFailed: return tr("DNS error");
		case ComputerControlInterface::State::AuthenticationFailed: return tr("Authentication failed");
		case ComputerControlInterface::State::AccessControlFailed: return computer->accessControlDetails();
		case ComputerControlInterface::State::ServerNotRunning: return tr("Service unavailable");
		default: return tr("Offline");
		}
	case Thumbnail: return QVariant::fromValue(QIdentityProxyModel::data(index, Qt::DecorationRole).value<QImage>());
	case ComputerObject: return QVariant::fromValue<QObject*>(computer.data());
	case Selected: return m_controller.isSelected(id);
	case ComputerUid: return id;
	case PositionX: return m_controller.position(id, index.row()).x();
	case PositionY: return m_controller.position(id, index.row()).y();
	default: return QIdentityProxyModel::data(index, role);
	}
}
void MasterComputerModel::refreshSelection() { if (rowCount()) Q_EMIT dataChanged(index(0,0),index(rowCount()-1,0)); }
