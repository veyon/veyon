#include "AnnotationPlugin.h"
#include "WhiteboardWidget.h"
#include <QApplication>

AnnotationPlugin::AnnotationPlugin(QObject* parent) :
	QObject(parent),
	m_annotationFeature(QStringLiteral("Annotation"), Feature::Flag::Action,
				  uid(), Feature::Uid(), tr("Ekran Çizimi"), {}, tr("Ekran üzerinde serbest çizim yapın."), QStringLiteral(":/core/document-edit.png")),
	m_features({m_annotationFeature})
{
}

AnnotationPlugin::~AnnotationPlugin()
{
	if (m_whiteboard) {
		m_whiteboard->close();
		m_whiteboard->deleteLater();
	}
}

bool AnnotationPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	Q_UNUSED(computerControlInterfaces);
	
	if (featureUid != m_annotationFeature.uid()) return false;

	if (operation == Operation::Start) {
		if (!m_whiteboard) {
			m_whiteboard = new WhiteboardWidget();
			m_whiteboard->setAttribute(Qt::WA_DeleteOnClose);
		}
		m_whiteboard->showFullScreen();
		return true;
	} else if (operation == Operation::Stop) {
		if (m_whiteboard) {
			m_whiteboard->close();
			m_whiteboard = nullptr;
		}
		return true;
	}
	return false;
}
