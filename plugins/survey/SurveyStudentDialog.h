#pragma once

#include <QDialog>
#include <QPointer>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include "FeatureMessage.h"

class VeyonWorkerInterface;

namespace Ui { class SurveyStudentDialog; }

class SurveyStudentDialog : public QDialog
{
	Q_OBJECT
public:
	explicit SurveyStudentDialog(const FeatureMessage& message, VeyonWorkerInterface* worker, QWidget* parent = nullptr);
	~SurveyStudentDialog() override;

	static void showDialog(const FeatureMessage& message, VeyonWorkerInterface* worker);
	static void closeDialog();

private Q_SLOTS:
	void onSubmitClicked();

private:
	Ui::SurveyStudentDialog* ui;
	FeatureMessage m_message;
	VeyonWorkerInterface* m_worker;

	static QPointer<SurveyStudentDialog> s_instance;

	int m_questionType;
	QList<QRadioButton*> m_radioButtons;
	QList<QCheckBox*> m_checkBoxes;
	QLineEdit* m_shortTextEdit = nullptr;
	QTextEdit* m_longTextEdit = nullptr;
};
