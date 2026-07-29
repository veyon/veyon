#include "FocusTrackerDialog.h"
#include "FocusTrackerPlugin.h"
#include "FeatureMessage.h"
#include "ComputerControlInterface.h"
#include <QVBoxLayout>
#include <QHeaderView>

FocusTrackerDialog::FocusTrackerDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Dikkat Takibi - Aktif Pencereler"));
	resize(700, 400);

	m_tableWidget = new QTableWidget(this);
	m_tableWidget->setColumnCount(2);
	m_tableWidget->setHorizontalHeaderLabels({tr("Bilgisayar (IP)"), tr("Aktif Uygulama / Pencere")});
	m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_tableWidget);
	
	m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, &FocusTrackerDialog::pollFocus);
	
	// Poll every 3 seconds
	m_timer->start(3000);
}

FocusTrackerDialog::~FocusTrackerDialog()
{
}

void FocusTrackerDialog::setControlInterfaces(const ComputerControlInterfaceList& interfaces)
{
	m_controlInterfaces = interfaces;
	// Initial poll immediately
	pollFocus();
}

void FocusTrackerDialog::setFeatureUid(const Feature::Uid& uid)
{
	m_featureUid = uid;
}

void FocusTrackerDialog::pollFocus()
{
	if (m_controlInterfaces.isEmpty()) return;

	FeatureMessage msg(m_featureUid, FocusTrackerCommand::PollFocus);
	for (auto* interface : m_controlInterfaces) {
		if (interface) {
			interface->sendFeatureMessage(msg);
		}
	}
}

void FocusTrackerDialog::updateComputerInfo(const QString& host, const QString& title)
{
	int row = -1;
	if (m_hostRowMap.contains(host)) {
		row = m_hostRowMap[host];
	} else {
		row = m_tableWidget->rowCount();
		m_tableWidget->insertRow(row);
		
		QTableWidgetItem* hostItem = new QTableWidgetItem(host);
		m_tableWidget->setItem(row, 0, hostItem);
		
		QTableWidgetItem* titleItem = new QTableWidgetItem(title);
		m_tableWidget->setItem(row, 1, titleItem);
		
		m_hostRowMap[host] = row;
	}
	
	// Update title and coloring
	QTableWidgetItem* item = m_tableWidget->item(row, 1);
	if (item) {
		item->setText(title);
		
		// Optional: basic heuristic for game or non-educational app highlighting
		if (title.contains("Minecraft", Qt::CaseInsensitive) || 
			title.contains("Oyun", Qt::CaseInsensitive) ||
			title.contains("YouTube", Qt::CaseInsensitive)) {
			item->setBackground(QColor(255, 200, 200)); // Light red
		} else {
			item->setBackground(QColor(200, 255, 200)); // Light green
		}
	}
}
