#pragma once

#include <QDialog>

namespace Ui { class ChatStudentDialog; }
class ChatPlugin;

class ChatStudentDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChatStudentDialog(ChatPlugin* plugin, QWidget* parent = nullptr);
    ~ChatStudentDialog() override;

    void receiveMessage(const QString& msg);

private Q_SLOTS:
    void onSendClicked();

private:
    Ui::ChatStudentDialog* ui;
    ChatPlugin* m_plugin;
};
