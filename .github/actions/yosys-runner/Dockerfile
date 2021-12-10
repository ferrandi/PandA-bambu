FROM bambuhls/yosys:focal

RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
   python3-defusedxml python3-distutils \
   && apt-get clean -y && rm -rf /var/lib/apt/lists/* /tmp/library-scripts

COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT [ "/entrypoint.sh" ]
