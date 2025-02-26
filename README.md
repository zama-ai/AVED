# AVED

Full documentation for the ALVEO Versal Example Design can be found at the following link: 
[xilinx.github.io/AVED/](https://xilinx.github.io/AVED/)

# Why this fork?

The TFHE accelerator developed by Zama is running on [Alveo V80 board](https://www.amd.com/en/products/accelerators/alveo/v80.html) and is today using the AMI driver to:
- manage the V80 board: load flash, read sensors...
- read and write registers of this accelerator:
  - added peek & poke commands to ami char device, see [sw/AMI/driver/ami_cdev.h](/sw/AMI/driver/ami_cdev.h)
  - these 2 commands are generating GCQ commands to read & write our accelerator registers
  - [sw/AMI/driver/ami_peekpoke.c](/sw/AMI/driver/ami_peekpoke.c)
  - [sw/AMI/api/src/ami_peek_poke_access.c](/sw/AMI/api/src/ami_peek_poke_access.c)
  - [sw/AMI/app/cmd_handlers/cmd_peek.c](/sw/AMI/app/cmd_handlers/cmd_peek.c)
- push instructions to this accelerator:
  - added iop push command to ami char device, see [sw/AMI/driver/ami_cdev.h](/sw/AMI/driver/ami_cdev.h)
  - this command is pushing accelerator instructions in a queue located in DDR of V80
  - [sw/AMI/driver/ami_iop_push.c](/sw/AMI/driver/ami_iop_push.c)

The small additions we did on the ami_tool application and the AMI driver are related to these two latest objectives. We believe that none of these additions, very specific to our accelerator, deserve to be integrated in original AVED repository.

As this AMI driver and the associated API & application (ami_tool) are GPL-2.0-only, we decided to keep these in a separate repository.
When running the Zama TFHE accelerator on V80 FPGA board, only binary compilation results from this repository are used.
