#pragma once

#include <QDialog>
#include "ComputerControlInterface.h"

namespace Ui { class ChatMasterDialog; }
class ChatPlugin;

class ChatMasterDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChatMasterDialog(ChatPlugin* plugin, const ComputerControlInterfaceList& interfaces, QWidget* parent = nullptr);
    ~ChatMasterDialog() override;

    void receiveMessage(const QString& from, const QString& msg);

private Q_SLOTS:
    void onSendClicked();

private:
    Ui::ChatMasterDialog* ui;
    ChatPlugin* m_plugin;
    ComputerControlInterfaceList m_interfaces;
};
