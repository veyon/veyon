#include <QBuffer>
#include <QGuiApplication>

#include "VariantStream.h"

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	// QVariant requires GUI support to create types such as QPixmap
	new QGuiApplication(*argc, *argv);
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const char *data, size_t size)
{
	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);
	buffer.write(QByteArray::fromRawData(data, size));
	buffer.seek(0);

	VariantStream{&buffer}.read();

	return 0;
}
