/***************************************************************************

	FILE:		example.c

	AUTHOR:		Eric Chasanoff

	REVISIONS:	08.15.96	Written

	DESCRIPTION:

		This is a simple Windows 32 bit application to demonstrate
		communications with the Canon RE-350. The program demonstrates
		how to initialize the RE-350 and track the zoom position.

		RE350.C contains the basic serial driver for the RE-350
		camera. Although the driver implements all of the RE-350
		commands, this application demonstrates a sub-set for
		purposes of clarity.

	DISCLAIMER:

		Raster Builders Inc. and Eric Chasanoff make no representations
		about the correctness or suitability of this software for
		any purpose. It is provided without express or implied
		warranty.

***************************************************************************/

#include <windows.h>

#include "re350.h"
#include "example.h"

#define	DEVICE_ID		0	// check dipswitches

#define	WINDOW_WIDTH	200
#define	WINDOW_HEIGHT	200

#define INRECT(r,x,y)	((x)>=((r)->left)&&(x)<((r)->right)&&(y)>=((r)->top)&&(y)<((r)->bottom))
#define	HRECT(r)		((r)->right-(r)->left)
#define	VRECT(r)		((r)->bottom-(r)->top)

// --------------------------------------------------
//	Local Data
// --------------------------------------------------

static HWND			hWndApp = NULL;			// for ErrorMessage
static HCURSOR		hWaitCursor = NULL;		// for StartWait/EndWait
static HINSTANCE	hLibDLL = NULL;			// LoadLibrary

static char			szAppName[] = { "EXAMPLE" };

static int			iCommPort = RE350_COM1;
static BOOL			bInitialized = FALSE;	// TRUE when camera is initialized
static BOOL			bOnline = FALSE;		// TRUE when camera is Online

static RECT			rctZoomWide;			// Button location
static RECT			rctZoomTele;

static RECT			rctZoomFill;			// Zoom Display location
static RECT			rctZoomDisplay;

static char			szZoomDisplay[16];

static char 		szZoomWide[] = { "Wide" };
static char			szZoomTele[] = { "Tele" };

// --------------------------------------------------
//	Forward Declarations
// --------------------------------------------------

LONG FAR PASCAL
			WndProc( HWND, UINT, WPARAM, LPARAM );

VOID		ErrorMessage( LPSTR fmt,... );
VOID		StartWait( VOID );
VOID		EndWait( VOID );

BOOL		ResetRE350( HWND );
BOOL		CheckRE350( HWND );
VOID		DisplayRE350Error( int );
int			UpdateZoomPosition( HWND );

VOID		Draw3DBorder( HDC hDC, LPRECT lpRect, BOOL bReverse );
VOID		DrawButton( HWND, HDC, LPRECT, LPSTR, BOOL );
int			WaitZoomButtonUp( HWND, int );

/****************************************************************************

	Application Entry

****************************************************************************/

int	PASCAL WinMain( HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow )
{
	HWND		hWnd;
	MSG			msg;
	WNDCLASS	wndclass;

	if ( hPrevInstance )
    {
    	ErrorMessage( "%ls is already running", (LPSTR)szAppName );
    	return FALSE;
    }
    
	wndclass.style			= CS_HREDRAW | CS_VREDRAW;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInstance;
	wndclass.hIcon			= LoadIcon( hInstance, "RE350ICON" );
	wndclass.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground	= GetStockObject(LTGRAY_BRUSH);
	wndclass.lpszMenuName	= (LPSTR)szAppName;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.lpszClassName	= (LPSTR)szAppName;

	if ( !RegisterClass( &wndclass ) )
		return FALSE;

	// --------------------------------------------------------
	// Force load/unload of DLL so it gets purged from memory
	// when the application terminates. This makes debugging easier.
	// --------------------------------------------------------

	if ( (hLibDLL = LoadLibrary( RE350_LIBNAME )) == NULL )
	{	
		ErrorMessage( "%ls wasn't loaded", (LPSTR)RE350_LIBNAME );
		return FALSE;
	}

	hWnd = CreateWindow(
				szAppName,
				szAppName,
				WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU,
				// center window on the desktop
				(GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH)/2,
				(GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT)/2,
				WINDOW_WIDTH,
				WINDOW_HEIGHT,
				NULL,
				NULL,
				hInstance,
				NULL );

	if ( hWnd == NULL )
	{
		FreeLibrary( hLibDLL );
		return FALSE;
	}

	hWndApp = hWnd;

	ShowWindow( hWnd, nCmdShow );
	UpdateWindow( hWnd );

	ResetRE350( hWnd );	// get online

	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	if ( hLibDLL )
		FreeLibrary( hLibDLL );

	return msg.wParam;
}

/**************************************************************************

	Process Main Window Messages

****************************************************************************/

LONG	FAR PASCAL WndProc( HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT	ps;

	switch( wMessage )
	{
		case	WM_CREATE:
		{
			int		x,y,cx,cy;
			RECT	rctClient;

			//  +-----------------+
			//  |     +-----+     |
			//  |     | 100 |     | (display)
			//  |     +-----+     |
			//  +--------+--------+
			//  |        |        |
			//  |  Wide  |  Tele  |
			//  |        |        |
			//  +--------+--------+

			GetClientRect( hWnd, &rctClient );

			x = 0;
			y = 0;
			cx = HRECT( &rctClient );
			cy = VRECT( &rctClient ) / 2;
			SetRect( &rctZoomFill, x, y, x+cx, y+cy );

			cx = HRECT( &rctZoomFill ) / 2;
			cy = VRECT( &rctZoomFill ) / 2;
			x = (HRECT( &rctZoomFill ) - cx) / 2;
			y = (VRECT( &rctZoomFill ) - cy) / 2;
			SetRect( &rctZoomDisplay, x, y, x+cx, y+cy );

			x = 0;
			y = VRECT( &rctClient ) / 2;
			cx = HRECT( &rctClient ) / 2;
			cy = VRECT( &rctClient ) / 2;
			SetRect( &rctZoomWide, x, y, x+cx, y+cy );

			x += cx;
			SetRect( &rctZoomTele, rctClient.right/2, rctClient.bottom/2, rctClient.right, rctClient.bottom );

			*szZoomDisplay = '\0';
			break;
		}

		case	WM_CLOSE:
			DestroyWindow( hWnd );
			break;

		case	WM_DESTROY:
			if ( bOnline )
			{
				RE350_Command( DEVICE_ID, RE350_CMD_OFF_LINE_MODE, RE350_NULL_REQUEST, NULL );
				bOnline = FALSE;
			}

			RE350_Close();

			PostQuitMessage( 0 );
			break;

		case	WM_PAINT:
		{
			HDC hDC = BeginPaint( hWnd, &ps );

			DrawButton( hWnd, hDC, &rctZoomFill, NULL, FALSE );
			DrawButton( hWnd, hDC, &rctZoomDisplay, (LPSTR)szZoomDisplay, TRUE );
			DrawButton( hWnd, hDC, &rctZoomWide, szZoomWide, FALSE );
			DrawButton( hWnd, hDC, &rctZoomTele, szZoomTele, FALSE );

			EndPaint( hWnd, &ps );
			break;
		}

		case	WM_LBUTTONDOWN:
		{
			int		nErrorCode;
			POINT	ptLoc;
			
			ptLoc.x = LOWORD(lParam);
			ptLoc.y = HIWORD(lParam);

			if ( !CheckRE350( hWnd ) )
				break;

			if ( INRECT( &rctZoomWide, ptLoc.x, ptLoc.y ) )
			{
				DrawButton( hWnd, NULL, &rctZoomWide, szZoomWide, TRUE );

				nErrorCode = WaitZoomButtonUp( hWnd, RE350_CMD_HI_SPEED_WIDE_ZOOM_AF );

				DrawButton( hWnd, NULL, &rctZoomWide, szZoomWide, FALSE );

				DisplayRE350Error( nErrorCode );
				break;
			}

			else if ( INRECT( &rctZoomTele, ptLoc.x, ptLoc.y ) )
			{
				DrawButton( hWnd, NULL, &rctZoomTele, szZoomTele, TRUE );

				nErrorCode = WaitZoomButtonUp( hWnd, RE350_CMD_HI_SPEED_TELE_ZOOM_AF );

				DrawButton( hWnd, NULL, &rctZoomTele, szZoomTele, FALSE );

				DisplayRE350Error( nErrorCode );
				break;
			}

			break;
		}

		case	WM_INITMENU:
			CheckMenuItem( (HMENU)wParam, IDM_COM1, MF_UNCHECKED );
			CheckMenuItem( (HMENU)wParam, IDM_COM2, MF_UNCHECKED );
			CheckMenuItem( (HMENU)wParam, IDM_COM3, MF_UNCHECKED );
			CheckMenuItem( (HMENU)wParam, IDM_COM4, MF_UNCHECKED );

			switch( iCommPort )
			{
				case	RE350_COM1:
					CheckMenuItem( (HMENU)wParam, IDM_COM1, MF_CHECKED );
					break;
				case	RE350_COM2:
					CheckMenuItem( (HMENU)wParam, IDM_COM2, MF_CHECKED );
					break;
				case	RE350_COM3:
					CheckMenuItem( (HMENU)wParam, IDM_COM3, MF_CHECKED );
					break;
				case	RE350_COM4:
					CheckMenuItem( (HMENU)wParam, IDM_COM4, MF_CHECKED );
					break;
			}
			break;

		case	WM_COMMAND:
		{
			WORD	wStatusA;
			WORD	wStatusB;
			int		nErrorCode;
			
			switch( wParam )
			 {
				// ------------------------------------
				//	File Menu
				// ------------------------------------

				case	IDM_EXIT:
					SendMessage( hWnd, WM_CLOSE, 0, 0L );
					break;
					
				// ------------------------------------
				//	Setup Menu
				// ------------------------------------

				case	IDM_RESET:
					ResetRE350( hWnd );
					break;

				case	IDM_COM1:
					if ( iCommPort != RE350_COM1 )
					{
						iCommPort = RE350_COM1;
						ResetRE350( hWnd );
					}
					break;

				case	IDM_COM2:
					if ( iCommPort != RE350_COM2 )
					{
						iCommPort = RE350_COM2;
						ResetRE350( hWnd );
					}
					break;

				case	IDM_COM3:
					if ( iCommPort != RE350_COM3 )
					{
						iCommPort = RE350_COM3;
						ResetRE350( hWnd );
					}
					break;

				case	IDM_COM4:
					if ( iCommPort != RE350_COM4 )
					{
						iCommPort = RE350_COM4;
						ResetRE350( hWnd );
					}
					break;
      			
				// ------------------------------------
				//	Commands Menu
				// ------------------------------------

				case	IDM_LIGHTS:
					if ( !CheckRE350( hWnd ) )
						break;

					nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_REQUEST_STATUSB, RE350_NULL_REQUEST, (LPWORD)&wStatusB );

					if ( wStatusB & RE350_STATUSB_LIGHTS )
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_LIGHT_OFF, RE350_NULL_REQUEST, NULL );
					else
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_LIGHT_ON, RE350_NULL_REQUEST, NULL );

					if ( nErrorCode != RE350_ERR_NONE )
						DisplayRE350Error( nErrorCode );
					break;

				case	IDM_BACKLIGHT:
					if ( !CheckRE350( hWnd ) )
						break;

					nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_REQUEST_STATUSB, RE350_NULL_REQUEST, (LPWORD)&wStatusB );

					if ( wStatusB & RE350_STATUSB_BACKLIGHT )
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_BACKLIGHT_OFF, RE350_NULL_REQUEST, NULL );
					else
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_BACKLIGHT_ON, RE350_NULL_REQUEST, NULL );
					
					if ( nErrorCode != RE350_ERR_NONE )
						DisplayRE350Error( nErrorCode );
					break;

				case	IDM_BW:
					if ( !CheckRE350( hWnd ) )
						break;

					nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_REQUEST_STATUSA, RE350_NULL_REQUEST, (LPWORD)&wStatusA );

					if ( wStatusA & RE350_STATUSA_VIDEOCM )
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_SET_BW_OUTPUT, RE350_NULL_REQUEST, NULL );
					else
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_SET_COLOR_OUTPUT, RE350_NULL_REQUEST, NULL );

					if ( nErrorCode != RE350_ERR_NONE )
						DisplayRE350Error( nErrorCode );
					break;

				case	IDM_NEGATIVE:
					if ( !CheckRE350( hWnd ) )
						break;

					nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_REQUEST_STATUSA, RE350_NULL_REQUEST, (LPWORD)&wStatusA );

					if ( wStatusA & RE350_STATUSA_VIDEOPN )
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_SET_NEGATIVE, RE350_NULL_REQUEST, NULL );
					else
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_SET_POSITIVE, RE350_NULL_REQUEST, NULL );

					if ( nErrorCode != RE350_ERR_NONE )
						DisplayRE350Error( nErrorCode );
					break;

				case	IDM_INPUT:
					if ( !CheckRE350( hWnd ) )
						break;

					nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_REQUEST_STATUSB, RE350_NULL_REQUEST, (LPWORD)&wStatusB );

					if ( wStatusB & RE350_STATUSB_INPUT )
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_SELECT_EXTERNAL_VIDEO, RE350_NULL_REQUEST, NULL );
					else
						nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_SELECT_DOCUMENT_VIDEO, RE350_NULL_REQUEST, NULL );

					if ( nErrorCode != RE350_ERR_NONE )
						DisplayRE350Error( nErrorCode );
					break;

				default:
					return FALSE;
			}
			return TRUE;
		}

		default:
			return DefWindowProc( hWnd, wMessage, wParam, lParam );

	}
	return FALSE;
}

/****************************************************************************

	Reset RE-350 

****************************************************************************/

BOOL	ResetRE350( HWND hWnd )
{
	int		nErrorCode = RE350_ERR_NONE;

	StartWait();

	if ( bInitialized )
	{
		if ( bOnline )
		{
			nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_OFF_LINE_MODE, 
				RE350_NULL_REQUEST, NULL );
			
			bOnline = FALSE;
		}

    	RE350_Close();

		bInitialized = FALSE;
	}

 	nErrorCode = RE350_Open( iCommPort );
	
	if ( nErrorCode != RE350_ERR_NONE )
		goto ErrorExit;

	bInitialized = TRUE;

	nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_ON_LINE_MODE, 
			RE350_NULL_REQUEST, NULL );
	
	if ( nErrorCode == RE350_ERR_NONE )
	{
		bOnline = TRUE;
		nErrorCode = UpdateZoomPosition( hWnd );
	}

ErrorExit:

	EndWait();

	if ( nErrorCode != RE350_ERR_NONE )
		DisplayRE350Error( nErrorCode );

	return bOnline;
}

/****************************************************************************

	Make sure RE-350 is on line 

****************************************************************************/

BOOL	CheckRE350( HWND hWnd )
{
	int		nErrorCode = RE350_ERR_NONE;

	StartWait();

	if ( !bInitialized )
	{
	 	nErrorCode = RE350_Open( iCommPort );
	
		if ( nErrorCode != RE350_ERR_NONE )
			goto ErrorExit;

		bInitialized = TRUE;
	}

	if ( !bOnline )
	{
		nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_ON_LINE_MODE, 
				RE350_NULL_REQUEST, NULL );
			
		if ( nErrorCode != RE350_ERR_NONE )
			goto ErrorExit;

		bOnline = TRUE;
	}
	
ErrorExit:

	EndWait();

	if ( nErrorCode != RE350_ERR_NONE )
		DisplayRE350Error( nErrorCode );

	return bOnline;
}

/***************************************************************************

	Display RE-350 error message

***************************************************************************/

VOID	DisplayRE350Error( int nErrorCode )
{
	switch( nErrorCode )
	{
	case	RE350_ERR_NONE:
			break;

	case	RE350_ERR_INIT_OPEN:
			ErrorMessage( 
				(LPSTR)"Communications device is already in use" );
			break;

	case	RE350_ERR_INIT_NOTOPEN:
			ErrorMessage( 
				(LPSTR)"Communications device could not be opened" );
			break;

	case	RE350_ERR_INIT_HARDWARE:
			ErrorMessage( 
				(LPSTR)"Communications device is not available" );
			break;

	case	RE350_ERR_INIT_SET:
	case	RE350_ERR_INIT_FORMAT:
			ErrorMessage( 
				(LPSTR)"Error initializing driver parameters" );
			break;

	case	RE350_ERR_DRIVER_INIT:
			ErrorMessage( 
				(LPSTR)"Communications has not been initialized" );
			break;

	case	RE350_ERR_SEND_TMO:
			ErrorMessage( (LPSTR)"Send timeout" );
			break;

	case	RE350_ERR_RECV_TMO:
			ErrorMessage( (LPSTR)"Receive timeout" );
			break;

	case	RE350_ERR_RECV_IO:
			ErrorMessage( (LPSTR)"Receive I/O error" );
			break;

	case	RE350_ERR_SEND_IO:
			ErrorMessage( (LPSTR)"Send character I/O error" );
			break;

	case	RE350_ERR_BUSY:
			ErrorMessage( (LPSTR)"Busy timeout" );
			break;

	case	RE350_ERR_CMD:
			ErrorMessage( (LPSTR)"Command error" );
			break;

	case	RE350_ERR_PARAM:
			ErrorMessage( (LPSTR)"Invalid function parameter error" );
			break;

	case	RE350_ERR_INVALID_CMD:
			ErrorMessage( (LPSTR)"Invalid RE-350 command" );
			break;

	case	RE350_ERR_MEMORY:
			ErrorMessage( (LPSTR)"Out of memory" );
			break;

	case	RE350_ERR_IDLE_TIMEOUT:
			ErrorMessage( (LPSTR)"Timeout waiting for idle state" );
			break;

	case	RE350_ERR_MODE:
			ErrorMessage( (LPSTR)"RE-350 Mode error" );
			break;

	case	RE350_ERR_TIMEOUT:
			ErrorMessage( (LPSTR)"RE-350 timeout error" );
			break;

	case	RE350_ERR_SYSTEM:
			ErrorMessage( (LPSTR)"RE-350 system error" );
			break;

	case	RE350_ERR_INVALID_RESP:
			ErrorMessage( (LPSTR)"Invalid RE-350 response" );
			break;

	case	RE350_ERR_UNKNOWN:
			ErrorMessage( (LPSTR)"Unknown error" );
			break;

	default:
			ErrorMessage( (LPSTR)"Unknown driver error [%d]", nErrorCode );
			break;
	}
}

/****************************************************************************

	Wait for user to release mouse button - update zoom display

****************************************************************************/

static int WaitZoomButtonUp( HWND hWnd, int nCommand )
{
	MSG		msg;
	int		nErrorCode;

	nErrorCode = RE350_Command( DEVICE_ID, nCommand, RE350_NULL_REQUEST, NULL );
				
	SetCapture( hWnd );

	for ( ;; )
	{
		if ( PeekMessage( &msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ) )
		{
			if ( msg.message == WM_LBUTTONUP )
				break;
		}
	}

	ReleaseCapture();

	if ( nErrorCode == RE350_ERR_NONE )
	{
			nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_STOP_ZOOM_MOTOR, RE350_NULL_REQUEST, NULL );
			nErrorCode = UpdateZoomPosition( hWnd );
	}

	return nErrorCode;
}

/****************************************************************************

	Update Zoom Position Display

****************************************************************************/

static int UpdateZoomPosition( HWND hWnd )
{
	int		nErrorCode;
	WORD	wZoomPosition;

	nErrorCode = RE350_Command( DEVICE_ID, RE350_CMD_REQUEST_ZOOM_POSITION, RE350_NULL_REQUEST, (LPWORD)&wZoomPosition );

	if ( nErrorCode == RE350_ERR_NONE )
		wsprintf( szZoomDisplay, "%d", wZoomPosition );
	else
		wsprintf( szZoomDisplay, "??" );

	DrawButton( hWnd, NULL, &rctZoomDisplay, (LPSTR)szZoomDisplay, TRUE );
					
	return nErrorCode;
}

/**************************************************************************

	Draw button based on bReverse flag

****************************************************************************/

static VOID DrawButton( HWND hWnd, HDC hDC, LPRECT lpRect, LPSTR lpszText, BOOL bReverse )
{
	HBRUSH		hOldBrush;
	HDC			hDCTemp = NULL;
	RECT		rctButton;

	if ( hDC == NULL )
	{
		hDCTemp = GetDC( hWnd );
		hDC = hDCTemp;
	}

	CopyRect( &rctButton, lpRect );

	// Fill in background
	hOldBrush = SelectObject( hDC, GetStockObject(LTGRAY_BRUSH) );

	PatBlt( hDC, 
		rctButton.left, 
		rctButton.top, 
		rctButton.right - rctButton.left, 
		rctButton.bottom - rctButton.top, 
		PATCOPY );

	SelectObject( hDC, hOldBrush );

	// Draw border 2 pixels thick
	Draw3DBorder( hDC, &rctButton, bReverse );
	InflateRect( &rctButton, -1, -1 );
	Draw3DBorder( hDC, &rctButton, bReverse );

	// Draw text centered in rectangle if specified
	if ( lpszText )
	{
		HFONT hOldFont = SelectObject( hDC, GetStockObject(SYSTEM_FONT) );

		SetBkMode( hDC, TRANSPARENT );
		SetTextColor( hDC, GetSysColor(COLOR_WINDOWTEXT) );

		DrawText( hDC,
			(LPSTR) lpszText,
			lstrlen( (LPSTR)lpszText ),
			(LPRECT) &rctButton,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE );

		SelectObject( hDC, hOldFont );
	}

	if ( hDCTemp )
		ReleaseDC( hWnd, hDCTemp );
}

/**************************************************************************

	Draw 3D Border base on bReverse flag

****************************************************************************/

static VOID Draw3DBorder( HDC hDC, LPRECT lpRect, BOOL bReverse )
{
	HBRUSH		hOldBrush;

	hOldBrush = SelectObject( hDC, GetStockObject(LTGRAY_BRUSH) );

	if ( bReverse )
		SelectObject( hDC, GetStockObject(WHITE_BRUSH) );
	else
		SelectObject( hDC, GetStockObject(GRAY_BRUSH) );

	PatBlt( hDC, 
		lpRect->right - 1, 
		lpRect->top, 
		1, 
		lpRect->bottom - lpRect->top - 1, 
		PATCOPY );

	PatBlt( hDC,
		lpRect->left,
		lpRect->bottom - 1, 
		lpRect->right - lpRect->left - 1, 
		1,
		PATCOPY );

	if ( bReverse )
		SelectObject( hDC, GetStockObject(GRAY_BRUSH) );
	else
		SelectObject( hDC, GetStockObject(WHITE_BRUSH) );

	PatBlt( hDC, 
		lpRect->left,
		lpRect->top,
		lpRect->right - lpRect->left, 
		1,
		PATCOPY );

	PatBlt( hDC,
		lpRect->left,
		lpRect->top,
		1,
		lpRect->bottom - lpRect->top, 
		PATCOPY );

	SelectObject( hDC, hOldBrush );
}

/****************************************************************************

	Display error message

****************************************************************************/

VOID	ErrorMessage( LPSTR szFmt,... )
{
	char szMsg[128];

	wvsprintf( (LPSTR)szMsg, szFmt, (LPSTR)(&szFmt+1) );
	
	MessageBox( hWndApp, (LPSTR)szMsg, (LPSTR)szAppName, 
			MB_OK|MB_APPLMODAL );
}

/***************************************************************************

	Display/remove cursors for lengthy operations

***************************************************************************/

VOID	StartWait( VOID )
{
	if ( hWaitCursor == NULL )
		hWaitCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
}

VOID	EndWait( VOID )
{
	if ( hWaitCursor )
	{
		SetCursor( hWaitCursor );
		hWaitCursor = NULL;
	}
}

/* end of file : example.c */
