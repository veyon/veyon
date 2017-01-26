#ifndef NETWORK_OBJECT_DIRECTORY_H
#define NETWORK_OBJECT_DIRECTORY_H

#include <QObject>

#include "NetworkObject.h"

class NetworkObjectDirectory : public QObject
{
	Q_OBJECT
public:
	NetworkObjectDirectory( QObject* parent ) : QObject( parent ) { }

	virtual QList<NetworkObject> objects( const NetworkObject& parent ) = 0;

signals:
	void objectsAboutToBeInserted( const NetworkObject& parent, int index, int count );
	void objectsInserted();
	void objectsAboutToBeRemoved( const NetworkObject& parent, int index, int count );
	void objectsRemoved();

};

#endif // NETWORK_OBJECT_DIRECTORY_H
