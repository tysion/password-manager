# Этап 1: Сборка приложения
FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY src /app/src
COPY CMakeLists.txt /app
COPY configs /app/configs
COPY postgresql /app/postgresql

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --target all_with_tests -- -j$(nproc)

# Уменьшение размера бинарника
RUN strip /app/build/vaulty

# Этап 2: Упаковка минимального образа
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Установка необходимых библиотек
RUN apt-get update && apt-get install -y \
    libabsl-dev \
    libbenchmark-dev \
    libboost-context1.74-dev \
    libboost-coroutine1.74-dev \
    libboost-filesystem1.74-dev \
    libboost-iostreams1.74-dev \
    libboost-locale1.74-dev \
    libboost-program-options1.74-dev \
    libboost-regex1.74-dev \
    libboost-stacktrace1.74-dev \
    libboost1.74-dev \
    libbson-dev \
    libbz2-dev \
    libc-ares-dev \
    libcctz-dev \
    libcrypto++-dev \
    libcurl4-openssl-dev \
    libdouble-conversion-dev \
    libev-dev \
    libfmt-dev \
    libgflags-dev \
    libgmock-dev \
    libgrpc++-dev \
    libgrpc++1 \
    libgrpc-dev \
    libgtest-dev \
    libhiredis-dev \
    libidn11-dev \
    libjemalloc-dev \
    libkrb5-dev \
    libldap2-dev \
    librdkafka-dev \
    liblz4-dev \
    libmariadb-dev \
    libmongoc-dev \
    libnghttp2-dev \
    libpq-dev \
    libprotoc-dev \
    libpugixml-dev \
    libsnappy-dev \
    libsasl2-dev \
    libssl-dev \
    libxxhash-dev \
    libyaml-cpp0.7 \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Копирование бинарника и конфигов
COPY --from=builder /app/build/vaulty /app/
COPY --from=builder /app/configs /app/configs
COPY --from=builder /app/postgresql /app/postgresql

CMD ["./vaulty", "--config", "./configs/static_config.yaml", "--config_vars", "./configs/config_vars.yaml"]
