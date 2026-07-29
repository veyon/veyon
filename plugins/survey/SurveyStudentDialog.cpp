#include "SurveyStudentDialog.h"
#include "ui_SurveyStudentDialog.h"
#include "SurveyPlugin.h"
#include "VeyonWorkerInterface.h"
#include <QMessageBox>

QPointer<SurveyStudentDialog> SurveyStudentDialog::s_instance = nullptr;

SurveyStudentDialog::SurveyStudentDialog(const FeatureMessage& message, VeyonWorkerInterface* worker, QWidget* parent) :
	QDialog(parent, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint),
	ui(new Ui::SurveyStudentDialog),
	m_message(message),
	m_worker(worker)
{
	ui->setupUi(this);

	m_questionType = message.argument(SurveyArgument::QuestionType).toInt();
	QString question = message.argument(SurveyArgument::Question).toString();
	QStringList options = message.argument(SurveyArgument::Options).toStringList();

	ui->questionLabel->setText(question);

	QuestionType qt = static_cast<QuestionType>(m_questionType);

	if (qt == QuestionType::SingleChoice)
	{
		for (const QString& opt : options)
		{
			QRadioButton* rb = new QRadioButton(opt, ui->scrollAreaWidgetContents);
			ui->contentLayout->addWidget(rb);
			m_radioButtons.append(rb);
		}
	}
	else if (qt == QuestionType::MultipleChoice)
	{
		for (const QString& opt : options)
		{
			QCheckBox* cb = new QCheckBox(opt, ui->scrollAreaWidgetContents);
			ui->contentLayout->addWidget(cb);
			m_checkBoxes.append(cb);
		}
	}
	else if (qt == QuestionType::ShortText)
	{
		m_shortTextEdit = new QLineEdit(ui->scrollAreaWidgetContents);
		ui->contentLayout->addWidget(m_shortTextEdit);
	}
	else if (qt == QuestionType::LongText)
	{
		m_longTextEdit = new QTextEdit(ui->scrollAreaWidgetContents);
		ui->contentLayout->addWidget(m_longTextEdit);
	}
	else if (qt == QuestionType::TrueFalse)
	{
		QRadioButton* rb1 = new QRadioButton(tr("True"), ui->scrollAreaWidgetContents);
		QRadioButton* rb2 = new QRadioButton(tr("False"), ui->scrollAreaWidgetContents);
		ui->contentLayout->addWidget(rb1);
		ui->contentLayout->addWidget(rb2);
		m_radioButtons.append(rb1);
		m_radioButtons.append(rb2);
	}

	ui->contentLayout->addStretch();

	connect(ui->submitButton, &QPushButton::clicked, this, &SurveyStudentDialog::onSubmitClicked);
}

SurveyStudentDialog::~SurveyStudentDialog()
{
	delete ui;
}

void SurveyStudentDialog::showDialog(const FeatureMessage& message, VeyonWorkerInterface* worker)
{
	closeDialog();
	s_instance = new SurveyStudentDialog(message, worker);
	s_instance->show();
	s_instance->raise();
	s_instance->activateWindow();
}

void SurveyStudentDialog::closeDialog()
{
	if (s_instance)
	{
		s_instance->deleteLater();
	}
}

void SurveyStudentDialog::onSubmitClicked()
{
	QString answer;
	QuestionType qt = static_cast<QuestionType>(m_questionType);

	if (qt == QuestionType::SingleChoice || qt == QuestionType::TrueFalse)
	{
		for (QRadioButton* rb : m_radioButtons)
		{
			if (rb->isChecked())
			{
				answer = rb->text();
				break;
			}
		}
	}
	else if (qt == QuestionType::MultipleChoice)
	{
		QStringList selected;
		for (QCheckBox* cb : m_checkBoxes)
		{
			if (cb->isChecked())
			{
				selected << cb->text();
			}
		}
		answer = selected.join(QStringLiteral(", "));
	}
	else if (qt == QuestionType::ShortText)
	{
		answer = m_shortTextEdit->text().trimmed();
	}
	else if (qt == QuestionType::LongText)
	{
		answer = m_longTextEdit->toPlainText().trimmed();
	}

	if (answer.isEmpty()) return;

	FeatureMessage reply(Feature::Uid(SURVEY_FEATURE_UID), SurveyCommand::SubmitAnswer);
	reply.addArgument(SurveyArgument::Answer, answer);

	if (m_worker)
	{
		m_worker->sendFeatureMessageReply(reply);
	}

	QMessageBox::information(this, tr("Survey"), tr("Cevabınız gönderildi / Your answer has been submitted."));
	closeDialog();
}
