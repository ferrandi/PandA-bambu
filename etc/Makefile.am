SUBDIRS = devices macros scripts
DIST_SUBDIRS = devices macros scripts 

if BUILD_GCC_PLUGIN
   SUBDIRS += gcc_plugin clang_plugin
   DIST_SUBDIRS += gcc_plugin clang_plugin
endif

if BUILD_LIBBAMBU
   SUBDIRS += lib libbambu
   DIST_SUBDIRS += lib libbambu
endif

if BUILD_LATEX_TABLE_FORMAT
   SUBDIRS += latex_table_format
   DIST_SUBDIRS += latex_table_format
endif

EXTRA_DIST = Doxyfile \
             VMs/PandA-bambu-VM_32bit/Vagrantfile VMs/PandA-bambu-VM_32bit/script.sh \
             VMs/PandA-bambu-VM_64bit/Vagrantfile VMs/PandA-bambu-VM_64bit/script.sh \
             VMs/PandA-bambu-VM_64bit-ArchLinux/Vagrantfile VMs/PandA-bambu-VM_64bit-ArchLinux/script.sh \
             VMs/PandA-bambu-VM_64bit-CentOS7/script.sh VMs/PandA-bambu-VM_64bit-CentOS7/Vagrantfile \
             VMs/PandA-bambu-VM_64bit-fedora29/script.sh VMs/PandA-bambu-VM_64bit-fedora29/Vagrantfile\
             VMs/PandA-bambu-VM_64bit-macport/script.sh VMs/PandA-bambu-VM_64bit-macport/Vagrantfile\
             VMs/PandA-bambu-VM_64bit-precise/script.sh VMs/PandA-bambu-VM_64bit-precise/Vagrantfile \
             VMs/PandA-bambu-VM_64bit-trusty/script.sh VMs/PandA-bambu-VM_64bit-trusty/Vagrantfile \
             VMs/PandA-bambu-VM_32bit-Debian9/script.sh VMs/PandA-bambu-VM_32bit-Debian9/Vagrantfile \
             VMs/PandA-bambu-VM_64bit-xenial/script.sh VMs/PandA-bambu-VM_64bit-xenial/Vagrantfile

.PHONY documentation: documentation-recursive
.PHONY documentation-tag:documentation-tag-recursive
documentation:
documentation-tag:

