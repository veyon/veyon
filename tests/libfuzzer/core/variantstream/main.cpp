#include <QBuffer>

#include "VariantStream.h"

extern "C" int LLVMFuzzerTestOneInput(const char *data, size_t size)
{
	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);
	buffer.write(QByteArray::fromRawData(data, size));
	buffer.seek(0);

	VariantStream{&buffer}.read();

	return 0;
}
