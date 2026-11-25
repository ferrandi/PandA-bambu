# MNIST benchmark provenance

The matrix/image/neural-network routines in this adapted benchmark originate
from [Mark Kraay's mnist-from-scratch](https://github.com/markkraay/mnist-from-scratch),
verified against commit `5a36f92c841e06e13fa620dd814df8f00ebf5ea6`.
The upstream [MIT license](https://github.com/markkraay/mnist-from-scratch/blob/5a36f92c841e06e13fa620dd814df8f00ebf5ea6/LICENSE)
is reproduced in `LICENSE.markkraay`. Upstream `matrix/matrix.c`, `matrix/ops.c`,
`neural/nn.c` and the associated headers contain the matrix allocation,
randomization, arithmetic, prediction and training routines adapted here.

PandA adaptations flatten matrix storage, use fixed-size float buffers, add
static OpenMP tasks and bank/AXI directives, and supply deterministic synthetic
inputs and host/RTL checks. Existing project adaptations retain their
Apache-2.0 WITH LLVM-exception notice; that notice does not replace the upstream
MIT copyright and permission notice.

The CSV dataset has separate provenance and licensing in `dataset/README.md`.
The upstream project README itself points to `oddrationale/mnist-in-csv` on Kaggle;
the numerical rows here also match the pinned public GitHub mirror documented
in the dataset README.
