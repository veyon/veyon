#include <QBuffer>
#include <QGuiApplication>

#include "VncServerClient.h"
#include "VncServerProtocol.h"

class VncServerProtocolTest : public VncServerProtocol
{
public:
	VncServerProtocolTest(Plugin::Uid authMethod, QIODevice* socket, VncServerClient* client) :
		VncServerProtocol(socket, client),
		m_authMethod(authMethod)
	{
	}

	AuthMethodUids supportedAuthMethodUids() const override
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
	const Plugin::Uid m_authMethod;

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

	const VncServerProtocol::AuthMethodUids authMethodUids{
		Plugin::Uid{QByteArrayLiteral("63611f7c-b457-42c7-832e-67d0f9281085")},
		Plugin::Uid{QByteArrayLiteral("73430b14-ef69-4c75-a145-ba635d1cc676")},
		Plugin::Uid{QByteArrayLiteral("0c69b301-81b4-42d6-8fae-128cdd113314")}
	};

	VncServerClient client;
	VncServerProtocolTest protocol(authMethodUids.value(data[0]), &buffer, &client);

	client.setProtocolState(VncServerProtocol::State(data[1]));
	client.setAuthState(VncServerClient::AuthState(data[2]));
	client.setAccessControlState(VncServerClient::AccessControlState(data[3]));

	buffer.write(QByteArray(data+4, size-4));
	buffer.seek(0);

	protocol.read();

	return 0;
}
