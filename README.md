# PandA Bambu HLS Framework
![](style/img/panda.png.in)

[![License: Apache 2.0 + BAMBU Exceptions](https://img.shields.io/badge/License-Apache%202.0%20%2B%20BAMBU%20Exceptions-blue.svg)](LICENSE)

----

The primary objective of the PandA project is to develop a usable framework
that will enable the research of new ideas in the HW-SW Co-Design field.

The PandA framework includes methodologies supporting the research on high-level 
synthesis of hardware accelerators, on parallelism extraction for embedded systems, 
on hardware/software partitioning and mapping, on metrics for performance estimation 
of embedded software applications and on dynamic reconfigurable devices.

PandA is free software released under the Apache License v2.0 with BAMBU Exceptions and being 
developed at Politecnico di Milano (Italy).

The source files currently distributed mainly cover the high-level synthesis 
of C/C++ based descriptions. In particular, the tool Bambu provides a research environment to experiment with new ideas across HLS, high-level verification and debugging, FPGA/ASIC design, design flow space exploration, and parallel hardware accelerator design.
Bambu accepts as input standard C/C++ specifications and compiler intermediate representations coming from the well-known Clang/LLVM and GCC compilers.
The broad spectrum and flexibility of input formats allow the electronic design automation research community to explore and integrate new transformations and optimizations.

If you use Bambu in your research, please cite:
```
@INPROCEEDINGS{ferrandi2021bambu,
  author={Ferrandi, Fabrizio and Castellana, Vito Giovanni 
          and Curzel, Serena and Fezzardi, Pietro and Fiorito, Michele 
          and Lattuada, Marco and Minutoli, Marco and Pilato, Christian 
          and Tumeo, Antonino},
  booktitle={2021 58th ACM/IEEE Design Automation Conference (DAC)}, 
  title={Invited: Bambu: an Open-Source Research Framework for the 
         High-Level Synthesis of Complex Applications}, 
  year={2021},
  pages={1327-1330},
  abstract = {This paper presents the open-source high-level synthesis (HLS) research 
              framework Bambu. Bambu provides a research environment to experiment with 
              new ideas across HLS, high-level verification and debugging, FPGA/ASIC design,
              design flow space exploration, and parallel hardware accelerator design. The 
              tool accepts as input standard C/C++ specifications and compiler intermediate 
              representations (IRs) coming from the well-known Clang/LLVM and GCC compilers. 
              The broad spectrum and flexibility of input formats allow the electronic 
              design automation (EDA) research community to explore and integrate new 
              transformations and optimizations. The easily extendable modular framework 
              already includes many optimizations and HLS benchmarks used to evaluate 
              the QoR of the tool against existing approaches [1]. The integration with 
              synthesis and verification backends (commercial and open-source) allows 
              researchers to quickly test any new finding and easily obtain performance 
              and resource usage metrics for a given application. Different FPGA devices 
              are supported from several different vendors: AMD/Xilinx, Intel/Altera, 
              Lattice Semiconductor, and NanoXplore. Finally, integration with the OpenRoad 
              open-source end-to-end silicon compiler perfectly fits with the recent push 
              towards open-source EDA.},
  publisher={{IEEE}},
  doi={10.1109/DAC18074.2021.9586110},
  ISSN={0738-100X},
  month={Dec},
  pdf={https://re.public.polimi.it/retrieve/668507/dac21_bambu.pdf}
}
```
<br>

# Installation instructions

You may also download pre-compiled AppImage distributions at https://release.bambuhls.eu/.

Dockerfiles are also available under *etc/containers* and pre-built docker images can be downloaded at [Docker Hub](https://hub.docker.com/u/bambuhls).

A Google Colab notebook with many examples to play with Bambu is available. [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/ferrandi/PandA-bambu/blob/dev%2Fpanda/documentation/bambu101/tutorial_isca_2025.ipynb)

<br>

# CMake build
The CMake build is the primary path; `compile_commands.json` is generated automatically. Default install prefix is `/usr/local` unless overridden.

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build -j"$(nproc)"
cmake --install build
```

Before running `bambu` for synthesis or simulation, source the installed environment script:
```bash
source <install_dir>/settings.sh
# example for the default CI/local install path:
source /opt/panda/settings.sh
```
This sets `BAMBU_HLS` and `BAMBU_HLS_BACKEND_PATH`, required by backend flows.

For OpenROAD backend users, ORFS revision pinning and recovery instructions are documented in
[`documentation/openroad_orfs_pinned_digest.md`](documentation/openroad_orfs_pinned_digest.md).

Verbose one-off build: `cmake --build build -- -j"$(nproc)" VERBOSE=1` (Makefiles) or `cmake --build build -- -v` (Ninja).

Ninja generator: `cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release` then `cmake --build build-ninja -j"$(nproc)"` (use `-v` for verbose).

AppImage (Docker) quick commands — recommended for portability:
```
# Configure for containerized AppImage build (GLIBC_2.17 / Xenial baseline)
cmake -G Ninja -S . -B build-app \
  -DPANDA_APPIMAGE_NAME=bambu \
  -DPANDA_APPIMAGE_BUILD_ENV=container

# Build everything inside the container and produce the AppImage
cmake --build build-app --target appimage_bundle

# Result: build-app/appimage-root/bambu.AppImage
```
The container target (`appimage_bundle`) runs a full cmake configure → build → install → GLIBC audit → appimagetool sequence inside the `bambuhls/dev:xenial-24.08` image (configurable via `-DPANDA_APPIMAGE_CONTAINER_IMAGE=...`). No host build or install step is needed.

To supply a pre-downloaded runtime add `-DPANDA_APPIMAGE_RUNTIME_FILE=/path/to/runtime-x86_64`. To change staging dir use `-DPANDA_APPIMAGE_ROOT=/path/to/stage`. Keep normal installs (e.g. /opt/panda) in a separate build directory. For host-only packaging (no container) use `-DPANDA_APPIMAGE_BUILD_ENV=host`.

Feature toggles: `-DPANDA_BUILD_BAMBU=ON`, `-DPANDA_BUILD_EUCALYPTUS=ON`, `-DPANDA_BUILD_CC=ON` (`bambu-cc`, required for libbambu/softfloat/softint/libm; default libbambu compiler `-DPANDA_LIBBAMBU_COMPILER=I386_CLANG13`), `-DPANDA_ENABLE_WERROR=ON` (default), `-DPANDA_ENABLE_DEBUG=ON/OFF`. Vendor deps (abseil/BuDDy/glaze/gzstream/mockturtle/or-tools/pugixml) are pulled from `ext/`.

Configure/build options of interest:
- `-DPANDA_MIN_CLANG_VERSION` / `-DPANDA_MAX_CLANG_VERSION` to narrow clang plugin probing (defaults 4.0.0 and 20.0.0); results are cached per build tree.
- `-DPANDA_CFG_VERBOSE_CLANG_PROBE=ON` for full plugin build/exec diagnostics; otherwise concise status lines.
- `-DPANDA_CFG_FORCE_CLANG_PROBE=ON` to re-run clang detection even if cached (default OFF to speed reconfigure).
- `-DPANDA_LIBBAMBU_COMPILER=I386_CLANG<ver>` to pin the libbambu compiler (e.g. `I386_CLANG16`).
- `-DPANDA_ENABLE_RELEASE=ON/OFF` sets release-style defines; OFF keeps assertions and disables `NDEBUG/NPROFILE`.
- `-DPANDA_ENABLE_ASSERTS=ON/OFF` toggles runtime assertions.
- `-DPANDA_ENABLE_OPT=ON/OFF` selects optimized flags (`-Ofast -march=native`) vs. mild `-O1`.
- `-DPANDA_ENABLE_DEBUG=ON/OFF` adds debug info (`-g3`).
- `-DPANDA_ENABLE_WERROR=ON/OFF` turns warnings into errors with the project’s warning set (default ON).
- `-DPANDA_SYNC_PARAMETER_METADATA=ON/OFF` controls whether builds rewrite `documentation/bambu_parameters_metadata.json`; keep it `OFF` for read-only source trees such as the containerized AppImage build.
- Clang plugins are built with the project C++ compiler but emulate each detected clang’s include paths and predefined macros.
- `-DPANDA_DIST_COMPILERS=\"clang-16,clang-13\"` optionally unpacks bundled compiler tarballs via `.devcontainer/library-scripts/compiler-download.sh`; regular installs place them under `<prefix>/compilers`.
- Install: `cmake --install build` (or `cmake --install build --prefix /opt/panda`) and optionally `DESTDIR=/tmp/pkg cmake --install build` to stage packages.
- Helper targets: `update-submodules`, `scan-build` (set `-DPANDA_SCAN_BUILD_VERSION=-11` to pick a clang; uses `compile_commands.json`), `package` (install/strip to `/tmp/bambuhls-<ver>` and tar).

# Panda-parameter documentation
Parameter documentation is managed via `documentation/bambu_parameters_metadata.json`. Add or update entries there (fields include `name`, `description`, `default_value`, `type`, `allowed_values`, `category`, `declared_in`). Discovery also scans `etc/libtech/*/*-seed.xml` `<device>` sections to surface device parameters. Normal builds keep the metadata file synchronized with the currently discovered parameter set and only rewrite it when the content actually changes. Do not edit `build/bambu_parameters_registry/*.cpp` inside `// BEGIN AUTOGEN` blocks; the registry is regenerated automatically during builds, or manually via:
```bash
etc/scripts/discover_bambu_parameters.py --build-dir build
etc/scripts/generate_bambu_parameter_registry.py \
  --input build/bambu_parameters_discovered.json \
  --metadata documentation/bambu_parameters_metadata.json \
  --output-dir build/bambu_parameters_registry
```
To force the same synchronization manually, rerun the generator with `--sync-metadata`:
```bash
etc/scripts/generate_bambu_parameter_registry.py \
  --input build/bambu_parameters_discovered.json \
  --metadata documentation/bambu_parameters_metadata.json \
  --output-dir build/bambu_parameters_registry \
  --sync-metadata
```

# Contacts
Issues, patches, and pull requests are welcome at https://github.com/ferrandi/PandA-bambu.<br>
For further information send an e-mail to panda-info@polimi.it, visit [PandA website](https://panda.dei.polimi.it/).

<br>

# Acknowledgements
Bambu has been supported throughout its history by the following projects.

European Union projects:
 - Grant agreement ID 004452 ICODES – Interface and Communication based Design of Embedded Systems
 - Grant agreement ID 035143 hArtes – Holistic Approach to Reconfigurable Real-Time Embedded Systems
 - Grant agreement ID 248538 Synaptic – SYNthesis using Advanced Process Technology Integrated in regular Cells, IPs, architectures, and design platforms
 - Grant agreement ID 287804 Faster – Facilitating Analysis and Synthesis Technologies for Effective Reconfiguration
 - Grant agreement ID 101004203 HERMES – qualification of High pErformance pRogrammable Microprocessor and dEvelopment of Software ecosystem
 - Grant agreement ID 957269 EVEREST – dEsign enVironmEnt foR Extreme-Scale big data analytics on heterogeneous platforms

European Space Agency contracts:
 - ESA/ESTEC/Contract N. 4000100797 – Development of methodologies and tools for predictable, real-time LEON-DSP-based embedded systems.
 - ESA/ESTEC/Contract No. 22167/09/NL/JK. Cache Optimization for LEON Analysis (COLA).
 - ESA/ESTEC/Contract Call-Off Order 4 “Multicore and Schedulability Analysis” for TASTE project.
 - ESA/ESTEC/Contract No. 4000121154/17/NL/LF Compact Reconfigurable Avionics Model-Based Avionic Design (CORA-MBAD)
