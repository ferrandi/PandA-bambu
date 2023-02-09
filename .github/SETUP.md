# Expected github runners setup

Github runners may be configured to run within a docker container or directly in the host environment. In the former case there may be some issues during the execution of external vendor tools which may not support a containerized environment.

## Runner labels
Runners may expose many different labels based on what tools are available on the host machine. Comments about docker volumes are intender for a containerized environment only.

- **altera**: Altera synthesis tools are available on the host machine and a docker volume named altera-tools exposes Altera tools install directories (e.g. Quartus, QuestaSim, ...) 
- **intel**: Intel synthesis tools are available on the host machine and a docker volume named intel-tools exposes modern Intel FPGA tools install directories (e.g. Quartus Prime, QuestaSim, ...)
- **lattice**: Lattice synthesis tools are available on the host machine and a docker volume named lattice-tools exposes Lattice tools install directories (e.g. Diamond, ...)
- **mentor**: Mentor Grahpics synthesis tools are available on the host machine and a docker volume named mentor-tools exposes Mentor Graphics tools install directories (e.g. ModelSim, QuestaSim, ...)
- **nanoxplore**: NanoXplore synthesis tools are available on the host machine and a docker volume named nanoxplore-tools exposes NanoXplore tools install directories
- **xilinx**: Xilinx synthesis tools are available on the host machine and a docker volume named xilinx-tools exposes Xilinx tools install directories (e.g. Vivado, Vitis HLS, ...)

If working with docker containers, directories containing license files should be copied or linked in container user home. Volumes may be defined as read-only to avoid issues.

## Environment variables
Some environment variables are expected to be set by each runner host:

- **J**: number of maximum parallel jobs handled by the runner
- **LM_LICENSE_FILE**: license file path for simulation/synthesis tools
- **NXLMD_LICENSE_FILE**: NanoXplore license file path (needed only if different from LM_LICENSE_FILE)
- **NANOXPLORE_BYPASS**: NanoXplore bypass setting
- **LIBRARY_PATH**: necessary to support older gcc compilers (set to: /usr/lib/x86_64-linux-gnu)

## Python support
Current CI implementation requires Python 3.6.15 to be available in the runner environment. Pyenv is recommended to provide the support.
Furthermore pip packages from `etc/scripts/requirements.txt` are required to run python scripts correctly.

Use the following to install the required Python version through PyEnv and set it as global default.

```
CONFIGURE_OPTS="--enable-shared" pyenv install 3.6.15
pyenv global 3.6.15
pip install -r /path/to/repo/etc/scripts/requirements.txt
```

Note that Github Runners are launched as systemd services, thus `~/.bashrc` or `~/.profile` are not loaded.
A `.path` file should be added in the runner directory containing standard _PATH_ variable prepended with PyEnv shims and bin paths.
