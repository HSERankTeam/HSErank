# container for build

FROM ghcr.io/userver-framework/docker-userver-build-base:nightly as build

RUN apt-get install -y --allow-unauthenticated libcrypto++-dev libcrypto++-doc libcrypto++-utils

WORKDIR /HSERank/backend/build

# ADD ./external /HSERank/backend/external
# ADD ./gatewayservice /HSERank/backend/gatewayservice
# ADD ./CMakeLists.txt /HSERank/backend/CMakeLists.txt

#  ["/HSERank/backend/bin"]
# VOLUME ["/HSERank/backend/build"]
# ADD ./bin /HSERank/backend/bin
# mount /HSERank/backend/build
# mount /HSERank/backend/bin
# ENTRYPOINT ["touch", "bin/RES.txt"]
# ENTRYPOINT cmake -D USERVER_NO_WERROR=1 .. && make

# COPY 

# docker run -it -v $(pwd)/bin/:/HSERank/backend/bin/ -v $(pwd)/build/:/HSERank/backend/build/ aaa