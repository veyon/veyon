#pragma once

#include <QDialog>
#include "ComputerControlInterface.h"
#include "FeatureMessage.h"

namespace Ui { class SurveyMasterDialog; }

class SurveyMasterDialog : public QDialog
{
	Q_OBJECT
public:
	explicit SurveyMasterDialog(QWidget* parent = nullptr);
	~SurveyMasterDialog() override;

	void setComputers(const ComputerControlInterfaceList& computers);
	void addAnswer(const QString& computerName, const QString& answer);

private Q_SLOTS:
	void onSendClicked();
	void onStopClicked();
	void onTypeChanged(int index);
	void onAddOptionClicked();
	void onRemoveOptionClicked();

private:
	Ui::SurveyMasterDialog* ui;
	ComputerControlInterfaceList m_computers;
};
