#include "SurveyMasterDialog.h"
#include "ui_SurveyMasterDialog.h"
#include "SurveyPlugin.h"
#include <QMessageBox>

SurveyMasterDialog::SurveyMasterDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::SurveyMasterDialog)
{
	ui->setupUi(this);
	connect(ui->sendButton, &QPushButton::clicked, this, &SurveyMasterDialog::onSendClicked);
	connect(ui->stopButton, &QPushButton::clicked, this, &SurveyMasterDialog::onStopClicked);
	connect(ui->typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SurveyMasterDialog::onTypeChanged);
	connect(ui->addOptionButton, &QPushButton::clicked, this, &SurveyMasterDialog::onAddOptionClicked);
	connect(ui->removeOptionButton, &QPushButton::clicked, this, &SurveyMasterDialog::onRemoveOptionClicked);

	ui->answersTable->setColumnCount(2);
	ui->answersTable->setHorizontalHeaderLabels({tr("Computer"), tr("Answer")});
	ui->answersTable->horizontalHeader()->setStretchLastSection(true);

	onTypeChanged(ui->typeComboBox->currentIndex());
}

SurveyMasterDialog::~SurveyMasterDialog()
{
	delete ui;
}

void SurveyMasterDialog::setComputers(const ComputerControlInterfaceList& computers)
{
	m_computers = computers;
}

void SurveyMasterDialog::onTypeChanged(int index)
{
	if (index == 0 || index == 1)
	{
		ui->optionsWidget->show();
	}
	else
	{
		ui->optionsWidget->hide();
	}
}

void SurveyMasterDialog::onAddOptionClicked()
{
	QString text = ui->addOptionEdit->text().trimmed();
	if (!text.isEmpty())
	{
		ui->optionsList->addItem(text);
		ui->addOptionEdit->clear();
	}
}

void SurveyMasterDialog::onRemoveOptionClicked()
{
	qDeleteAll(ui->optionsList->selectedItems());
}

void SurveyMasterDialog::onSendClicked()
{
	if (m_computers.isEmpty())
	{
		QMessageBox::warning(this, tr("Survey"), tr("No computers selected."));
		return;
	}

	ui->answersTable->setRowCount(0);

	FeatureMessage msg(Feature::Uid(SURVEY_FEATURE_UID), SurveyCommand::StartSurvey);

	QuestionType qt = QuestionType::SingleChoice;
	switch(ui->typeComboBox->currentIndex()) {
		case 0: qt = QuestionType::SingleChoice; break;
		case 1: qt = QuestionType::MultipleChoice; break;
		case 2: qt = QuestionType::ShortText; break;
		case 3: qt = QuestionType::LongText; break;
		case 4: qt = QuestionType::TrueFalse; break;
	}

	msg.addArgument(SurveyArgument::QuestionType, static_cast<int>(qt));
	msg.addArgument(SurveyArgument::Question, ui->questionEdit->toPlainText());

	QStringList options;
	if (qt == QuestionType::SingleChoice || qt == QuestionType::MultipleChoice)
	{
		for (int i = 0; i < ui->optionsList->count(); ++i)
		{
			options << ui->optionsList->item(i)->text();
		}
	}
	msg.addArgument(SurveyArgument::Options, options);

	for (auto& computer : m_computers)
	{
		computer->sendFeatureMessage(msg);
	}

	QMessageBox::information(this, tr("Survey"), tr("Survey sent to %1 computers.").arg(m_computers.size()));
}

void SurveyMasterDialog::onStopClicked()
{
	FeatureMessage msg(Feature::Uid(SURVEY_FEATURE_UID), SurveyCommand::StopSurvey);

	for (auto& computer : m_computers)
	{
		computer->sendFeatureMessage(msg);
	}
}

void SurveyMasterDialog::addAnswer(const QString& computerName, const QString& answer)
{
	int row = ui->answersTable->rowCount();
	ui->answersTable->insertRow(row);
	ui->answersTable->setItem(row, 0, new QTableWidgetItem(computerName));
	ui->answersTable->setItem(row, 1, new QTableWidgetItem(answer));
}
