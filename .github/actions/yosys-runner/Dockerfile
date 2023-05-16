FROM bambuhls/dev-yosys:focal-23.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y --no-install-recommends \
   python3-defusedxml python3-distutils \
   && apt-get clean -y && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT [ "/entrypoint.sh" ]
