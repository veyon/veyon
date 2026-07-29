#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>
#include <QPointer>

class WhiteboardWidget;

class VEYON_CORE_EXPORT AnnotationPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit AnnotationPlugin(QObject* parent = nullptr);
	~AnnotationPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("b2c3d4e5-6789-01ab-cdef-234567890123")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("annotation"); }
	QString description() const override { return tr("Ekran Çizimi (Anotasyon)"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Efrail"); }
	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

private:
	const Feature m_annotationFeature;
	const FeatureList m_features;
	
	QPointer<WhiteboardWidget> m_whiteboard;
};
