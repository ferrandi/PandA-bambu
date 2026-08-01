FROM bambuhls/dev:bookworm-24.08

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        bc \
        clang-16 \
        file \
        gawk \
        gettext-base \
        g++-multilib \
        libclang-16-dev \
        llvm-16 \
        llvm-16-dev \
        verilator \
    && rm -rf /var/lib/apt/lists/*

RUN dpkg-query -W bc clang-16 g++-multilib gawk gettext-base libclang-16-dev llvm-16 llvm-16-dev verilator \
    && test -f /usr/lib/llvm-16/include/clang/AST/ASTConsumer.h \
    && test -f /usr/lib/llvm-16/include/clang/Frontend/FrontendPluginRegistry.h \
    && test -f /usr/lib/llvm-16/include/llvm/IR/Module.h \
    && test -f /usr/lib/llvm-16/lib/cmake/llvm/LLVMConfig.cmake \
    && test -f /usr/lib/llvm-16/lib/cmake/clang/ClangConfig.cmake \
    && clang-16 --version \
    && clang++-16 --version \
    && clang-cpp-16 --version \
    && llvm-config-16 --version \
    && llvm-link-16 --version \
    && opt-16 --version \
    && awk --version \
    && bc --version \
    && envsubst --version \
    && printf '#include <cerrno>\nint main() { return errno; }\n' \
       | clang++-16 -m32 -x c++ - -o /tmp/clang16-m32-smoke \
    && file /tmp/clang16-m32-smoke \
    && rm /tmp/clang16-m32-smoke \
    && verilator --version

COPY entrypoint.sh /entrypoint.sh
COPY normalize-output-permissions.sh /normalize-output-permissions.sh
ENTRYPOINT [ "/entrypoint.sh" ]
