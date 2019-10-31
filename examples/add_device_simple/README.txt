This example shows how to add a non-supported device to the bambu synthesis flow.
The file xc7z045-2ffg900-VVD.xml has copied from the framework distribution etc/devices/Xilinx_devices/xc7z020-1clg484-VVD.xml and then renamed in xc7z045-2ffg900-VVD.xml.
After copying the file few changes have been made. All of them relates to the new device characteristics: model, package and speed grade.
Here it follows the changed part of the xml file:
    <model value="xc7z045"/>
    <package value="ffg900"/>
    <speed_grade value="-2"/>

Note that the field 
    <family value="Zynq-VVD"/>
refers to the synthesis script stored in etc/devices/Xilinx_devices/Zynq-VVD.xml.
So, the bambu.sh will first simulate and then synthesize the C based description using the above specified Zynq device.

Note that, this example shows another nice feature of the HLS framework. The file module.c contains the C specification of the factorial function in its recursive form.
bambu is not actually able to synthesize recursive functions but GCC is able to automatically translate it in its non-recursive form once -O2 option is passed. To understand what exactly 
has been synthesized please check the a.c in the add_device_simple_sim or add_device_simple_synth directory created by bambu.sh.
This example shows how to add a new device very similar to one of the already available XML characterizations. In case the device is not very similar to one of the already characterized devices, the user should check and accordingly add the characterization scripts. Example of characterization scripts based on eucalyptus tool are available in etc/devices.
Note that, eucalyptus is automatically built once a RTL synthesis backend is configured.


