# **Adding support for a new target device**

Here is an example of how users can add a new target device in Bambu, following the steps that were needed to support a Xilinx Kintex Ultrascale FPGA. The process is based on the *characterization* of library components on the new target in order to inform the allocation and scheduling steps in the HLS process.

## **Step 1 - Create a seed XML file**

Create an XML file in [etc/libtech/xilinx/](../../../etc/libtech/xilinx) with the code of the target board in Bambu style, the  suffix to indicate that logic synthesis is to be performed through Vivado, and the -seed suffix. In this case the file name will be [xcku060-3ffva1156-seed.xml](xcku060-3ffva1156-seed.xml).

The seed XML file must contain the following fields, which identify the specific FPGA:
* Vendor: Xilinx
* Family: Kintex-Ultrascale
* Model: xcku060
* Package: -ffva1156
* Speed: -3-e

The following XML fields, instead, are used to describe characteristics of the FPGA fabric that can be found in the board documentation:
* Number of inputs to a LUT (max_lut_size): 6
* Bitwidth of the inputs to the multiplier in a DSP slice (DSPs_x and y_sizes): 18, 27
* Default Vivado support for unaligned BRAM accesses (BRAM_bitsize_min and max): 8, 4096

## **Step 2 - Create/Update synthesis backend flow**

If this is the first board in a new Xilinx family, it might need a specific backend script for synthesis and implementation. For the Kintex Ultrascale family, for example, it was necessary to adjust how LUTs and registers are extracted from Vivado utilization reports. A new version of the *dump_statistics* TCL function was added to [etc/libtech/backend/xilinx/](../../../etc/libtech/xilinx) and synthesis scripts where updated to use the correct version based on device family [etc/libtech/backend/xilinx/flow/vivado.tcl](../../../etc/libtech/backend/xilinx/flow/vivado.tcl)

## **Step 3 - Copy sample characterization results**

It is useful to make sure that the new target was addedd correctly before running the characterization tool, as characterization is a long process. To do this, create an XML file copying characterization results from a similar board in [etc/libtech/xilinx/](../../../etc/libtech/xilinx) (see [xcku060-3ffva1156.xml](xcku060-3ffva1156.xml), copied from a Virtex7 target).

## **Step 4 - Add the new target to the list of available devices**

Add seed and sample characterization in [etc/libtech/xilinx/Makefile.am](../../../etc/libtech/xilinx/Makefile.am)

**The process might end here:** if the new board is very similar to one of the supported ones, old characterization results can be reused directly. Try it out by launching Bambu on a test file and selecting the new target device.

## **Step 5 - Run Eucalyptus to characterize the new target**

Eucalyptus is a tool that supports the characterization of new targets for Bambu, and it is available in the default Bambu installation. Run the [characterize_device](../../../etc/libtech/characterize_device.sh) script to launch Eucalyptus:

`bash characterize_device.sh --devices xcku060-3ffva1156`

Note: this can take more than a day, since the script has to launch multiple logic synthesis and implementation runs. You can add the -j option to use multiple cores in parallel.

Once the characterization is complete, Eucalyptus will generate a new xcku060-3ffva1156.xml file that will replace the sample one created in Step 3.
