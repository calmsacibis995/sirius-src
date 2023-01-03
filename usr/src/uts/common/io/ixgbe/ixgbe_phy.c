/*
 * CDDL HEADER START
 *
 * Copyright(c) 2007-2008 Intel Corporation. All rights reserved.
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at:
 *      http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When using or redistributing this file, you may do so under the
 * License only. No other modification of this header is permitted.
 *
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms of the CDDL.
 */

/* IntelVersion: 1.37 v2008-03-04 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include "ixgbe_api.h"
#include "ixgbe_common.h"
#include "ixgbe_phy.h"

/*
 * ixgbe_init_phy_ops_generic - Inits PHY function ptrs
 * @hw: pointer to the hardware structure
 *
 * Initialize the function pointers.
 */
s32
ixgbe_init_phy_ops_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_phy_info *phy = &hw->phy;

	/* PHY */
	phy->ops.identify = &ixgbe_identify_phy_generic;
	phy->ops.reset = &ixgbe_reset_phy_generic;
	phy->ops.read_reg = &ixgbe_read_phy_reg_generic;
	phy->ops.write_reg = &ixgbe_write_phy_reg_generic;
	phy->ops.setup_link = &ixgbe_setup_phy_link_generic;
	phy->ops.setup_link_speed = &ixgbe_setup_phy_link_speed_generic;

	return (IXGBE_SUCCESS);
}

/*
 * ixgbe_identify_phy_generic - Get physical layer module
 * @hw: pointer to hardware structure
 *
 * Determines the physical layer module found on the current adapter.
 */
s32
ixgbe_identify_phy_generic(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_ERR_PHY_ADDR_INVALID;
	u32 phy_addr;

	if (hw->phy.type == ixgbe_phy_unknown) {
		for (phy_addr = 0; phy_addr < IXGBE_MAX_PHY_ADDR; phy_addr++) {
			if (ixgbe_validate_phy_addr(hw, phy_addr)) {
				hw->phy.addr = phy_addr;
				(void) ixgbe_get_phy_id(hw);
				hw->phy.type =
				    ixgbe_get_phy_type_from_id(hw->phy.id);
				status = IXGBE_SUCCESS;
				break;
			}
		}
	} else {
		status = IXGBE_SUCCESS;
	}

	return (status);
}

/*
 * ixgbe_validate_phy_addr - Determines phy address is valid
 * @hw: pointer to hardware structure
 *
 */
bool
ixgbe_validate_phy_addr(struct ixgbe_hw *hw, u32 phy_addr)
{
	u16 phy_id = 0;
	bool valid = FALSE;

	hw->phy.addr = phy_addr;
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_HIGH,
	    IXGBE_MDIO_PMA_PMD_DEV_TYPE, &phy_id);

	if (phy_id != 0xFFFF && phy_id != 0x0)
		valid = TRUE;

	return (valid);
}

/*
 * ixgbe_get_phy_id - Get the phy type
 * @hw: pointer to hardware structure
 *
 */
s32
ixgbe_get_phy_id(struct ixgbe_hw *hw)
{
	u32 status;
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;

	status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_HIGH,
	    IXGBE_MDIO_PMA_PMD_DEV_TYPE,
	    &phy_id_high);

	if (status == IXGBE_SUCCESS) {
		hw->phy.id = (u32)(phy_id_high << 16);
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_LOW,
		    IXGBE_MDIO_PMA_PMD_DEV_TYPE,
		    &phy_id_low);
		hw->phy.id |= (u32)(phy_id_low & IXGBE_PHY_REVISION_MASK);
		hw->phy.revision = (u32)(phy_id_low & ~IXGBE_PHY_REVISION_MASK);
	}

	return (status);
}

/*
 * ixgbe_get_phy_type_from_id - Get the phy type
 * @hw: pointer to hardware structure
 *
 */
enum ixgbe_phy_type
ixgbe_get_phy_type_from_id(u32 phy_id)
{
	enum ixgbe_phy_type phy_type;

	switch (phy_id) {
	case QT2022_PHY_ID:
		phy_type = ixgbe_phy_qt;
		break;
	default:
		phy_type = ixgbe_phy_unknown;
		break;
	}

	return (phy_type);
}

/*
 * ixgbe_reset_phy_generic - Performs a PHY reset
 * @hw: pointer to hardware structure
 */
s32
ixgbe_reset_phy_generic(struct ixgbe_hw *hw)
{
	/*
	 * Perform soft PHY reset to the PHY_XS.
	 * This will cause a soft reset to the PHY
	 */
	return hw->phy.ops.write_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	    IXGBE_MDIO_PHY_XS_DEV_TYPE,
	    IXGBE_MDIO_PHY_XS_RESET);
}

/*
 * ixgbe_read_phy_reg_generic - Reads a value from a specified PHY register
 * @hw: pointer to hardware structure
 * @reg_addr: 32 bit address of PHY register to read
 * @phy_data: Pointer to read data from PHY register
 */
s32
ixgbe_read_phy_reg_generic(struct ixgbe_hw *hw, u32 reg_addr,
    u32 device_type, u16 *phy_data)
{
	u32 command;
	u32 i;
	u32 data;
	s32 status = IXGBE_SUCCESS;
	u16 gssr;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		gssr = IXGBE_GSSR_PHY1_SM;
	else
		gssr = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, gssr) != IXGBE_SUCCESS)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == IXGBE_SUCCESS) {
		/* Setup and write the address cycle command */
		command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
		    (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
		    (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
		    (IXGBE_MSCA_ADDR_CYCLE | IXGBE_MSCA_MDI_COMMAND));

		IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

		/*
		 * Check every 10 usec to see if the address cycle completed.
		 * The MDI Command bit will clear when the operation is
		 * complete
		 */
		for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
			usec_delay(10);

			command = IXGBE_READ_REG(hw, IXGBE_MSCA);

			if ((command & IXGBE_MSCA_MDI_COMMAND) == 0) {
				break;
			}
		}

		if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
			DEBUGOUT("PHY address command did not complete.\n");
			status = IXGBE_ERR_PHY;
		}

		if (status == IXGBE_SUCCESS) {
			/*
			 * Address cycle complete, setup and write the read
			 * command
			 */
			command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
			    (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
			    (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
			    (IXGBE_MSCA_READ | IXGBE_MSCA_MDI_COMMAND));

			IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

			/*
			 * Check every 10 usec to see if the address cycle
			 * completed. The MDI Command bit will clear when the
			 * operation is complete
			 */
			for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
				usec_delay(10);

				command = IXGBE_READ_REG(hw, IXGBE_MSCA);

				if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
					break;
			}

			if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
				DEBUGOUT("PHY read command didn't complete\n");
				status = IXGBE_ERR_PHY;
			} else {
				/*
				 * Read operation is complete.  Get the data
				 * from MSRWD
				 */
				data = IXGBE_READ_REG(hw, IXGBE_MSRWD);
				data >>= IXGBE_MSRWD_READ_DATA_SHIFT;
				*phy_data = (u16)(data);
			}
		}

		ixgbe_release_swfw_sync(hw, gssr);
	}

	return (status);
}

/*
 * ixgbe_write_phy_reg_generic - Writes a value to specified PHY register
 * @hw: pointer to hardware structure
 * @reg_addr: 32 bit PHY register to write
 * @device_type: 5 bit device type
 * @phy_data: Data to write to the PHY register
 */
s32 ixgbe_write_phy_reg_generic(struct ixgbe_hw *hw, u32 reg_addr,
    u32 device_type, u16 phy_data)
{
	u32 command;
	u32 i;
	s32 status = IXGBE_SUCCESS;
	u16 gssr;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		gssr = IXGBE_GSSR_PHY1_SM;
	else
		gssr = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, gssr) != IXGBE_SUCCESS)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == IXGBE_SUCCESS) {
		/*
		 * Put the data in the MDI single read and write data register
		 */
		IXGBE_WRITE_REG(hw, IXGBE_MSRWD, (u32)phy_data);

		/* Setup and write the address cycle command */
		command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
		    (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
		    (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
		    (IXGBE_MSCA_ADDR_CYCLE | IXGBE_MSCA_MDI_COMMAND));

		IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

		/*
		 * Check every 10 usec to see if the address cycle completed.
		 * The MDI Command bit will clear when the operation is
		 * complete
		 */
		for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
			usec_delay(10);

			command = IXGBE_READ_REG(hw, IXGBE_MSCA);

			if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
				break;
		}

		if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
			DEBUGOUT("PHY address cmd didn't complete\n");
			status = IXGBE_ERR_PHY;
		}

		if (status == IXGBE_SUCCESS) {
			/*
			 * Address cycle complete, setup and write the write
			 * command
			 */
			command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
			    (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
			    (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
			    (IXGBE_MSCA_WRITE | IXGBE_MSCA_MDI_COMMAND));

			IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

			/*
			 * Check every 10 usec to see if the address cycle
			 * completed. The MDI Command bit will clear when the
			 * operation is complete
			 */
			for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
				usec_delay(10);

				command = IXGBE_READ_REG(hw, IXGBE_MSCA);

				if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
					break;
			}

			if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
				DEBUGOUT("PHY address cmd didn't complete\n");
				status = IXGBE_ERR_PHY;
			}
		}

		ixgbe_release_swfw_sync(hw, gssr);
	}

	return (status);
}

/*
 * ixgbe_setup_phy_link_generic - Set and restart autoneg
 * @hw: pointer to hardware structure
 *
 * Restart autonegotiation and PHY and waits for completion.
 */
s32
ixgbe_setup_phy_link_generic(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_NOT_IMPLEMENTED;
	u32 time_out;
	u32 max_time_out = 10;
	u16 autoneg_reg = IXGBE_MII_AUTONEG_REG;

	/*
	 * Set advertisement settings in PHY based on autoneg_advertised
	 * settings. If autoneg_advertised = 0, then advertise default values
	 * tnx devices cannot be "forced" to a autoneg 10G and fail.  But can
	 * for a 1G.
	 */
	hw->phy.ops.read_reg(hw, IXGBE_MII_SPEED_SELECTION_REG,
	    IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &autoneg_reg);

	if (hw->phy.autoneg_advertised == IXGBE_LINK_SPEED_1GB_FULL)
		autoneg_reg &= 0xEFFF; /* 0 in bit 12 is 1G operation */
	else
		autoneg_reg |= 0x1000; /* 1 in bit 12 is 10G/1G operation */

	hw->phy.ops.write_reg(hw, IXGBE_MII_SPEED_SELECTION_REG,
	    IXGBE_MDIO_AUTO_NEG_DEV_TYPE, autoneg_reg);

	/* Restart PHY autonegotiation and wait for completion */
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	    IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &autoneg_reg);

	autoneg_reg |= IXGBE_MII_RESTART;

	hw->phy.ops.write_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	    IXGBE_MDIO_AUTO_NEG_DEV_TYPE, autoneg_reg);

	/* Wait for autonegotiation to finish */
	for (time_out = 0; time_out < max_time_out; time_out++) {
		usec_delay(10);
		/* Restart PHY autonegotiation and wait for completion */
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_STATUS,
		    IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		    &autoneg_reg);

		autoneg_reg &= IXGBE_MII_AUTONEG_COMPLETE;
		if (autoneg_reg == IXGBE_MII_AUTONEG_COMPLETE) {
			status = IXGBE_SUCCESS;
			break;
		}
	}

	if (time_out == max_time_out)
		status = IXGBE_ERR_LINK_SETUP;

	return (status);
}

/*
 * ixgbe_setup_phy_link_speed_generic - Sets the auto advertised capabilities
 * @hw: pointer to hardware structure
 * @speed: new link speed
 * @autoneg: TRUE if autonegotiation enabled
 */
s32 ixgbe_setup_phy_link_speed_generic(struct ixgbe_hw *hw,
    ixgbe_link_speed speed,
    bool autoneg,
    bool autoneg_wait_to_complete)
{
	UNREFERENCED_PARAMETER(autoneg);
	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);

	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = 0;

	if (speed & IXGBE_LINK_SPEED_10GB_FULL) {
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_10GB_FULL;
	}
	if (speed & IXGBE_LINK_SPEED_1GB_FULL) {
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_1GB_FULL;
	}

	/* Setup link based on the new speed settings */
	hw->phy.ops.setup_link(hw);

	return (IXGBE_SUCCESS);
}
