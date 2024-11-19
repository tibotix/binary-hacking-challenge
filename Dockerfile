FROM python:3.12-slim@sha256:9638987a16d55a28096d4d222ea267710d830b207ed1a9f0b48ad2dbf67475aa

# https://github.com/reproducible-containers/repro-sources-list.sh
# Sorry for the mess.
COPY repro-sources-list.sh /usr/local/bin/repro-sources-list.sh

RUN chmod +x /usr/local/bin/repro-sources-list.sh && \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    DOCKER_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        socat

RUN useradd -UM chall
COPY flag /flag

COPY . /app
RUN chmod +x cpue
WORKDIR /app

EXPOSE 1024

CMD /app/socat.sh