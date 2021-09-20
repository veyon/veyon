include(BuildVeyonApplication)
include(WindowsBuildHelpers)

build_veyon_application(veyon-worker
	src/FeatureWorkerManagerConnection.cpp
	src/FeatureWorkerManagerConnection.h
	src/main.cpp
	src/VeyonWorker.cpp
	src/VeyonWorker.h
	)

add_windows_resource(veyon-worker)
make_graphical_app(veyon-worker)
