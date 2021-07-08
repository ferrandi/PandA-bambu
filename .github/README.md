# Expected github runners setup

## Job configuration
A worflow job may be configured to run within a docker machine or directly in the host environment, in the latter case be aware that Bambu distributions coming from other jobs as artifacts may need the _APPDIR_ variable to be set to the new install location.

## Runner labels
Runners may expose many different labels based on what tools are available on the host machine.

- **altera**: Altera synthesis tools are available on the host machine and a docker volume named altera-tools exposes Altera tools install directories (e.g. Quartus, QuestaSim, ...) 
- **intel**: Intel synthesis tools are available on the host machine and a docker volume named intel-tools exposes modern Intel FPGA tools install directories (e.g. Quartus Prime, QuestaSim, ...)
- **lattice**: Lattice synthesis tools are available on the host machine and a docker volume named lattice-tools exposes Lattice tools install directories (e.g. Diamond, ...)
- **mentor**: Mentor Grahpics synthesis tools are available on the host machine and a docker volume named mentor-tools exposes Mentor Graphics tools install directories (e.g. ModelSim, QuestaSim, ...)
- **nanoxplore**: NanoXplore synthesis tools are available on the host machine and a docker volume named nanoxplore-tools exposes NanoXplore tools install directories
- **xilinx**: Xilinx synthesis tools are available on the host machine and a docker volume named xilinx-tools exposes Xilinx tools install directories (e.g. Vivado, Vitis HLS, ...)

Directories containing license files should be copied or linked in container user home. Volumes may be defined as read-only to avoid issues.

A **licenses-home** volume is always expected when at least one of the above is defined. It should contain all necessary license files for available tools. Workflow job is expected to copy the volume content into the home directory of current user when running a container.

To create such volumes the following may be useful:

```
docker volume create --driver local --opt o=bind,ro --opt type=none --opt device=/path/to/dir vendor-tools
```