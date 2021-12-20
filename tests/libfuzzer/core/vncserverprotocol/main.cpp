#include <QBuffer>
#include <QGuiApplication>

#include "VncServerClient.h"
#include "VncServerProtocol.h"

class VncServerProtocolTest : public VncServerProtocol
{
public:
	VncServerProtocolTest(RfbVeyonAuth::Type authMethod, QIODevice* socket, VncServerClient* client) :
		VncServerProtocol(socket, client),
		m_authMethod(authMethod)
	{
	}

	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override
	{
		return {m_authMethod};
	}

	void processAuthenticationMessage(VariantArrayMessage& message) override
	{
		Q_UNUSED(message)
	}

	void performAccessControl() override
	{
	}

private:
	const RfbVeyonAuth::Type m_authMethod;

};



extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	// QVariant requires GUI support to create types such as QPixmap
	new QGuiApplication(*argc, *argv);
	return 0;
}



extern "C" int LLVMFuzzerTestOneInput(const char *data, size_t size)
{
	if (size < 5)
	{
		return 0;
	}

	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);

	VncServerClient client;
	VncServerProtocolTest protocol(RfbVeyonAuth::Type(data[0]), &buffer, &client);

	client.setProtocolState(VncServerProtocol::State(data[1]));
	client.setAuthState(VncServerClient::AuthState(data[2]));
	client.setAccessControlState(VncServerClient::AccessControlState(data[3]));

	buffer.write(QByteArray(data+4, size-4));
	buffer.seek(0);

	protocol.read();

	return 0;
}
