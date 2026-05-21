// SCCB write address
#define SCCB_REG_ADDR 0x01

// OV7670 camera settings
#define OV7670_REG_NUM 121 
#define OV7670_WRITE_ADDR 0x42
#define OV7670_READ_ADDR 0x43
// Image settings
#define QCIF_ROWS 144
#define QCIF_COLUMNS 176
#define QVGA_ROWS 240
#define QVGA_COLUMNS 320

const uint8_t OV7670_QCIF_UYVY[][2] = {
	//Set QCIF resolution (COM7[3] = 1)
	//Set YUV 4:2:2 (COM7[2] and COM7[0] = 0)
	//Default COM7 is 0b00000000
	{0x12, 0b00001000},

	//Set YUV output: UYVY 
	//Set TSLB[3] = 1, COM13[0] = 0
	//Default TSLB value is 0b00001101
	//Default COM31 value is 0b10001000
	{0x3A, 0b00001101},
	{0x3D, 0b10001000}, 

	//Disable auto exposure (AGC, AEC, AWB)
	{0x13, 0x8F &(~0b00000111)},
	//Set Exposure Value
	{0x10, 0x28}, //AEC[9:2]
	{0x04, 0x02}, //AEC[1:0]
	{0x07, 0x00}, //AEC[15:10]

	//Move H window slightly further, set HREF edge offset to 3
	// {0x32, 0b11000010},

 	//Flip image vertically
	{0x1E, 0x31},
};

//For 8 color bar SCALING_XSC[7] = 0, SCALING_YSC[7]=1 (different to what datasheet says)
//SCALING_XSC default value is 0b00111010
//SCALING_YSC default value is 0b00110101
const uint8_t OV7670_TestPattern[][2] = {
	{0x70, 0b00111010},
	{0x71, 0b10110101},
};