// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_vsec.c - This file contains logic to parse PCI XILINX VSEC.
 *
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "ami_vsec.h"
#include "ami_pci_dbg.h"

/* VSEC is only applicable for xilinx-vendor boards
 * No need to check the vendor ID is PCIE_VENDOR_ID_XILINX here prior to VSEC
 * discovery as only Xilinx card are used in MODULE_DEVICE_TABLE */

int read_logic_uuid(struct pci_dev *dev, endpoints_struct **endpoints)
{
	int ret = 0;
	int i = 0;
	void __iomem *virt_addr = NULL;

	if (!dev || !endpoints)
		return -EINVAL;

	if (!(*endpoints)->uuid0_rom.found) {
		DEV_ERR(dev, "Endpoint %s not found in HW discovery",
			XILINX_ENDPOINT_NAME_UUID0_ROM);
		ret = -ENODEV;
		goto fail;
	}

	ret = pci_request_region(dev, (*endpoints)->uuid0_rom.bar_num,
			PCIE_BAR_NAME[(*endpoints)->uuid0_rom.bar_num]);
	if (ret) {
		DEV_ERR(dev, "Could not request %s region (%s)",
			PCIE_BAR_NAME[(*endpoints)->uuid0_rom.bar_num],
			(*endpoints)->uuid0_rom.name);
		ret = -EIO;
		goto fail;
	}

	virt_addr = pci_iomap_range(dev,
		(*endpoints)->uuid0_rom.bar_num,
		(*endpoints)->uuid0_rom.start_addr,
		(*endpoints)->uuid0_rom.bar_len);

	if (!virt_addr) {
		DEV_ERR(dev, "Could not map %s endpoint into virtual memory at start address 0x%llX",
			(*endpoints)->uuid0_rom.name, (*endpoints)->uuid0_rom.start_addr);
		ret = -EIO;
		goto release_bar;
	}

	(*endpoints)->logic_uuid_str[0] = '\0';
	for (i = ARRAY_SIZE((*endpoints)->logic_uuid) - 1; i >= 0; i--) {
		(*endpoints)->logic_uuid[i] = \
			ioread32(virt_addr + sizeof(uint32_t) * i);

		sprintf((*endpoints)->logic_uuid_str + \
			strlen((*endpoints)->logic_uuid_str),
			"%08x", (*endpoints)->logic_uuid[i]);
	}

	DEV_INFO(dev, "Logic uuid = %s", (*endpoints)->logic_uuid_str);
	pci_iounmap(dev, virt_addr);
	pci_release_region(dev, (*endpoints)->uuid0_rom.bar_num);

	return SUCCESS;

release_bar:
	pci_release_region(dev, (*endpoints)->uuid0_rom.bar_num);

fail:
	DEV_ERR(dev, "Failed to read logic UUID");
	return ret;
}

int read_vsec(struct pci_dev *dev, uint32_t vsec_base_addr,
	endpoints_struct **endpoints)
{
	int ret = 0;
	int i = 0;
	bool end_of_table = false;
	uint32_t table_length = 0, table_entry_size = 0;
	uint8_t ep_bar_num = 0;
	uint8_t pcie_function_num = 0;

	if (!dev || !endpoints)
		return -EINVAL;

	pcie_function_num = PCI_FUNC(dev->devfn);

	DEV_VDBG(dev, "Reading vendor specific information for PF %d",
		pcie_function_num);

	(*endpoints) = kzalloc(sizeof(endpoints_struct), GFP_KERNEL);
	if (!(*endpoints)) {
		DEV_ERR(dev, "Failed to allocate memory for endpoints");
		ret = -ENOMEM;
		goto fail;
	}

	/*
	 * Traverse the Xilinx Capabilities
	 *
	 * Xilinx Capabilities Format
	 *  ------------------------------------------------------------------------------
	 * | Rsvd[31:29] | Last Capability[28] | Format Revision[27:20] | Format ID[19:0] |
	 *  ------------------------------------------------------------------------------
	 * |                        Length[31:0]                                          |
	 *  ------------------------------------------------------------------------------
	 * | Rsvd[31:8]                           |       Entry Size[7:0]                 |
	 *  ------------------------------------------------------------------------------
	 * |                        Rsvd[31:0]                                            |
	 *  ------------------------------------------------------------------------------
	 *
	 *  @Format ID -
	 *  0x0 - Reserved and should not be used
	 *  0x1 - BAR Layout Register Format (Currently, only valid value format)
	 *  0xEF100 - Host Interface (Reserved)
	 *  0xFFFFE - Continued Capabilities Address
	 *  0xFFFFF - Termination of the list of capabilities
	 *
	 *  @Format Revision - 1 (fixed)
	 *
	 *  @Entry Size -
	 *  0x08 - 8 byte entry size
	 *  0x10 - 16 byte entry size
	 *  0x20 - 32 byte entry size
	 *  0x40 - 64 byte entry size
	 *  0x80 - 128 byte entry size
	 */
	table_length = 0x40;
	table_entry_size = 0x10;

	/*
	 *                                         Table Entry
	 *
	 *      -----------------------------------------------------------------------------------------
	 *     |    Low Addr Offset[31:16]   |  Bar Number[15:13]   |  Type Revision[12:8]  | Type[7:0]  |
	 *      -----------------------------------------------------------------------------------------
	 *     |                                High Addr Offset[31:0]                                   |
	 *      -----------------------------------------------------------------------------------------
	 *     | Rsvd[31:24]        |  Major[23:16]       | Minor[15:8]       | Version Type[7:0]        |
	 *      -----------------------------------------------------------------------------------------
	 *     |                                Rsvd[31:0]                                               |
	 *      -----------------------------------------------------------------------------------------
	 */
	int ep_type[3]= {0x50,0x54,0x55};
	int ep_start_addr[3] = {0x1001000, 0x1010000,0x8000000};
	int index = 0;

	/* Traverse all the table entry */
	for (i = 0; i < table_length; i += table_entry_size) {
		if (end_of_table)
			break;


		ep_bar_num  = 0x0; /* BAR Num where the target aperture is located */

		switch(ep_type[index]) {
		case XILINX_TABLE_TYPE_UUID0_ROM:
			(*endpoints)->uuid0_rom.found = \
				true;

			(*endpoints)->uuid0_rom.bar_num = \
				ep_bar_num;

			(*endpoints)->uuid0_rom.start_addr = ep_start_addr[0];

			(*endpoints)->uuid0_rom.bar_len = \
				XILINX_ENDPOINT_BAR_LEN_UUID0_ROM;

			(*endpoints)->uuid0_rom.end_addr = \
				(*endpoints)->uuid0_rom.start_addr +
				(*endpoints)->uuid0_rom.bar_len - 1;

			strcpy((*endpoints)->uuid0_rom.name,
				XILINX_ENDPOINT_NAME_UUID0_ROM);

			print_endpoint_info(dev, (*endpoints)->uuid0_rom);
			break;

		case XILINX_TABLE_TYPE_GCQ:
			(*endpoints)->gcq.found = \
				true;

			(*endpoints)->gcq.bar_num = \
				ep_bar_num;

			(*endpoints)->gcq.start_addr = \
				ep_start_addr[1];

			(*endpoints)->gcq.bar_len = \
				XILINX_ENDPOINT_BAR_LEN_GCQ;

			(*endpoints)->gcq.end_addr = \
				(*endpoints)->gcq.start_addr +
				(*endpoints)->gcq.bar_len - 1;

			strcpy((*endpoints)->gcq.name, XILINX_ENDPOINT_NAME_GCQ);
			print_endpoint_info(dev, (*endpoints)->gcq);
			break;

		case XILINX_TABLE_TYPE_GCQ_PAYLOAD:
			(*endpoints)->gcq_payload.found = \
				true;

			(*endpoints)->gcq_payload.bar_num = \
				ep_bar_num;

			(*endpoints)->gcq_payload.start_addr = \
				ep_start_addr[2];

			(*endpoints)->gcq_payload.bar_len = \
				XILINX_ENDPOINT_BAR_LEN_GCQ_PAYLOAD;

			(*endpoints)->gcq_payload.end_addr = \
				(*endpoints)->gcq_payload.start_addr +
				(*endpoints)->gcq_payload.bar_len - 1;

			strcpy((*endpoints)->gcq_payload.name,
				XILINX_ENDPOINT_NAME_GCQ_PAYLOAD);
			print_endpoint_info(dev, (*endpoints)->gcq_payload);
			break;

		case XILINX_TABLE_TYPE_END_OF_TABLE:
			end_of_table = true;
			DEV_VDBG(dev, "End of table");
			break;

		default:
			DEV_ERR(dev,
				"Found Unsupported or Reserved Type Endpoint: 0x%X",
				ep_type);
			ret = -EINVAL;
			goto fail;
			break;
		}
	}

	ret = read_logic_uuid(dev, endpoints);
	if (ret)
		goto fail;

	DEV_VDBG(dev, "Successfully read Vendor Specific Region (VSEC)");
	return SUCCESS;

fail:
	DEV_ERR(dev, "Failed to read Vendor Specific Region (VSEC)");
	return ret;
}

void release_endpoints(endpoints_struct **endpoints)
{
	if (endpoints && *endpoints) {
		kfree(*endpoints);
		*endpoints = NULL;
	}
}

void release_vsec_mem(endpoints_struct **endpoints)
{
	if (!endpoints)
		release_endpoints(endpoints);
}
