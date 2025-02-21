FROM python:3.12-slim@sha256:9638987a16d55a28096d4d222ea267710d830b207ed1a9f0b48ad2dbf67475aa AS build-env

# https://github.com/reproducible-containers/repro-sources-list.sh
# Sorry for the mess.
COPY ./challenge/repro-sources-list.sh /usr/local/bin/repro-sources-list.sh

RUN chmod +x /usr/local/bin/repro-sources-list.sh && \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    DOCKER_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake g++ gcc make

COPY ./cpue /cpue

# now build cpue and install it
RUN cd /cpue && rm -rf build/ && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=0 -DENABLE_TRACE=1 -G "Unix Makefiles" .. && \
    cmake --build .


FROM python:3.12-slim@sha256:9638987a16d55a28096d4d222ea267710d830b207ed1a9f0b48ad2dbf67475aa

# https://github.com/reproducible-containers/repro-sources-list.sh
# Sorry for the mess.
COPY ./challenge/repro-sources-list.sh /usr/local/bin/repro-sources-list.sh

RUN chmod +x /usr/local/bin/repro-sources-list.sh && \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    DOCKER_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        bash socat

RUN useradd -UM chall
COPY ./challenge /app
COPY --from=build-env /cpue/build/cpue /app/cpue
RUN chmod +x /app/socat.sh /app/cpue /app/cpue.py
RUN chown chall:chall /app /app/*

WORKDIR /app

EXPOSE 1024
CMD ["/app/socat.sh"]