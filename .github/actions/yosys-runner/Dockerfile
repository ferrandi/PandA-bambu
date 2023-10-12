
FROM bambuhls/verilator:focal-23.10 as verilator
FROM bambuhls/dev-yosys:focal-23.10 as base
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y --no-install-recommends \
   python3-defusedxml python3-distutils \
   && apt-get clean -y && rm -rf /var/lib/apt/lists/*
COPY --from=verilator /opt/verilator /opt/verilator
COPY entrypoint.sh /entrypoint.sh
ENV PATH /opt/verilator/bin:$PATH
ENTRYPOINT [ "/entrypoint.sh" ]
