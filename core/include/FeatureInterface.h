#ifndef FEATURE_INTERFACE_H
#define FEATURE_INTERFACE_H

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"
#include "Feature.h"

class FeatureWorkerManager;

class FeatureInterface
{
public:
	virtual ~FeatureInterface() {}

	/*!
	 * \brief Returns a list of features implemented by the feature class
	 */
	virtual const FeatureList& featureList() const = 0;

	/*!
	 * \brief Start a feature on master side for given computer control interfaces
	 * \param feature the feature to start
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 * \param parent a pointer to the main window instance
	 */
	virtual bool startMasterFeature( const Feature& feature,
									 const ComputerControlInterfaceList& computerControlInterfaces,
									 QWidget* parent ) = 0;

	/*!
	 * \brief Stops a feature on master side for given computer control interfaces
	 * \param feature the feature to stop
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 * \param parent a pointer to the main window instance
	 */
	virtual bool stopMasterFeature( const Feature& feature,
									const ComputerControlInterfaceList& computerControlInterfaces,
									QWidget* parent ) = 0;

	/*!
	 * \brief Handles a received feature message inside service
	 * \param message the message which has been received and needs to be handled
	 * \param socketDevice the socket device which can be used for sending responses
	 */
	virtual bool handleServiceFeatureMessage( const FeatureMessage& message,
											  QIODevice* ioDevice,
											  FeatureWorkerManager& featureWorkerManager ) = 0;

	/*!
	 * \brief Handles a received feature message inside worker
	 * \param message the message which has been received and needs to be handled
	 * \param socketDevice the socket device which can be used for sending responses
	 */
	virtual bool handleWorkerFeatureMessage( const FeatureMessage& message,
											 QIODevice* ioDevice ) = 0;

};

typedef QList<FeatureInterface *> FeatureInterfaceList;

#define FeatureInterface_iid "org.italc-solutions.iTALC.Features.FeatureInterface"

Q_DECLARE_INTERFACE(FeatureInterface, FeatureInterface_iid)

#endif // FEATUREINTERFACE_H
