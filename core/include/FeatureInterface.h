#ifndef FEATURE_INTERFACE_H
#define FEATURE_INTERFACE_H

#include "ComputerControlInterface.h"
#include "ItalcCore.h"
#include "Feature.h"


class FeatureInterface
{
public:
	virtual ~FeatureInterface() {}

	/*!
	 * \brief Returns a list of features implemented by the feature class
	 */
	virtual const FeatureList& featureList() const = 0;

	/*!
	 * \brief Run a feature on master side for given computer control interfaces
	 * \param feature the feature to run
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 * \param parent a pointer to the main window instance
	 */
	virtual bool runMasterFeature( const Feature& feature, const ComputerControlInterfaceList& computerControlInterfaces, QWidget* parent ) = 0;

	/*!
	 * \brief Run a feature on service side after receiving given message
	 * \param feature the feature to run
	 * \param socketDevice the socket device which can be used for sending responses
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool runServiceFeature( const Feature& feature, SocketDevice& socketDevice, const ItalcCore::Msg& message ) = 0;

};

typedef QList<FeatureInterface *> FeatureInterfaceList;

#define FeatureInterface_iid "org.italc-solutions.iTALC.Features.FeatureInterface"

Q_DECLARE_INTERFACE(FeatureInterface, FeatureInterface_iid)

#endif // FEATUREINTERFACE_H
