/***************************************************************************

	FILE:		re350.c

	AUTHOR:		Eric Chasanoff

	REVISIONS:	08.15.96	Written

	DESCRIPTION:

		This is a basic driver which controls the RE-350 through the
		Windows serial interface. All of the RE-350 functions are
		implemented through RE350_Command.

		All commands and responses are synchronous, that is the
		driver does not return to the application until a response
		is received or an error occurs after sending a request.
		PeekMessage() is used to delay and wait for responses without
		dequeuing or yielding [see PeekWindows()]. You may want to
		rewrite this to handle requests and responses asynchronously.

	DISCLAIMER:

		Raster Builders inc. and Eric Chasanoff make no representations
		about the correctness or suitability of this software for
		any purpose. It is provided without express or implied
		warranty.

***************************************************************************/

#include <windows.h>

#include "re350.h"

#define	RE350_MAKEWORD(l,h)	((WORD)((h)<<8)|(l))

#define	COMM_QSIZE		80	// Size of send/recv queues

// ---------------------------------------------
//	Local data - only channel may be opened
//	at a time in this implementation.
// ---------------------------------------------

static DCB			theDCB;			// Windows device control

static RE350_CMDS CommandTable[RE350_NUM_CMDS] =
{
//	MaxMs	  Head  ID    Cmd   (H)   (L)
	
	500,	{ 0x10, 0x80, 0x00, 0x01, 0x00 },	// HI_SPEED_WIDE_ZOOM,
	2000,	{ 0x10, 0x80, 0x00, 0x03, 0x00 },	// STEP_FEED_WIDE_ZOOM,
	500,	{ 0x10, 0x80, 0x01, 0x01, 0x00 },	// HI_SPEED_TELE_ZOOM,
	2000,	{ 0x10, 0x80, 0x01, 0x03, 0x00 },	// STEP_FEED_TELE_ZOOM,
	500,	{ 0x10, 0x80, 0x02, 0x00, 0x00 },	// STOP_ZOOM_MOTOR,
	9000,	{ 0x10, 0x80, 0x03, 0x00, 0x00 },	// ACCESS_ZOOM_POSITION,
	100,	{ 0x10, 0x80, 0x04, 0x00, 0x00 },	// REQUEST_ZOOM_POSITION,
	500,	{ 0x10, 0x80, 0x05, 0x00, 0x00 },	// HI_SPEED_WIDE_ZOOM_AF,
	500,	{ 0x10, 0x80, 0x06, 0x00, 0x00 },	// HI_SPEED_TELE_ZOOM_AF,
	        
	20000,	{ 0x10, 0x80, 0x10, 0x00, 0x00 },	// OPERATE_AF,
	500,	{ 0x10, 0x80, 0x12, 0x01, 0x00 },	// HI_SPEED_FOCUS_NEAR,
	2000,	{ 0x10, 0x80, 0x12, 0x03, 0x00 },	// STEP_FEED_FOCUS_NEAR,
	500,	{ 0x10, 0x80, 0x13, 0x01, 0x00 },	// HI_SPEED_FOCUS_FAR,
	2000,	{ 0x10, 0x80, 0x13, 0x03, 0x00 },	// STEP_FEED_FOCUS_FAR,
	500,	{ 0x10, 0x80, 0x14, 0x00, 0x00 },	// STOP_AF,
	        
	7000,	{ 0x10, 0x80, 0x20, 0x00, 0x00 },	// OPERATE_WB,
	500,	{ 0x10, 0x80, 0x22, 0x00, 0x00 },	// ACCESS_WB,
	500,	{ 0x10, 0x80, 0x2A, 0x00, 0x00 },	// ACCESS_EXP,
	500,	{ 0x10, 0x80, 0x30, 0x00, 0x00 },	// ACCESS_DETAIL,
	500,	{ 0x10, 0x80, 0x38, 0x00, 0x00 },	// SET_POSITIVE,
	500,	{ 0x10, 0x80, 0x38, 0x01, 0x00 },	// SET_NEGATIVE,
	500,	{ 0x10, 0x80, 0x40, 0x00, 0x00 },	// SET_COLOR_OUTPUT,
	500,	{ 0x10, 0x80, 0x40, 0x01, 0x00 },	// SET_BW_OUTPUT,
	        
	500,	{ 0x10, 0x80, 0x50, 0x00, 0x00 },	// SELECT_DOCUMENT_VIDEO,
	500,	{ 0x10, 0x80, 0x50, 0x01, 0x00 },	// SELECT_EXTERNAL_VIDEO,
	        
	500,	{ 0x10, 0x80, 0x58, 0x00, 0x00 },	// LIGHT_ON,
	500,	{ 0x10, 0x80, 0x58, 0x01, 0x00 },	// LIGHT_OFF,
	500,	{ 0x10, 0x80, 0x59, 0x00, 0x00 },	// BACKLIGHT_ON,
	500,	{ 0x10, 0x80, 0x59, 0x01, 0x00 },	// BACKLIGHT_OFF,
	        
	500,	{ 0x10, 0x80, 0x68, 0x00, 0x00 },	// LED_NORMAL,
	500,	{ 0x10, 0x80, 0x69, 0x00, 0x00 },	// LED_ON,
	500,	{ 0x10, 0x80, 0x6A, 0x00, 0x00 },	// LED_OFF,
	500,	{ 0x10, 0x80, 0x6B, 0x00, 0x00 },	// LED_BLINK,
	        
	500,	{ 0x10, 0x80, 0x70, 0x00, 0x00 },	// OFF_LINE_MODE,
	500,	{ 0x10, 0x80, 0x70, 0x01, 0x00 },	// ON_LINE_MODE,
	500,	{ 0x10, 0x80, 0x70, 0x02, 0x00 },	// NOTIFICATION_MODE,
	        
	100,	{ 0x10, 0x80, 0x80, 0x00, 0x00 },	// REQUEST_STATUS_GROUP_A,
	100,	{ 0x10, 0x80, 0x80, 0x01, 0x00 },	// REQUEST_STATUS_GROUP_B,
	100,	{ 0x10, 0x80, 0x81, 0x00, 0x00 },	// REQUEST_BUTTON_CONDITION,
	100,	{ 0x10, 0x80, 0x82, 0x00, 0x00 },	// REQUEST_WB_VOL_POSITION,
	100,	{ 0x10, 0x80, 0x82, 0x01, 0x00 },	// REQUEST_EXP_VOL_POSITION,
	100,	{ 0x10, 0x80, 0x82, 0x02, 0x00 },	// REQUEST_DETAIL_VOL_POSITION,
	100,	{ 0x10, 0x80, 0x83, 0x00, 0x00 },	// REQUEST_AWB_RESULT_DATA,
	100,	{ 0x10, 0x80, 0x88, 0x00, 0x00 },	// REQUEST_NAME_OF_EQUIPMENT,
	100,	{ 0x10, 0x80, 0x88, 0x01, 0x00 },	// REQUEST_ROM_VERSION,
	        
	100,	{ 0x10, 0x80, 0xF0, 0x00, 0x00 },	// SET_TEMP_MODE,
	100,	{ 0x10, 0x80, 0xF1, 0xFF, 0xFF },	// SET_SERVICE_MODE,
	100,	{ 0x10, 0x80, 0xF1, 0xFF, 0x00 }	// SET_DATA_READ_MODE,
};

static LPSTR szCommMsgs[] = { "COM1", "COM2", "COM3", "COM4" };

// ---------------------------------------------
//	Forward declarations
// ---------------------------------------------

int		RecvCommand( LPBYTE lpCmd, DWORD dwTimeout );
int		SendCommand( LPBYTE lpCmd );
int		SendRecvCommand( LPBYTE lpCmd, LPBYTE lpResp, DWORD dwTimeout );

BOOL	ComReady( VOID );
int		ComCount( VOID );
int		ComGetc( DWORD );
int		ComPutc( BYTE );
VOID	ComFlush( VOID );

VOID	PeekWindows( VOID );
VOID	DelayMSecs( DWORD );
DWORD	ReadMSecs( VOID );

/***************************************************************************

	Initialize RE-350 communications

***************************************************************************/

int FAR PASCAL RE350_Open( int iCommPort )
{
	int		nComError;
	int		nPortId;
	char	szComm[80];
       
	if ( iCommPort < RE350_COM1 || iCommPort > RE350_COM4 )
		return RE350_ERR_PARAM;

	wsprintf( (LPSTR)szComm, "%ls:9600,N,8,2", (LPSTR)szCommMsgs[iCommPort] );

	nComError = BuildCommDCB( (LPSTR)szComm, (DCB FAR *)&theDCB );

	if ( nComError != 0 )
		return RE350_ERR_INIT_FORMAT;

	nPortId = OpenComm( (LPSTR)szCommMsgs[iCommPort], COMM_QSIZE, COMM_QSIZE );

	if ( nPortId < 0 )
	{
		switch( nPortId )
		{
			case	IE_BADID:
			case	IE_BAUDRATE:
			case	IE_BYTESIZE:
			case	IE_DEFAULT:
					// parameter errors
					return RE350_ERR_PARAM;

			case	IE_HARDWARE:
					// hardware is locked by another device
					return RE350_ERR_INIT_HARDWARE;

			case	IE_MEMORY:
					// function cannot allocate queues
					return RE350_ERR_MEMORY;

			case	IE_OPEN:
					// device is already open
					return RE350_ERR_INIT_OPEN;

			case	IE_NOPEN:
					// device is not open
					return RE350_ERR_INIT_NOTOPEN;

			default:
					return RE350_ERR_UNKNOWN;
		}
	}

	theDCB.Id = (BYTE) nPortId;
	theDCB.fRtsDisable = FALSE;
	theDCB.fDtrDisable = FALSE;
	theDCB.fOutxCtsFlow = TRUE;	// use CTS flow control
	theDCB.CtsTimeout = 1000;
	theDCB.fBinary = TRUE;
	theDCB.fParity = FALSE;
	theDCB.fNull = 0;
	theDCB.fChEvt = 0;
	theDCB.fRtsflow = FALSE;
	theDCB.fDtrflow = FALSE;
	theDCB.fOutX = 0;
	theDCB.fInX = 0;

	nComError = SetCommState( (DCB FAR *)&theDCB );

	if ( nComError != 0 )
	{
		CloseComm( theDCB.Id );
		return RE350_ERR_INIT_SET;
	}

	return RE350_ERR_NONE;
}

/***************************************************************************

	Close RE-350 communications

***************************************************************************/

VOID FAR PASCAL RE350_Close( VOID )
{
	// This is required to make sure the Windows
	//  transmit queue is dispatched.
	DelayMSecs( RE350_CLOSE_DELAY );

	CloseComm( theDCB.Id );
}

/*****************************************************************************

	Send RE-350 command

****************************************************************************/

int FAR PASCAL RE350_Command( int byDevice, int nCmd, WORD wRequest, LPWORD lpwResponse )
{
	int				nRetCode;
	static RE350_HEADER	response;

	if ( nCmd < 0 || nCmd >= RE350_NUM_CMDS )
		return RE350_ERR_INVALID_CMD;

	CommandTable[nCmd].header.byID = 0x80 | (BYTE)byDevice;

	if ( wRequest != RE350_NULL_REQUEST )
	{
		CommandTable[nCmd].header.byParamH = (BYTE)((wRequest >> 8) & 0xFF);
		CommandTable[nCmd].header.byParamL = (BYTE)(wRequest & 0xFF);
	}	

	nRetCode = SendRecvCommand( (LPBYTE)&CommandTable[nCmd].header, (LPBYTE)&response, CommandTable[nCmd].dwMaxMs );
	
	if ( nRetCode == RE350_ERR_NONE )
	{
		if ( lpwResponse )
			*lpwResponse = RE350_MAKEWORD(response.byParamL,response.byParamH);
	}

	return nRetCode;
}

/*****************************************************************************

	Send command and wait for response

****************************************************************************/

static int SendRecvCommand( LPBYTE lpCmd, LPBYTE lpResp, DWORD dwTimeout )
{
	int		nRetCode;

	// x2 fudge factor for timeout
	//
	dwTimeout *= 2;

	ComFlush(); // synchronize input stream

	if ( (nRetCode = SendCommand( lpCmd )) == RE350_ERR_NONE )
	{
		if ( (nRetCode = RecvCommand( lpResp, dwTimeout )) == RE350_ERR_NONE )
		{
			if ( lpResp[0] == RE350_C_RESPONSE )
			{
				if ( lpResp[2] == 0xF0 )
				{
					if ( lpResp[4] == 0x00 )
						return RE350_ERR_SYSTEM;
					else if ( lpResp[4] == 0x01 )
						return RE350_ERR_MODE;
					else if ( lpResp[4] == 0x02 )
						return RE350_ERR_TIMEOUT;
					else
						return RE350_ERR_INVALID_RESP;
				}
				else
					return RE350_ERR_NONE;
			}
			else
				return RE350_ERR_INVALID_RESP;
		}
	}

	return nRetCode;
}

/*****************************************************************************

	Send command

****************************************************************************/

static int SendCommand( LPBYTE lpCmd )
{
	int		i;
	int		nRetCode;
	DWORD	dwStartTime;

	dwStartTime = ReadMSecs();

	for ( i=0; i < RE350_REQUEST_LEN; i++ )
	{
		if ( (nRetCode = ComPutc( lpCmd[i] )) != RE350_ERR_NONE )
			return nRetCode;

		if ( ReadMSecs() - dwStartTime >= RE350_SEND_TMO )
			return RE350_ERR_SEND_TMO;
	}

	return RE350_ERR_NONE;
}

/*****************************************************************************

	Receive Command

****************************************************************************/

static int RecvCommand( LPBYTE lpCmd, DWORD dwTimeout )
{
	int		i,c;
	DWORD	dwStartTime;

	for ( dwStartTime=ReadMSecs();; )
	{
		if ( ComCount() >= RE350_RESPONSE_LEN )
		{
			for ( i=0; i < RE350_RESPONSE_LEN; i++ )
			{
				c = ComGetc( RE350_CHAR_TMO );

				if ( c < 0 )
					return c;

				lpCmd[i] = (BYTE) c;
			}

			// -------------------------------
			//	Check for Device ID and
			//	valid command.
			// -------------------------------

			return RE350_ERR_NONE;
		}

		if ( dwStartTime > 0 && ReadMSecs() - dwStartTime >= dwTimeout )
			return RE350_ERR_RECV_TMO;
	}
}


/***************************************************************************

	Flush receive queue

***************************************************************************/

static VOID ComFlush( VOID )
{
	while ( ComCount() > 0 )
		ComGetc( RE350_CHAR_TMO );
}

/***************************************************************************

	Returns TRUE if receive character is available

***************************************************************************/

static BOOL ComReady( VOID )
{
	return( ComCount() > 0 );
}

/***************************************************************************

	Returns number of chars in recv queue

***************************************************************************/

static int ComCount( VOID )
{
	COMSTAT	CommStat;

	GetCommError( theDCB.Id, (COMSTAT FAR *)&CommStat );

	return( CommStat.cbInQue );
}

/**************************************************************************

	Read character from serial queue for maximum dwMSecs

***************************************************************************/

static int ComGetc( DWORD dwMSecs )
{
	BYTE	c;
	int		nStatus;
	DWORD	dwTimeout;
	COMSTAT	CommStat;
	short	nErr;
	WORD	wEvt;

	dwTimeout = ReadMSecs() + dwMSecs;

	while ( ReadMSecs() < dwTimeout )
	{
		PeekWindows();

		GetCommError( theDCB.Id, (COMSTAT FAR *)&CommStat );

		if ( CommStat.cbInQue )
		{
			nStatus = ReadComm( theDCB.Id, (LPSTR)&c, 1 );

			if ( nStatus < 0 )
			{
				nErr = GetCommError( theDCB.Id, (COMSTAT FAR *)&CommStat );
				wEvt = GetCommEventMask( theDCB.Id, 0xFFFF );

				return RE350_ERR_RECV_IO;
			}

			else if ( nStatus == 1 )
				return( c & 255 );
		}
	}

	return RE350_ERR_RECV_TMO;
}

/***************************************************************************

	Write character to serial queue

***************************************************************************/

static int ComPutc( BYTE cByte )
{
	int		nStatus;
	COMSTAT	CommStat;
	short	nErr;
	WORD	wEvt;


	nStatus = WriteComm( theDCB.Id, (LPSTR)&cByte, 1 );

	if ( nStatus != 1 )
	{
		nErr = GetCommError( theDCB.Id, (COMSTAT FAR *)&CommStat );
		wEvt = GetCommEventMask( theDCB.Id, 0xFFFF );

		return RE350_ERR_SEND_IO;
	}

	return RE350_ERR_NONE;
}

/***************************************************************************

	Read system time in msecs

***************************************************************************/

static DWORD ReadMSecs( VOID )
{
	return( (DWORD) GetCurrentTime() );
}

/***************************************************************************

	Delay number of msecs - calls PeekWindows

***************************************************************************/

static VOID DelayMSecs( DWORD dwMSecs )
{
	if ( dwMSecs )
	{
		DWORD dwTimeout = ReadMSecs() + dwMSecs;

		while ( ReadMSecs() < dwTimeout )
			PeekWindows();
	}
}

/**************************************************************************

	Let Windows breath without yielding or dequeuing

**************************************************************************/

static VOID PeekWindows( VOID )
{
	MSG		msg;
	DWORD	dwCount = 0L;

	while ( PeekMessage( &msg, NULL, NULL, NULL, PM_NOREMOVE ) )
	{
		if ( ++dwCount > RE350_PEEK_RETRY )
			break;
	}
}

/* end of file : re350.c */
