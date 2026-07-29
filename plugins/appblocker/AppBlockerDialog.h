#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>

class AppBlockerDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AppBlockerDialog(QWidget* parent = nullptr);
	~AppBlockerDialog() override = default;

	QStringList getBlockedApps() const;
	bool getWhitelistMode() const;

private Q_SLOTS:
	void addApp();
	void removeApp();
	void applySettings();

private:
	void loadSettings();

	QListWidget* m_listWidget;
	QLineEdit* m_appInput;
	QPushButton* m_addButton;
	QPushButton* m_removeButton;
	QPushButton* m_applyButton;
	class QCheckBox* m_whitelistCheckbox;
};
