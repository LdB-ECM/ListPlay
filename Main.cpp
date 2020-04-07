#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#pragma comment( lib, "comctl32.lib")
#pragma comment( lib,"Msimg32.lib")   // needed for GradientFill() API
 
// HINSTANCE of our window
HINSTANCE hInst;

// ** LdB the list box had an ID of 7000 it coccurs a few places I dragged it up here to a constant which is how it should be done
//  I have replaced the number 7000 where it occurred
#define ID_LISTBOX 7000
// this is the second listbox ID
#define ID_LISTBOX1 7001
 
// helper function for drawing parent background
void GradientTriangle( HDC MemDC, 
    LONG x1, LONG y1, 
    LONG x2, LONG y2, 
    LONG x3, LONG y3, 
    COLORREF top, COLORREF bottom )
{
    TRIVERTEX vertex[3];
 
    vertex[0].x     = x1;
    vertex[0].y     = y1;
    vertex[0].Red   = GetRValue(bottom) << 8;
    vertex[0].Green = GetGValue(bottom) << 8;
    vertex[0].Blue  = GetBValue(bottom) << 8;
    vertex[0].Alpha = 0x0000;
 
    vertex[1].x     = x3;
    vertex[1].y     = y3; 
    vertex[1].Red   = GetRValue(bottom) << 8;
    vertex[1].Green = GetGValue(bottom) << 8;
    vertex[1].Blue  = GetBValue(bottom) << 8;
    vertex[1].Alpha = 0x0000;
 
    vertex[2].x     = x2;
    vertex[2].y     = y2;
    vertex[2].Red   = GetRValue(top) << 8;
    vertex[2].Green = GetGValue(top) << 8;
    vertex[2].Blue  = GetBValue(top) << 8;
    vertex[2].Alpha = 0x0000;
 
    // Create a GRADIENT_TRIANGLE structure that
    // references the TRIVERTEX vertices.

    GRADIENT_TRIANGLE gTriangle;
 
    gTriangle.Vertex1 = 0;
    gTriangle.Vertex2 = 1;
    gTriangle.Vertex3 = 2;
 
    // Draw a shaded triangle.

    GradientFill(MemDC, vertex, 3, &gTriangle, 1, GRADIENT_FILL_TRIANGLE);
}
 

/*--------------------------------------------------------------------------
  Pass in any window handle and a tooltip string and this function will set
  the create a tooltip to display on the window if you hover over it.
  -------------------------------------------------------------------------*/
HWND AddToolTip (HWND hWnd,											// Handle for window to put tooltip over 							 
				 TCHAR* tooltip) {									// Text the tool tip should say
	TOOLINFO ti;
	HWND TTWnd;

	if (tooltip == 0) return (0);									// Check we have a tooltip
	InitCommonControls(); 	     									// Check common controls are initialized
	TTWnd = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
		NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, hWnd, 0, 0, 0);								// Create tooltip window
	memset(&ti, 0, sizeof(TOOLINFO));								// Clear structure
    ti.cbSize = sizeof(TOOLINFO);									// Size of structure
    ti.uFlags = TTF_SUBCLASS;										// Class is subclass
    ti.hwnd = hWnd;													// Parent window
    ti.hinst = 0;													// This instance
    ti.uId = 0;														// No uid
    ti.lpszText = tooltip;											// Transfer the text pointer
    GetClientRect (hWnd, &ti.rect);									// Tooltip to cover whole window
    SendMessage(TTWnd, TTM_ADDTOOL, 0, (LPARAM) &ti);				// Add tooltip
	return(TTWnd);                                                  // Return the tooltip window
}



// LdB this is the property title of the FONT to attach to our window
#define TEXTPROP TEXT("FONT")

// subclass procedure for transparent tree
static LRESULT CALLBACK TreeProc( HWND hwnd, UINT message, 
    WPARAM wParam, LPARAM lParam, 
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    switch (message)
    {
    // handle messages that paint tree without WM_PAINT
    case WM_TIMER:  // handles autoscrolling when item is partially visible
    case TVM_DELETEITEM:
    case TVM_INSERTITEM: 
    case WM_MOUSEWHEEL: 
    case WM_HSCROLL:  
    case WM_VSCROLL:  
        {
            ::SendMessage( hwnd, WM_SETREDRAW, (WPARAM)FALSE, 0 );
             LRESULT lres = ::DefSubclassProc( hwnd, message, wParam, lParam );
             ::SendMessage( hwnd, WM_SETREDRAW, (WPARAM)TRUE, 0 );
             ::RedrawWindow( hwnd, NULL, NULL, 
				 RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW );
             return lres;
        }
    case WM_PAINT:
        {
            // usual stuff
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );
			// Simply get printclient to paint  
  			::SendMessage(hwnd, WM_PRINTCLIENT,(WPARAM) hdc, (LPARAM)(PRF_CLIENT) ); 
            EndPaint( hwnd, &ps );
        }
        return 0L;
    case WM_PRINTCLIENT:
		{
	    BITMAPINFO bmi;
		// Get parent client co-ordinates
		RECT rc;
        GetClientRect(GetParent(hwnd), &rc);
		// Create a memory DC
		HDC memDC = CreateCompatibleDC(0);
		// Create a DIB header for parent
		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = rc.right - rc.left;
		bmi.bmiHeader.biHeight = rc.bottom - rc.top;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biCompression = BI_RGB;
		// Create a DIB bitmap
		HBITMAP tempBmp = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, 0, 0, 0);
        // Select tempBmp onto DC to force size and DIB change on the DC
        SelectObject(memDC, tempBmp );
        // Put parent background on our memory DC
        ::SendMessage( GetParent(hwnd), WM_PRINTCLIENT, (WPARAM) memDC,  (LPARAM)(PRF_CLIENT));
		// Now we can dispose of the DIB bitmap no longer needed
		DeleteObject(tempBmp);
		// map tree's coordinates to parent window
        POINT ptTreeUL;
        ptTreeUL.x = 0;
        ptTreeUL.y = 0;
        ClientToScreen( hwnd, &ptTreeUL );
        ScreenToClient( GetParent(hwnd), &ptTreeUL );
		GetClientRect( hwnd, &rc);
		// Transfer parent background to given DC
		BitBlt((HDC)wParam, 0, 0,  rc.right-rc.left, rc.bottom-rc.top,
			memDC, ptTreeUL.x, ptTreeUL.y, SRCCOPY );
	    // Okay get treeview to draw on our memory DC
		DefSubclassProc(hwnd, WM_PRINTCLIENT, (WPARAM)memDC,  (LPARAM)(PRF_CLIENT));
		// Transfer the treeview DC onto finalDC excluding pink
		TransparentBlt((HDC)wParam, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
			memDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, RGB(0xFF, 0x00, 0xFF));
		// Finished with MemDC 
		DeleteObject(memDC);
		return (0); 
		}
    case WM_ERASEBKGND:
        return TRUE;
	case WM_SETFONT:
		LOGFONT lf;
		HFONT OldFont, Newfont;
		GetObject((HFONT)wParam, sizeof(lf), &lf);              // Get proposed font
		if (lf.lfQuality != NONANTIALIASED_QUALITY){            // If its antialiased we can't use
			lf.lfQuality = NONANTIALIASED_QUALITY;              // So change it to non antialiased
			Newfont = CreateFontIndirect(&lf);                  // Create a new font 
			OldFont = (HFONT) GetProp(hwnd, TEXTPROP);			// Get the old font
			if (OldFont != 0) DeleteObject(OldFont);            // If valid delete it
			SetProp(hwnd, TEXTPROP, (HANDLE)Newfont);           // Set new font to property
		} else Newfont = (HFONT) wParam;                        // Use the font provided
		return ::DefSubclassProc( hwnd, WM_SETFONT, (WPARAM)Newfont, lParam); // Treat it as usual
    case WM_NCDESTROY:
		HFONT Font = (HFONT) GetProp(hwnd, TEXTPROP);           // Fetch the font property
		if (Font != 0) DeleteObject(Font);                      // Delete the font
		RemoveProp(hwnd, TEXTPROP);                             // Remove the property   
        ::RemoveWindowSubclass(hwnd, TreeProc, 0 );
        return ::DefSubclassProc( hwnd, message, wParam, lParam);
    }
    return ::DefSubclassProc( hwnd, message, wParam, lParam);
}
 

// ** LdB Consolidate function to create a listbox and do all the repeat stuff
HWND CreateListBox (HWND parent,                               // Parent window to insert this control in
					TCHAR* title,			                   // List box title text
					int x, int y,							   // x,y co-ordinate of parent for the insert
					int cx, int cy,						       // Width and Height of the control
					int id,									   // Id of the control
 				    HFONT hFont) {							   // Handle to any special font (0 = default)
	DWORD dwStyle;
	HWND TreeView;
  
	TreeView = CreateWindowEx(0, WC_TREEVIEW, title, 
		WS_VISIBLE | WS_CHILD | WS_BORDER 
		| TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT, 
		x, y, cx, cy,	parent, (HMENU)id, 0, NULL);

	dwStyle = GetWindowLong( TreeView , GWL_STYLE);
	dwStyle |= TVS_CHECKBOXES;
	SetWindowLongPtr( TreeView , GWL_STYLE, dwStyle );
	SetWindowSubclass( TreeView, TreeProc, 0, 0 );
	TreeView_SetBkColor(TreeView, RGB(0xFF, 0, 0xFF));
	//(TreeView, RGB(0xFF, 0xFF, 0xFF));
	if (hFont == 0) hFont = (HFONT) SendMessage(TreeView, WM_GETFONT, 0, 0);
	SendMessage(TreeView, WM_SETFONT, (WPARAM)hFont, FALSE);


	HIMAGELIST hImages = ImageList_Create( 16, 16, ILC_MASK, 1, 0 );

    // load system icon so we can dodge the deletion and rc.file
	HICON hiBG = reinterpret_cast<HICON>( LoadImage( 0, 
		MAKEINTRESOURCE(IDI_WARNING), 
        IMAGE_ICON, 0, 0, LR_SHARED ) );
 
	ImageList_AddIcon( hImages, hiBG );
 
	TreeView_SetImageList(TreeView, hImages, TVSIL_NORMAL);
	return (TreeView);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CREATE:
		{

			HWND TreeView = CreateListBox(hwnd, (TCHAR*)TEXT("Tree View"), 
				50, 10, 200, 200, ID_LISTBOX, 0);
		
			/************ add items and subitems **********/
 
			// add root item

			TVINSERTSTRUCT tvis = {0};
 
			tvis.item.mask = TVIF_TEXT;
			tvis.item.pszText = TEXT("This is root item");
			tvis.hInsertAfter = TVI_LAST;
			tvis.hParent = TVI_ROOT;
 
			HTREEITEM hRootItem = (HTREEITEM)(SendMessage( TreeView , TVM_INSERTITEM, 0, (LPARAM) &tvis));
 
			// add subitems for the hTreeItem

			for( int i = 0; i < 15; i++ )
			{
				memset( &tvis, 0, sizeof(tvis) );
 
				tvis.item.mask = TVIF_TEXT;
				tvis.item.pszText = TEXT("This is subitem");
				tvis.hInsertAfter = TVI_LAST;
				tvis.hParent = hRootItem;
 
				SendMessage(TreeView , TVM_INSERTITEM, 0, (LPARAM)&tvis);
			}

			TreeView = CreateListBox(hwnd, (TCHAR*)TEXT("Tree View 1"), 
				300, 10, 200, 200, ID_LISTBOX1,  0);
			AddToolTip(TreeView, TEXT("This is list 2"));
		
			/************ add items and subitems **********/
 
			// add root item
			memset( &tvis, 0, sizeof(tvis) );
			tvis.item.mask = TVIF_TEXT;
			tvis.item.pszText = TEXT("2nd list root item");
			tvis.hInsertAfter = TVI_LAST;
			tvis.hParent = TVI_ROOT;
 
			hRootItem = (HTREEITEM)(SendMessage( TreeView , TVM_INSERTITEM, 0, (LPARAM) &tvis));
 
			// add subitems for the hTreeItem

			for( int i = 0; i < 15; i++ )
			{
				memset( &tvis, 0, sizeof(tvis) );
 
				tvis.item.mask = TVIF_TEXT;
				tvis.item.pszText = TEXT("List2 subitem");
				tvis.hInsertAfter = TVI_LAST;
				tvis.hParent = hRootItem;
				SendMessage(TreeView , TVM_INSERTITEM, 0, (LPARAM)&tvis);
			}

		}
		return 0L;
	case WM_PRINTCLIENT:
		{
			RECT r;
			GetClientRect( hwnd, &r );
 
			GradientTriangle( (HDC)wParam, 
				r.right, r.bottom - r.top, 
				r.left, r.bottom - r.top,
				r.left, r.top,
				RGB( 0x0, 0x0, 0xFF ), 
                RGB( 0xFF, 0xFF, 0x0 ) );
 
			GradientTriangle( (HDC)wParam, 
                r.right, r.bottom - r.top, 
				r.right, r.top,
				r.left, r.top, 
				RGB( 0xFF, 0x0, 0x0 ), 
                RGB( 0x0, 0xFF, 0x0 ) );
 			return 0L;
		}
		break;
		// **LdB We accept this message so we can set a minimum window size the users can drag it down too
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpInfo = (LPMINMAXINFO)lParam;
				if (lpInfo){
					lpInfo->ptMinTrackSize.x = 550;
				    lpInfo->ptMinTrackSize.y = 300;
				}
			}
			return 0;
		/** LdB  **/
		// These next two messages are better to use rather than WM_MOVE/WM_SIZE.
		// Remember WM_MOVE/WM_SIZE are from 16bit windows. In 32bit windows the window
		// manager only sends these two messages and the DefWindowProc() handler actually
		// accepts them and converts them to WM_MOVE/WM_SIZE.
		// 
		// We accept this so we can scale our controls to the client size.
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			{
				HDWP hDWP;
				RECT rc;
				
				// Create a deferred window handle.
				// **LdB Deferring 2 child control at the moment (you now have 2 controls in window listbox, listbox1)
				if(hDWP = BeginDeferWindowPos(2)){ 
					GetClientRect(hwnd, &rc);
					
					// **LdB This time I use the SWP_NOMOVE & SWP_NOSIZE so the position and size isn't change 
					// from what you set in the WM_CREATE
					hDWP = DeferWindowPos(hDWP, GetDlgItem(hwnd, ID_LISTBOX), NULL,
						0, 0, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOMOVE | SWP_NOSIZE);

					// **LdB This time I use the SWP_NOMOVE & SWP_NOSIZE so the position and size isn't change 
					// from what you set in the WM_CREATE
					hDWP = DeferWindowPos(hDWP, GetDlgItem(hwnd, ID_LISTBOX1), NULL,
						0, 0, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOMOVE | SWP_NOSIZE);

					// Resize all windows under the deferred window handled at the same time.
					EndDeferWindowPos(hDWP);
					
	                // Now invalidate the window area to force redraw
					InvalidateRect(hwnd, 0, TRUE);
				}
			}
			return 0;
	case WM_ERASEBKGND:
		return (FALSE);
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hwnd, &ps );

			// LdB Simply get printclient to paint you already do all the gradient there  
  			::SendMessage(hwnd, WM_PRINTCLIENT,(WPARAM) hdc, (LPARAM)(PRF_CLIENT) ); 
			EndPaint( hwnd, &ps );
		}
		return 0L;
	case WM_CLOSE:
		{
			// delete tree's imagelist
			HIMAGELIST hImages = reinterpret_cast<HIMAGELIST>( 
                            SendMessage( GetDlgItem( hwnd, ID_LISTBOX ), 
                            TVM_GETIMAGELIST, 0, 0 ) );
 			ImageList_Destroy(hImages);
 
			DestroyWindow(hwnd);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
 
	// initialize common controls
	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES ;
	InitCommonControlsEx(&iccex);
 
	hInst = hInstance;
 
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = TEXT("Transparent TreeView");
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION);
 
	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"), 
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
 
	// LdB rather than repeat the text I used the Classname from above less chance for a typo to get it wrong
	hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, wc.lpszClassName,
		TEXT("The title of my window"), 
                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
		600, 400, NULL, NULL, hInstance, NULL);
 
	if(hwnd == NULL)
	{
		MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"), 
                    MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
 
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
 
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}