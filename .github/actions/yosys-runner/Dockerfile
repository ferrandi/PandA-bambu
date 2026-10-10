
FROM bambuhls/verilator:bullseye-24.08 AS verilator

FROM bambuhls/yosys:bullseye-24.08
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true
RUN apt-get update && apt-get install -y --no-install-recommends \
   g++ g++-multilib gcc gcc-multilib make perl python3-psutil python3-distutils \
   && rm -rf /var/lib/apt/lists/*
COPY --link --from=verilator /opt/verilator /opt/verilator
COPY entrypoint.sh /entrypoint.sh
ENV PATH=/opt/verilator/bin:$PATH
ENTRYPOINT [ "/entrypoint.sh" ]
