#include <QTcpSocket>

#include "VncServerClient.h"
#include "VncServerProtocol.h"

#include <cstring>

// a QTcpSocket that is never connected: reads come from the fuzzer input, writes are discarded
class FuzzedSocket : public QTcpSocket
{
public:
	FuzzedSocket(const char* data, qint64 size) :
		m_data(data),
		m_size(size)
	{
		setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
	}

	qint64 bytesAvailable() const override
	{
		return QIODevice::bytesAvailable() + m_size - m_position;
	}

protected:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto size = std::min(maxSize, m_size - m_position);
		if (size > 0)
		{
			std::memcpy(data, m_data + m_position, size_t(size));
			m_position += size;
		}
		return size;
	}

	qint64 writeData(const char* data, qint64 size) override
	{
		Q_UNUSED(data)
		return size;
	}

private:
	const char* m_data;
	const qint64 m_size;
	qint64 m_position = 0;
};

class VncServerProtocolTest : public VncServerProtocol
{
public:
	VncServerProtocolTest(RfbVeyonAuth::Type authMethod, QTcpSocket* socket, VncServerClient* client) :
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

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 5)
	{
		return 0;
	}

	FuzzedSocket socket(data + 4, qint64(size - 4));

	VncServerClient client;
	VncServerProtocolTest protocol(RfbVeyonAuth::Type(data[0]), &socket, &client);

	client.setProtocolState(VncServerProtocol::State(data[1]));
	client.setAuthState(VncServerClient::AuthState(data[2]));
	client.setAccessControlState(VncServerClient::AccessControlState(data[3]));

	protocol.read();

	return 0;
}
