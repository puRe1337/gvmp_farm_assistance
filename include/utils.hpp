//#define WIN64_LEAN_AND_MEAN
#pragma once
#include <Windows.h>
#include <gdiplus.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static int GetEncoderClsid( const WCHAR* format, CLSID* pClsid ) {
	UINT num = 0; // number of image encoders
	UINT size = 0; // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize( &num, &size );
	if ( size == 0 )
		return -1; // Failure

	pImageCodecInfo = ( ImageCodecInfo* )( malloc( size ) );
	if ( pImageCodecInfo == NULL )
		return -1; // Failure

	GetImageEncoders( num, size, pImageCodecInfo );

	for ( UINT j = 0; j < num; ++j ) {
		if ( wcscmp( pImageCodecInfo[ j ].MimeType, format ) == 0 ) {
			*pClsid = pImageCodecInfo[ j ].Clsid;
			free( pImageCodecInfo );
			return j; // Success
		}
	}

	free( pImageCodecInfo );
	return -1; // Failure
}


struct gdiplus_init
{
	gdiplus_init( ) {
		GdiplusStartupInput inp;
		GdiplusStartupOutput outp{};
		if ( Ok != GdiplusStartup( &token_, &inp, &outp ) )
			throw std::runtime_error( "GdiplusStartup" );
	}

	~gdiplus_init( ) {
		GdiplusShutdown( token_ );
	}

private:
	ULONG_PTR token_{};
};

static bool take_screenshot( HWND hWnd ) {
	using namespace Gdiplus;

	gdiplus_init gdiplus;

	HDC desktopdc = GetDC( hWnd );
	HDC mydc = CreateCompatibleDC( desktopdc );

	RECT target_rect;
	GetWindowRect( hWnd, &target_rect );
	int width = target_rect.right - target_rect.left;
	int height = target_rect.bottom - target_rect.top;

	HBITMAP mybmp = CreateCompatibleBitmap( desktopdc, width, height );
	HBITMAP oldbmp = ( HBITMAP )SelectObject( mydc, mybmp );
	BitBlt( mydc, 0, 0, width, height, desktopdc, 0, 0, SRCCOPY | CAPTUREBLT );
	SelectObject( mydc, oldbmp );

	Bitmap* b = Bitmap::FromHBITMAP( mybmp, NULL );

	Rect rect2 = { 30, 30, 240, 140 };

	Bitmap* ba = b->Clone( rect2, b->GetPixelFormat( ) );
	CLSID encoderClsid;
	Status stat = GenericError;
	if ( ba && GetEncoderClsid( L"image/png", &encoderClsid ) != -1 ) {
		stat = ba->Save( L"screen.png", &encoderClsid, NULL );
	}
	else {
		delete b;
		delete ba;

		// cleanup
		ReleaseDC( hWnd, desktopdc );
		DeleteObject( mybmp );
		DeleteDC( mydc );
		return false;
	}

	delete b;
	delete ba;

	// cleanup
	ReleaseDC( hWnd, desktopdc );
	DeleteObject( mybmp );
	DeleteDC( mydc );
	return true;
}

static bool string_contains( const std::string& str, const std::string& comp ) {
	if ( str.find( comp ) != std::string::npos )
		return true;
	return false;
}
