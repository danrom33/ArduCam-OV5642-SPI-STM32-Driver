#ifndef __ARDUCHIP_H
#define __ARDUCHIP_H

#include "spi_cam.h"

OV5642_StatusTypeDef ArduChip_write_reg(uint8_t reg_addr, uint8_t val);
OV5642_StatusTypeDef ArduChip_read_reg(uint8_t reg_addr, uint8_t *val);
OV5642_StatusTypeDef ArduChip_read_fifo(uint32_t length);

static void CS_Select();
static void CS_Deselect();

#endif /*__ARDUCHIP_H*/