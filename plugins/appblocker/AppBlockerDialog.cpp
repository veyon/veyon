#include "AppBlockerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QLabel>
#include <QMessageBox>
#include <QCheckBox>

AppBlockerDialog::AppBlockerDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Kara Liste Yönetimi (Uygulama Engelleyici)"));
	resize(400, 350);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	m_whitelistCheckbox = new QCheckBox(tr("Sadece sistem ve program klasörlerindeki uygulamalara izin ver\n(Taşınabilir/İndirilen uygulamaları engelle)"), this);
	mainLayout->addWidget(m_whitelistCheckbox);

	mainLayout->addWidget(new QLabel(tr("Engellenen Uygulamalar (örn: minecraft.exe):")));

	m_listWidget = new QListWidget(this);
	mainLayout->addWidget(m_listWidget);

	QHBoxLayout* inputLayout = new QHBoxLayout();
	m_appInput = new QLineEdit(this);
	m_appInput->setPlaceholderText(tr("Uygulama adı..."));
	m_addButton = new QPushButton(tr("Ekle"), this);
	
	inputLayout->addWidget(m_appInput);
	inputLayout->addWidget(m_addButton);
	mainLayout->addLayout(inputLayout);

	QHBoxLayout* actionLayout = new QHBoxLayout();
	m_removeButton = new QPushButton(tr("Seçileni Kaldır"), this);
	m_applyButton = new QPushButton(tr("Uygula ve Kapat"), this);
	
	actionLayout->addWidget(m_removeButton);
	actionLayout->addStretch();
	actionLayout->addWidget(m_applyButton);
	mainLayout->addLayout(actionLayout);

	connect(m_addButton, &QPushButton::clicked, this, &AppBlockerDialog::addApp);
	connect(m_removeButton, &QPushButton::clicked, this, &AppBlockerDialog::removeApp);
	connect(m_applyButton, &QPushButton::clicked, this, &AppBlockerDialog::applySettings);
	connect(m_appInput, &QLineEdit::returnPressed, this, &AppBlockerDialog::addApp);

	loadSettings();
}

void AppBlockerDialog::loadSettings()
{
	QSettings settings("VeyonCommunity", "AppBlocker");
	QStringList apps = settings.value("Blacklist").toStringList();
	m_listWidget->addItems(apps);
	m_whitelistCheckbox->setChecked(settings.value("WhitelistMode", false).toBool());
}

void AppBlockerDialog::addApp()
{
	QString app = m_appInput->text().trimmed();
	if (!app.isEmpty()) {
		// Check for duplicate
		QList<QListWidgetItem*> items = m_listWidget->findItems(app, Qt::MatchExactly);
		if (items.isEmpty()) {
			m_listWidget->addItem(app);
			m_appInput->clear();
		} else {
			QMessageBox::warning(this, tr("Uyarı"), tr("Bu uygulama zaten listede!"));
		}
	}
}

void AppBlockerDialog::removeApp()
{
	QList<QListWidgetItem*> items = m_listWidget->selectedItems();
	for (QListWidgetItem* item : items) {
		delete m_listWidget->takeItem(m_listWidget->row(item));
	}
}

void AppBlockerDialog::applySettings()
{
	QStringList apps = getBlockedApps();
	QSettings settings("VeyonCommunity", "AppBlocker");
	settings.setValue("Blacklist", apps);
	settings.setValue("WhitelistMode", m_whitelistCheckbox->isChecked());
	accept();
}

QStringList AppBlockerDialog::getBlockedApps() const
{
	QStringList apps;
	for (int i = 0; i < m_listWidget->count(); ++i) {
		apps.append(m_listWidget->item(i)->text());
	}
	return apps;
}

bool AppBlockerDialog::getWhitelistMode() const
{
	return m_whitelistCheckbox->isChecked();
}
