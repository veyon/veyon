/*
 * DocumentationFigureCreator.cpp - helper for creating documentation figures
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QScreen>
#include <QSpinBox>
#include <QStringListModel>
#include <QTextEdit>
#include <QTimeLine>
#include <QWindow>

#include "ComputerMonitoringWidget.h"
#include "DocumentationFigureCreator.h"
#include "FeatureManager.h"
#include "LocationDialog.h"
#include "MainToolBar.h"
#include "MainWindow.h"
#include "PasswordDialog.h"
#include "Plugin.h"
#include "PluginManager.h"
#include "Screenshot.h"
#include "ScreenshotManagementPanel.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "VncView.h"


#ifdef VEYON_DEBUG

DocumentationFigureCreator::DocumentationFigureCreator() :
	QObject(),
	m_master( nullptr ),
	m_eventLoop( this )
{
	m_master = new VeyonMaster;
}



void DocumentationFigureCreator::run()
{
	auto mainWindow = m_master->mainWindow();

	mainWindow->move( 0, 0 );
	mainWindow->resize( 3000, 1000 );
	mainWindow->show();

	hideComputers();

	createFeatureFigures();
	createContextMenuFigure();
	createLogonDialogFigure();
	createLocationDialogFigure();
	createScreenshotManagementPanelFigure();
	createTextMessageDialogFigure();
	createOpenWebsiteDialogFigure();
	createRunProgramDialogFigure();
	createRemoteAccessHostDialogFigure();
	createRemoteAccessWindowFigure();
	createPowerDownOptionsFigure();
	createPowerDownTimeInputDialogFigure();
}



void DocumentationFigureCreator::createFeatureFigures()
{
	Plugin::Uid previousPluginUid;
	Feature const* previousFeature = nullptr;

	int x = -1;
	int w = 0;

	auto toolbar = m_master->mainWindow()->findChild<MainToolBar *>();

	const QStringList separatedPluginFeatures( { QStringLiteral("a54ee018-42bf-4569-90c7-0d8470125ccf"),
												 QStringLiteral("80580500-2e59-4297-9e35-e53959b028cd")
											   } );

	const auto& features = m_master->features();

	for( const auto& feature : features )
	{
		auto btn = toolbar->findChild<ToolButton *>( feature.name() );
		const auto pluginUid = m_master->featureManager().pluginUid( feature );

		if( previousPluginUid.isNull() )
		{
			previousPluginUid = pluginUid;
			previousFeature = &feature;
			x = btn->x();
		}

		const auto separatedFeature = separatedPluginFeatures.contains( VeyonCore::formattedUuid( previousPluginUid ) );

		if( pluginUid != previousPluginUid || separatedFeature )
		{
			auto fileName = VeyonCore::pluginManager().pluginName( previousPluginUid );
			if( separatedFeature )
			{
				fileName = previousFeature->name();
			}
			grabWidget( toolbar, QPoint( x-2, btn->y()-1 ), QSize( w, btn->height() + 2 ),
						  QStringLiteral("Feature%1.png").arg( fileName ) );
			previousPluginUid = pluginUid;
			x = btn->x();
			w = btn->width() + 4;
		}
		else
		{
			w += btn->width() + 2;
		}

		if( feature == features.last() )
		{
			auto fileName = VeyonCore::pluginManager().pluginName( pluginUid );
			if( separatedFeature )
			{
				fileName = feature.name();
			}
			grabWidget( toolbar, QPoint( x-2, btn->y()-1 ), QSize( w, btn->height() + 2 ),
						  QStringLiteral("Feature%1.png").arg( fileName ) );
		}

		previousFeature = &feature;
	}
}



void DocumentationFigureCreator::createContextMenuFigure()
{
	auto view = m_master->mainWindow()->findChild<ComputerMonitoringWidget *>();
	auto menu = view->findChild<QMenu *>();

	connect( menu, &QMenu::aboutToShow, this, [menu]() {
		grabWidget( menu, QPoint(), menu->size(), QStringLiteral("ContextMenu.png") );
		menu->close();
	}, Qt::QueuedConnection );

	view->showContextMenu( QPoint( 200, 200 ) );
}



void DocumentationFigureCreator::createLogonDialogFigure()
{
	PasswordDialog dialog( m_master->mainWindow() );
	dialog.show();
	dialog.findChild<QLineEdit *>( QStringLiteral("password") )->setText( QStringLiteral( "TeacherPassword") );
	dialog.findChild<QLineEdit *>( QStringLiteral("password") )->cursorForward( false );
	dialog.findChild<QLineEdit *>( QStringLiteral("username") )->setText( tr( "Teacher") );

	grabDialog( &dialog, {}, QStringLiteral("LogonDialog.png") );

	dialog.exec();
}



void DocumentationFigureCreator::createLocationDialogFigure()
{
	QStringList locations;
	locations.reserve( 3 );

	for( int i = 0; i < 3; ++i )
	{
		locations.append( tr( "Room %1").arg( 101 + i ) );
	}

	QStringListModel locationsModel( locations, this );
	LocationDialog dialog( &locationsModel, m_master->mainWindow() );

	grabDialog( &dialog, QSize( 300, 200 ), QStringLiteral("LocationDialog.png") );

	dialog.exec();
}



void DocumentationFigureCreator::createScreenshotManagementPanelFigure()
{
	auto window = m_master->mainWindow();
	auto panel = window->findChild<ScreenshotManagementPanel *>();
	auto panelButton = window->findChild<QToolButton *>( QStringLiteral("screenshotManagementPanelButton") );
	auto list = panel->findChild<QListView *>();

	const QDate date( 2019, 4, 4 );
	const QTime time( 9, 36, 27 );

	QStringList screenshots({
								Screenshot::constructFileName( QStringLiteral("Albert Einstein"), QStringLiteral("mars"), date, time ),
								Screenshot::constructFileName( QStringLiteral("Blaise Pascal"), QStringLiteral("venus"), date, time ),
								Screenshot::constructFileName( QStringLiteral("Caroline Herschel"), QStringLiteral("saturn"), date, time ),
								Screenshot::constructFileName( QStringLiteral("Dorothy Hodgkin"), QStringLiteral("pluto"), date, time )
						   });


	QStringListModel screenshotsModel( screenshots, this );
	list->setModel( &screenshotsModel );

	constexpr int exampleScreenshotIndex = 1;
	list->selectionModel()->setCurrentIndex( screenshotsModel.index( exampleScreenshotIndex ), QItemSelectionModel::SelectCurrent );

	Screenshot exampleScreenshot( screenshots[exampleScreenshotIndex] );
	exampleScreenshot.setImage( QImage( QStringLiteral(":/examples/example-screenshot.png" ) ) );
	panel->setPreview( exampleScreenshot );

	window->setMaximumHeight( 600 );
	panel->setMaximumWidth( 400 );
	panelButton->setChecked( true );

	scheduleUiOperation( [this, panel, window, panelButton]() {
		const auto panelPos = panel->mapTo( window, QPoint( 0, 0 ) );
		const auto panelButtonPos = panelButton->mapTo( window, panelButton->rect().bottomRight() );
		grabWindow( window, panelPos,
					QSize( panel->width(), panelButtonPos.y() - panelPos.y() + 2 ),
					QStringLiteral("ScreenshotManagementPanel.png") );
		m_eventLoop.quit();
	} );

	m_eventLoop.exec();
}



void DocumentationFigureCreator::createPowerDownOptionsFigure()
{
	auto toolbar = m_master->mainWindow()->findChild<MainToolBar *>();
	auto powerDownButton = toolbar->findChild<ToolButton *>( QStringLiteral("PowerDown") );

	scheduleUiOperation( [this, powerDownButton]() {
		scheduleUiOperation( [this, powerDownButton]() {
			auto menu = powerDownButton->menu();

			grabWindow( m_master->mainWindow(), powerDownButton->mapTo( m_master->mainWindow(), QPoint( 0, 0 ) ),
						QSize( qMax( powerDownButton->width(), menu->width() ),
							   powerDownButton->height() + menu->height() ),
						QStringLiteral("PowerDownOptions.png") );
			menu->close();
		} );
		auto menu = powerDownButton->menu();
		menu->close();

		powerDownButton->click();

	} );

	powerDownButton->showMenu();
}



void DocumentationFigureCreator::createPowerDownTimeInputDialogFigure()
{
	scheduleUiOperation( []() {
		auto dialog = qobject_cast<QDialog *>( QApplication::activeWindow() );

		dialog->findChild<QSpinBox *>( QStringLiteral("minutesSpinBox") )->setValue( 3 );

		grabDialog( dialog, {}, QStringLiteral("PowerDownTimeInputDialog.png") );
	});

	m_master->runFeature( m_master->featureManager().feature( Feature::Uid( "352de795-7fc4-4850-bc57-525bcb7033f5" ) ) );
}



void DocumentationFigureCreator::createTextMessageDialogFigure()
{
	scheduleUiOperation( []() {
		auto dialog = qobject_cast<QDialog *>( QApplication::activeWindow() );

		dialog->findChild<QTextEdit *>()->setText( tr( "Please complete all tasks within the next 5 minutes." ) );

		grabDialog( dialog, {}, QStringLiteral("TextMessageDialog.png") );
	});

	m_master->runFeature( m_master->featureManager().feature( Feature::Uid( "e75ae9c8-ac17-4d00-8f0d-019348346208" ) ) );
}



void DocumentationFigureCreator::createOpenWebsiteDialogFigure()
{
	scheduleUiOperation( []() {
		auto dialog = qobject_cast<QDialog *>( QApplication::activeWindow() );
		dialog->findChild<QLineEdit *>( QStringLiteral("websiteLineEdit") )->
				setText( QStringLiteral("https://veyon.io") );

		grabDialog( dialog, {}, QStringLiteral("OpenWebsiteDialog.png") );
	});

	m_master->runFeature( m_master->featureManager().feature( Feature::Uid( "8a11a75d-b3db-48b6-b9cb-f8422ddd5b0c" ) ) );
}



void DocumentationFigureCreator::createRunProgramDialogFigure()
{
	scheduleUiOperation( []() {
		auto dialog = qobject_cast<QDialog *>( QApplication::activeWindow() );
		dialog->findChild<QTextEdit *>()->setText( QStringLiteral("notepad") );
		dialog->setFocus();

		grabDialog( dialog, {}, QStringLiteral("RunProgramDialog.png") );
	});

	m_master->runFeature( m_master->featureManager().feature( Feature::Uid( "da9ca56a-b2ad-4fff-8f8a-929b2927b442" ) ) );
}



void DocumentationFigureCreator::createRemoteAccessHostDialogFigure()
{
	scheduleUiOperation( []() {
		auto dialog = qobject_cast<QInputDialog *>( QApplication::activeWindow() );
		dialog->setTextValue( QStringLiteral("pc27") );

		grabDialog( dialog, {}, QStringLiteral("RemoteAccessHostDialog.png") );
	} );

	m_master->runFeature( m_master->featureManager().feature( Feature::Uid( "a18e545b-1321-4d4e-ac34-adc421c6e9c8" ) ) );
}



void DocumentationFigureCreator::createRemoteAccessWindowFigure()
{
	scheduleUiOperation( [this]() {
		auto dialog = qobject_cast<QInputDialog *>( QApplication::activeWindow() );
		dialog->setTextValue( QStringLiteral("pc27") );
		dialog->setFocus();
		dialog->move( 0, 0 );

		scheduleUiOperation( [this, dialog]() {
			dialog->accept();

			scheduleUiOperation( [this]() {
				auto window = QApplication::activeWindow();
				auto vncView = window->findChild<VncView *>();
				Q_ASSERT(vncView != nullptr);

				for( auto timeline : window->findChildren<QTimeLine *>() )
				{
					timeline->stop();
					timeline->setCurrentTime( 0 );
				}

				window->showNormal();
				window->setFixedSize( 1000, 450 );
				window->move( 0, 0 );

				scheduleUiOperation( [this, window]() {
					grabWindow( window, QStringLiteral("RemoteAccessWindow.png") );

					auto shortcuts = window->findChild<QToolButton *>( QStringLiteral("shortcuts") );
					Q_ASSERT(shortcuts != nullptr);

					scheduleUiOperation( [this, window, shortcuts]() {
						auto menu = shortcuts->menu();

						grabWindow( window, shortcuts->mapTo( window, QPoint( 0, 0 ) ),
									QSize( qMax( shortcuts->width(), menu->width() ),
										   shortcuts->height() + menu->height() ),
									QStringLiteral("RemoteAccessShortcutsMenu.png") );
						menu->close();
						window->close();
						m_eventLoop.quit();
					} );

					shortcuts->showMenu();
				} );
			} );
		} );
	} );

	m_master->runFeature( m_master->featureManager().feature( Feature::Uid( "ca00ad68-1709-4abe-85e2-48dff6ccf8a2" ) ) );
	m_eventLoop.exec();
}



void DocumentationFigureCreator::hideComputers()
{
	auto view = m_master->mainWindow()->findChild<ComputerMonitoringWidget *>();
	view->setSearchFilter( QStringLiteral("XXXXXX") );
}



void DocumentationFigureCreator::scheduleUiOperation( const std::function<void(void)>& operation )
{
	QTimer::singleShot( DialogDelay, this, operation );
}



void DocumentationFigureCreator::grabWidget(QWidget* widget, const QPoint& pos, const QSize& size, const QString& fileName)
{
	QPixmap pixmap( size );
	widget->render( &pixmap, QPoint(), QRegion( QRect( pos, size ) ) );
	pixmap.save( fileName );
}



void DocumentationFigureCreator::grabDialog(QDialog* dialog, const QSize& size, const QString& fileName)
{
	dialog->show();
	dialog->setFocus();
	dialog->move( 0, 0 );

	if( size.isValid() )
	{
		dialog->setFixedSize( size );
	}

	QTimer::singleShot( DialogDelay, dialog, [dialog, fileName]() {
		grabWindow( dialog, fileName );
		dialog->close();
	} );
}



void DocumentationFigureCreator::grabWindow(QWidget* widget, const QString& fileName)
{
	widget->repaint();
	QGuiApplication::sync();

	auto window = widget->window();
	const auto rect = window->frameGeometry();
	window->windowHandle()->screen()->grabWindow( 0, rect.x(), rect.y(), rect.width(), rect.height() ).save( fileName );
}



void DocumentationFigureCreator::grabWindow(QWidget* widget, const QPoint& pos, const QSize& size, const QString& fileName)
{
	QGuiApplication::sync();

	widget->window()->windowHandle()->screen()->grabWindow( widget->winId(), pos.x(), pos.y(), size.width(), size.height() ).save( fileName );
}

#include "moc_DocumentationFigureCreator.cpp"

#endif
