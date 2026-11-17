#include <QCoreApplication>

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
	auto app = new QCoreApplication(*argc, *argv);
	new VeyonCore(app, VeyonCore::Component::CLI, QStringLiteral("Fuzzer"), VeyonCore::defaultInitialization());
	return 0;
}
