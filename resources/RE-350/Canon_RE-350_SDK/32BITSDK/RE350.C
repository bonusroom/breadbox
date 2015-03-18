/***************************************************************************

	FILE:		re350.c

	AUTHOR:		Eric Chasanoff

	REVISIONS:	08.15.96	Written

	DESCRIPTION:

		This is a basic driver which controls the RE-350 through the
		Windows serial interface. All of the RE-350 functions are
		implemented through RE350_Command.

	DISCLAIMER:

		Raster Builders inc. and Eric Chasanoff make no representations
		about the correctness or suitability of this software for
		any purpose. It is provided without express or implied
		warranty.

***************************************************************************/

#include <windows.h>

#include "re350.h"

#define	RE350_MAKEWORD(l,h)	((WORD)((h)<<8)|(l))

#define	COMM_QSIZE	1024

// ---------------------------------------------
//	Local data - only channel may be opened
//	at a time in this implementation.
// ---------------------------------------------

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

typedef struct tagCOMMINFO
{
	HANDLE		idComDev;
	BOOL		fConnected;
	OVERLAPPED	osWrite;
	OVERLAPPED	osRead;

} COMMINFO;

static COMMINFO	comm;

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

VOID	DelayMSecs( DWORD );
DWORD	ReadMSecs( VOID );


/**************************************************************************

	Initialize RE-350 communications

***************************************************************************/

int FAR PASCAL RE350_Open( int iPort )
{
	DCB				dcb;
	char			szPort[15];
	COMMTIMEOUTS	CommTimeOuts;

	if ( iPort < RE350_COM1 || iPort > RE350_COM4 )
		return RE350_ERR_PARAM;	

	wsprintf( szPort, "COM%d", iPort );

	comm.idComDev			= 0;
	comm.fConnected			= FALSE;
	comm.fConnected			= FALSE;
	comm.osWrite.Offset		= 0;
	comm.osWrite.OffsetHigh	= 0;
	comm.osRead.Offset		= 0;
	comm.osRead.OffsetHigh	= 0;

	// create I/O event used for overlapped reads / writes
	comm.osRead.hEvent = CreateEvent(
				NULL,	// no security
				TRUE,	// explicit reset req
				FALSE,	// initial event reset
				NULL ); // no name

	if ( comm.osRead.hEvent == NULL )
		return RE350_ERR_MEMORY;	

	comm.osWrite.hEvent = CreateEvent(
				NULL,	// no security
				TRUE,	// explicit reset req
				FALSE,	// initial event reset
				NULL ); // no name

	if ( comm.osWrite.hEvent == NULL )
	{
		CloseHandle( comm.osRead.hEvent );
		return RE350_ERR_MEMORY;	
	}

	comm.idComDev = CreateFile(
			szPort,
			GENERIC_READ | GENERIC_WRITE,
			0,			// exclusive access
			NULL,			// no security attrs
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | 
			FILE_FLAG_OVERLAPPED,	// overlapped I/O
			NULL );

	if ( comm.idComDev == (HANDLE) -1 )
		return RE350_ERR_INIT_OPEN;	

	SetCommMask( comm.idComDev, EV_RXCHAR );

	SetupComm( comm.idComDev, COMM_QSIZE, COMM_QSIZE );

	PurgeComm( comm.idComDev, PURGE_TXABORT | PURGE_RXABORT |
			PURGE_TXCLEAR | PURGE_RXCLEAR );

	// set up for overlapped I/O
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 1000;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;

	SetCommTimeouts( comm.idComDev, &CommTimeOuts );

	dcb.DCBlength = sizeof( DCB );

	GetCommState( comm.idComDev, &dcb );

	dcb.BaudRate = CBR_9600;
	dcb.ByteSize = 8;
	dcb.fOutxDsrFlow = TRUE;
	dcb.fDtrControl = dcb.fOutxDsrFlow ? DTR_CONTROL_HANDSHAKE : DTR_CONTROL_ENABLE;
	dcb.fOutxCtsFlow = TRUE;
	dcb.fRtsControl = dcb.fOutxCtsFlow ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;

	dcb.fInX = FALSE;
	dcb.fInX = FALSE;
	dcb.XonChar = 0x11;
	dcb.XoffChar = 0x13;
	dcb.XonLim = 100;
	dcb.XoffLim = 100;

	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;

	if ( !SetCommState( comm.idComDev, &dcb ) )
	{
		comm.fConnected = FALSE;
		CloseHandle( comm.idComDev );
		return RE350_ERR_INIT_SET;	
	}

	comm.fConnected = TRUE;

	EscapeCommFunction( comm.idComDev, SETDTR );	// assert DTR

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

	comm.fConnected = FALSE;
	
	SetCommMask( comm.idComDev, 0 );

	EscapeCommFunction( comm.idComDev, CLRDTR );

	PurgeComm( comm.idComDev, PURGE_TXABORT | PURGE_RXABORT |
					PURGE_TXCLEAR | PURGE_RXCLEAR );

	CloseHandle( comm.idComDev );
	CloseHandle( comm.osRead.hEvent ) ;
	CloseHandle( comm.osWrite.hEvent ) ;
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

	for ( dwStartTime = ReadMSecs();; )
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
	COMSTAT		ComStat;
	DWORD		dwErrorFlags;

	ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );

	return ComStat.cbInQue;
}

/**************************************************************************

	Read character from serial queue for maximum dwMSecs

***************************************************************************/

static int ComGetc( DWORD dwMSecs )
{
	BYTE		c;
	DWORD		dwTimeout;
	BOOL		fReadStat;
	COMSTAT		ComStat;
	DWORD		dwErrorFlags;
	DWORD		dwLength;
	DWORD		dwError;

	dwTimeout = ReadMSecs() + dwMSecs;

	while ( ReadMSecs() < dwTimeout )
	{
		ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );

		if ( ComStat.cbInQue )
		{
			ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );
			dwLength = min( (DWORD)1, ComStat.cbInQue );

			if ( dwLength > 0 )
			{
				fReadStat = ReadFile( comm.idComDev, (LPSTR)&c,
						dwLength, &dwLength, &comm.osRead );
				
				if ( !fReadStat )
				{
					if (GetLastError() == ERROR_IO_PENDING)
					{
						while(!GetOverlappedResult( comm.idComDev, 
							&comm.osRead, &dwLength, TRUE ))
						{
							dwError = GetLastError();
							if ( dwError == ERROR_IO_INCOMPLETE )
								continue;
							else
							{
								// an error occurred, try to recover
								ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );
								break;
							}
							
						}
					}
					else
					{
						// some other error occurred
						dwLength = 0;
						ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );
					}
				}
			}
			
			if ( dwLength == 1 )
				return( c & 255 );
			else
				return RE350_ERR_RECV_IO;
		}
	}

	return RE350_ERR_RECV_TMO;
}

/***************************************************************************

	Write character to serial queue

***************************************************************************/

static int ComPutc( BYTE cByte )
{
	BOOL		fWriteStat;
	DWORD		dwBytesWritten;
	DWORD		dwErrorFlags;
	DWORD		dwError;
	COMSTAT		ComStat;

	fWriteStat = WriteFile( comm.idComDev, (LPSTR)&cByte, 1,
				&dwBytesWritten, &comm.osWrite );

	if ( !fWriteStat ) 
	{
		if ( GetLastError() == ERROR_IO_PENDING )
		{
			while ( !GetOverlappedResult( comm.idComDev,
				&comm.osWrite, &dwBytesWritten, TRUE ))
			{
				dwError = GetLastError();
				
				if ( dwError == ERROR_IO_INCOMPLETE )
					continue;
				else
				{
					ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );
					break;
				}
			}
		}
		else
		{							
			ClearCommError( comm.idComDev, &dwErrorFlags, &ComStat );
			return ( RE350_ERR_SEND_IO );
		}
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

	Delay number of msecs

***************************************************************************/

static VOID DelayMSecs( DWORD dwMSecs )
{
	if ( dwMSecs )
	{
		DWORD dwTimeout = ReadMSecs() + dwMSecs;

		while ( (DWORD) GetCurrentTime() < dwTimeout )
			;
	}
}

/* end of file : re350.c */
