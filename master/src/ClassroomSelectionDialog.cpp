#include "ClassroomSelectionDialog.h"

#include "ui_ClassroomSelectionDialog.h"

ClassroomSelectionDialog::ClassroomSelectionDialog( QAbstractItemModel* classroomListModel, QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::ClassroomSelectionDialog ),
	m_sortFilterProxyModel( this )
{
	ui->setupUi( this );

	m_sortFilterProxyModel.setSourceModel( classroomListModel );
	m_sortFilterProxyModel.sort( 0 );

	ui->listView->setModel( &m_sortFilterProxyModel );

	connect( ui->listView->selectionModel(), &QItemSelectionModel::currentChanged,
			 this, &ClassroomSelectionDialog::updateSelection );

	updateSearchFilter();
}



ClassroomSelectionDialog::~ClassroomSelectionDialog()
{
	delete ui;
}



void ClassroomSelectionDialog::updateSearchFilter()
{
	m_sortFilterProxyModel.setFilterRegExp( QRegExp( ui->filterLineEdit->text() ) );
	m_sortFilterProxyModel.setFilterCaseSensitivity( Qt::CaseInsensitive );

	ui->listView->selectionModel()->setCurrentIndex( m_sortFilterProxyModel.index( 0, 0 ),
														 QItemSelectionModel::ClearAndSelect );
}



void ClassroomSelectionDialog::updateSelection( const QModelIndex& current, const QModelIndex& previous )
{
	m_selectedClassroom = m_sortFilterProxyModel.data( current ).toString();
}
