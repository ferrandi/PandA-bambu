ARG VARIANT=xenial

FROM bambuhls/iverilog:${VARIANT} as iverilog
FROM bambuhls/verilator:${VARIANT} as verilator

FROM bambuhls/appimage:${VARIANT} as base

### From https://github.com/microsoft/vscode-dev-containers/blob/main/containers/ubuntu/.devcontainer/base.Dockerfile
# Options for setup script
ARG INSTALL_ZSH="true"
ARG UPGRADE_PACKAGES="false"
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID
ARG COMPILERS=gcc-4.9,clang-4
# Install needed packages and setup non-root user. Use a separate RUN statement to add your own dependencies.
COPY library-scripts/*.sh library-scripts/*.env /tmp/library-scripts/
WORKDIR /
RUN bash /tmp/library-scripts/common-debian.sh "${INSTALL_ZSH}" "${USERNAME}" "${USER_UID}" "${USER_GID}" "${UPGRADE_PACKAGES}" "true" "true" \
    && bash /tmp/library-scripts/compiler-download.sh "/" "${COMPILERS}" \
    && bash /tmp/library-scripts/compiler-setup.sh \
    && apt-get install --no-install-recommends -y clang-format python3-defusedxml python3-distutils \
    && apt-get clean -y && rm -rf /var/lib/apt/lists/* /tmp/library-scripts

COPY --from=iverilog /opt/iverilog /opt/iverilog
COPY --from=verilator /opt/verilator /opt/verilator
ENV PATH /opt/verilator/bin:/opt/iverilog/bin:$PATH
