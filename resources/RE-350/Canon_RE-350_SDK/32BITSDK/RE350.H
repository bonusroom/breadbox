/***************************************************************************

	FILE:		re350.h

	AUTHOR:		Eric Chasanoff

	REVISIONS:	08.15.96	Written

	DESCRIPTION:
			RE-350 driver definitions.
	
	DISCLAIMER:

		Raster Builders and Eric Chasanoff make no representations
		about the correctness or suitability of this software for 
		any purpose. It is provided without express or implied 
		warranty.

***************************************************************************/

#ifndef _RE350_DEFINED_
#define _RE350_DEFINED_

// ---------------------------------------------
//	Driver definitions
// ---------------------------------------------

#define	RE350_LIBNAME		"RE350.DLL"

#define	RE350_COM1	1
#define	RE350_COM2	2
#define	RE350_COM3	3
#define	RE350_COM4	4

#define	RE350_SEND_TMO		500	// Send char timeout
#define	RE350_CHAR_TMO		2000L	// Timeout for character
#define	RE350_PEEK_RETRY	10	// Number of PeekMessage calls
#define	RE350_CLOSE_DELAY	250	// Wait before closing driver

#define	RE350_C_RESPONSE	0xA0
#define	RE350_E_RESPONSE	0x20
#define	RE350_FP_RESPONSE	0x30

#define	RE350_RESPONSE_LEN	5
#define	RE350_REQUEST_LEN	5

// ---------------------------------------------
//	RE-350 Commands
// ---------------------------------------------

typedef struct
{
	BYTE	byHeader;
	BYTE	byID;
	BYTE	byCmd;
	BYTE	byParamH;
	BYTE	byParamL;

} RE350_HEADER, FAR * LPRE350_HEADER;

typedef struct
{
	DWORD			dwMaxMs;
	RE350_HEADER	header;

} RE350_CMDS, FAR * LPRE350_CMDS;

// -----------------------------------------
//	Error codes
// -----------------------------------------

#define	RE350_ERR_NONE			(0)
#define	RE350_ERR_INIT_FORMAT	(-1)
#define	RE350_ERR_INIT_OPEN		(-2)
#define	RE350_ERR_INIT_HARDWARE	(-3)
#define	RE350_ERR_INIT_SET		(-4)
#define	RE350_ERR_DRIVER_INIT	(-5)
#define	RE350_ERR_SEND_TMO		(-6)
#define	RE350_ERR_RECV_TMO		(-7)
#define	RE350_ERR_RECV_IO		(-8)
#define	RE350_ERR_SEND_IO		(-9)
#define	RE350_ERR_BUSY			(-10)
#define	RE350_ERR_MODE			(-11)
#define	RE350_ERR_CMD			(-12)
#define	RE350_ERR_PARAM			(-13)
#define	RE350_ERR_INVALID_CMD	(-14)
#define	RE350_ERR_MEMORY		(-15)
#define	RE350_ERR_IDLE_TIMEOUT	(-16)
#define	RE350_ERR_UNKNOWN		(-17)
#define	RE350_ERR_SYSTEM		(-18) 
#define	RE350_ERR_TIMEOUT		(-19) 
#define	RE350_ERR_INVALID_RESP	(-20) 
#define	RE350_ERR_INIT_NOTOPEN	(-21)

// RE350_Command wRequest value for no request parameter
//
#define	RE350_NULL_REQUEST	((WORD)-1)

// ---------------------------------------------
//	RE-350 Commands
// ---------------------------------------------

enum eRE350Cmds
{
// Zoom
	RE350_CMD_HI_SPEED_WIDE_ZOOM,
	RE350_CMD_STEP_FEED_WIDE_ZOOM,
	RE350_CMD_HI_SPEED_TELE_ZOOM,
	RE350_CMD_STEP_FEED_TELE_ZOOM,
	RE350_CMD_STOP_ZOOM_MOTOR,
	RE350_CMD_ACCESS_ZOOM_POSITION,
	RE350_CMD_REQUEST_ZOOM_POSITION,
	RE350_CMD_HI_SPEED_WIDE_ZOOM_AF,
	RE350_CMD_HI_SPEED_TELE_ZOOM_AF,
// Focus
	RE350_CMD_OPERATE_AF,
	RE350_CMD_HI_SPEED_FOCUS_NEAR,
	RE350_CMD_STEP_FEED_FOCUS_NEAR,
	RE350_CMD_HI_SPEED_FOCUS_FAR,
	RE350_CMD_STEP_FEED_FOCUS_FAR,
	RE350_CMD_STOP_AF,
// Color
	RE350_CMD_OPERATE_WB,
	RE350_CMD_ACCESS_WB,
	RE350_CMD_ACCESS_EXP,
	RE350_CMD_ACCESS_DETAIL,
	RE350_CMD_SET_POSITIVE,
	RE350_CMD_SET_NEGATIVE,
	RE350_CMD_SET_COLOR_OUTPUT,
	RE350_CMD_SET_BW_OUTPUT,
// Select Input Signal
	RE350_CMD_SELECT_DOCUMENT_VIDEO,
	RE350_CMD_SELECT_EXTERNAL_VIDEO,
// Lights ON/OFF Control
	RE350_CMD_LIGHT_ON,
	RE350_CMD_LIGHT_OFF,
	RE350_CMD_BACKLIGHT_ON,
	RE350_CMD_BACKLIGHT_OFF,
// LED
	RE350_CMD_LED_NORMAL,
	RE350_CMD_LED_ON,
	RE350_CMD_LED_OFF,
	RE350_CMD_LED_BLINK,
// Communications Control
	RE350_CMD_OFF_LINE_MODE,
	RE350_CMD_ON_LINE_MODE,
	RE350_CMD_NOTIFICATION_MODE,
// Status Request
	RE350_CMD_REQUEST_STATUSA,
	RE350_CMD_REQUEST_STATUSB,
	RE350_CMD_REQUEST_BUTTON_CONDITION,
	RE350_CMD_REQUEST_WB_VOL_POSITION,
	RE350_CMD_REQUEST_EXP_VOL_POSITION,
	RE350_CMD_REQUEST_DETAIL_VOL_POSITION,
	RE350_CMD_REQUEST_AWB_RESULT_DATA,
	RE350_CMD_REQUEST_NAME_OF_EQUIPMENT,
	RE350_CMD_REQUEST_ROM_VERSION,
// Set To Special Mode
	RE350_CMD_SET_TEMP_MODE,
	RE350_CMD_SET_SERVICE_MODE,
	RE350_CMD_SET_DATA_READ_MODE,
// End of Table
	RE350_NUM_CMDS
};

#define	RE350_STATUSA_VIDEOCM		0x0002
#define	RE350_STATUSA_VIDEOPN		0x0004
#define	RE350_STATUSA_AWB			0x0010
#define	RE350_STATUSA_AF			0x0020

#define	RE350_STATUSA_ATFOCUSNEAREND	0x0100
#define	RE350_STATUSA_ATFOCUSFAREND	0x0200
#define	RE350_STATUSA_ONFOCUSNEAR	0x0400
#define	RE350_STATUSA_ONFOCUSFAR	0x0800
#define	RE350_STATUSA_ATZOOMTELEEND	0x1000
#define	RE350_STATUSA_ATZOOMWIDEEND	0x2000
#define	RE350_STATUSA_ONZOOMTELE	0x4000
#define	RE350_STATUSA_ONZOOMWIDE	0x8000

#define	RE350_STATUSB_CMODE			0x1800
#define	RE350_STATUSB_BACKLIGHT		0x2000
#define	RE350_STATUSB_LIGHTS		0x4000
#define	RE350_STATUSB_INPUT			0x8000

#define	RE350_STATUSB_CMODE_SHIFT	11
#define	RE350_STATUSB_CMODE_SILENCE	0x0001
#define	RE350_STATUSB_CMODE_NOTIFY	0x0002

#define	RE350_BUTTONS_FOCUSNEAR		0x0001
#define	RE350_BUTTONS_FOCUSFAR		0x0002
#define	RE350_BUTTONS_ZOOMWIDE		0x0004
#define	RE350_BUTTONS_ZOOMTELE		0x0008
#define	RE350_BUTTONS_LIGHTS		0x0010
#define	RE350_BUTTONS_BACKLIGHT		0x0020

#define	RE350_BUTTONS_RS232		0x0100
#define	RE350_BUTTONS_INPUT		0x0200
#define	RE350_BUTTONS_NEGA		0x0400
#define	RE350_BUTTONS_WB		0x0800
#define	RE350_BUTTONS_BW		0x1000
#define	RE350_BUTTONS_AF		0x2000

// -----------------------------------------
//	Library functions
// -----------------------------------------

int FAR PASCAL 	RE350_Open( int iComPort );

int FAR PASCAL 	RE350_Command( int iDevice, int nCmd, WORD wRequest, LPWORD lpwResponse );

VOID FAR PASCAL	RE350_Close( VOID );

#endif	// _RE350_DEFINED_

/* end of file : re350.h */
