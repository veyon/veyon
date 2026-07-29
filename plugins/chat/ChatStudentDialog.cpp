#include "ChatStudentDialog.h"
#include "ui_ChatStudentDialog.h"
#include "ChatPlugin.h"
#include <QDateTime>

ChatStudentDialog::ChatStudentDialog(ChatPlugin* plugin, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ChatStudentDialog),
    m_plugin(plugin)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
    connect(ui->sendButton, &QPushButton::clicked, this, &ChatStudentDialog::onSendClicked);
    connect(ui->messageInput, &QLineEdit::returnPressed, this, &ChatStudentDialog::onSendClicked);
}

ChatStudentDialog::~ChatStudentDialog()
{
    delete ui;
}

void ChatStudentDialog::receiveMessage(const QString& msg)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->chatBrowser->append(QStringLiteral("<b>[%1] Öğretmen:</b> %2").arg(time, msg));
    if (!isVisible()) {
        show();
    }
    activateWindow();
}

void ChatStudentDialog::onSendClicked()
{
    QString msg = ui->messageInput->text().trimmed();
    if (msg.isEmpty()) return;

    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->chatBrowser->append(QStringLiteral("<b>[%1] Sen:</b> %2").arg(time, msg));
    
    m_plugin->sendStudentMessage(msg);
    ui->messageInput->clear();
}
