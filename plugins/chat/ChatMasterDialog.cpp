#include "ChatMasterDialog.h"
#include "ui_ChatMasterDialog.h"
#include "ChatPlugin.h"
#include <QDateTime>

ChatMasterDialog::ChatMasterDialog(ChatPlugin* plugin, const ComputerControlInterfaceList& interfaces, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ChatMasterDialog),
    m_plugin(plugin),
    m_interfaces(interfaces)
{
    ui->setupUi(this);
    connect(ui->sendButton, &QPushButton::clicked, this, &ChatMasterDialog::onSendClicked);
    connect(ui->messageInput, &QLineEdit::returnPressed, this, &ChatMasterDialog::onSendClicked);
}

ChatMasterDialog::~ChatMasterDialog()
{
    delete ui;
}

void ChatMasterDialog::receiveMessage(const QString& from, const QString& msg)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->chatBrowser->append(QStringLiteral("<b>[%1] %2:</b> %3").arg(time, from, msg));
}

void ChatMasterDialog::onSendClicked()
{
    QString msg = ui->messageInput->text().trimmed();
    if (msg.isEmpty()) return;

    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->chatBrowser->append(QStringLiteral("<b>[%1] Öğretmen:</b> %2").arg(time, msg));
    
    m_plugin->sendMasterMessage(msg, m_interfaces);
    ui->messageInput->clear();
}
