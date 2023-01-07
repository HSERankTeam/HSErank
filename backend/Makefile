build_container:
	docker build . -t userver_build_container

build_debug:
	docker run \
	-v $(CURDIR)/external/:/HSERank/backend/external \
	-v $(CURDIR)/bin/:/HSERank/backend/bin/ \
	-v $(CURDIR)/build/:/HSERank/backend/build/ \
	-v $(CURDIR)/gatewayservice/:/HSERank/backend/gatewayservice/ \
	-v $(CURDIR)/CMakeLists.txt:/HSERank/backend/CMakeLists.txt \
	userver_build_container \
	/bin/bash -c "cmake -D USERVER_NO_WERROR=1 -DCMAKE_BUILD_TYPE=Debug -D USERVER_LTO=0 -D USERVER_FEATURE_MONGODB=0 -D USERVER_FEATURE_REDIS=0 -D USERVER_FEATURE_CLICKHOUSE=0 -D USERVER_FEATURE_RABBITMQ=0 -D USERVER_IS_THE_ROOT_PROJECT=0 -D CMAKE_CXX_STANDARD=20 .. && make"
