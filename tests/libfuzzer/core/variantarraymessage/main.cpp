#include <QBuffer>

#include "VariantArrayMessage.h"

extern "C" int LLVMFuzzerTestOneInput(const char *data, size_t size)
{
	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);
	buffer.write(QByteArray(data, size));
	buffer.seek(0);

	VariantArrayMessage(&buffer).receive();

	return 0;
}
