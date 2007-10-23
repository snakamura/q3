#ifdef __cplusplus
extern "C" {
#endif

#define		CR			TEXT('\r')
#define		LF			TEXT('\n')
#define		ESC			TEXT('\033')

#define		ERROR_FONT	(0x20000000)

#define		MODE_NORMAL		(0)
#define		MODE_QUOTE		(1)
#define		MODE_BASE64		(2)
#define		MODE_NOCONV		(3)

#define		LOCALE_US		(0)
#define		LOCALE_USONLY	(1)
#define		LOCALE_JAPAN	(2)
#define		LOCALE_CHINA	(3)
#define		LOCALE_US20		(4)

BOOL	InitKanjiControls( void ) ;
void	ReleaseKanjiControls( void ) ;
void	KDrawText( HDC hDC, LPCTSTR str, int len, LPRECT pos, UINT format ) ;
void	KDrawTextW( HDC hDC, LPCTSTR str, int len, LPRECT pos, UINT format ) ;
void	KDrawTextI( HDC hDC, LPCTSTR str, int len, LPRECT pos, UINT format ) ;
BOOL	han2zen_kanastr( const BYTE *src, LPBYTE dst, DWORD dstlen, BOOL fHira ) ;
DWORD	unicode2sjis( LPCTSTR src, BYTE *dst, DWORD max ) ;
DWORD	sjis2unicode( const BYTE *src, LPTSTR dst, DWORD max ) ;
void	error_dialog( LPCTSTR message, LPCTSTR opt ) ;
void	debug_dialog( LPCTSTR message, LPCTSTR opt ) ;
DWORD	GetFontZW( void ) ; /* 全角横ドット数 */
DWORD	GetFontHW( void ) ; /* 半角横ドット数 */
DWORD	GetFontH( void ) ;  /* 縦ドット数 */
void	SetTabWidth( DWORD tab ) ;	/* タブ設定 */
DWORD	GetTabWidth( void ) ;		/* タブ取得 */
DWORD	GetKVersion( void ) ;
void	unicode_jis_char( TCHAR c ) ;
void	jis_unicode_char( TCHAR c ) ;
void	setup_codeconv( LPTSTR ptr ) ;
DWORD	getcount_codeconv( void ) ;
void	putchar_codeconv( TCHAR c ) ;
void	puts_codeconv( LPCTSTR str ) ;
void	set_convmode( DWORD mode ) ;

WORD	sjis2jis_char( WORD sjis ) ;
WORD	jis2sjis_char( WORD jis ) ;
TCHAR	sjis2unicode_char( WORD sjis ) ;
WORD	unicode2sjis_char( TCHAR unicode ) ;
BOOL	is_hankaku( TCHAR c ) ;

DWORD	GetKLocale( void ) ;

DWORD	GetNumFont( void ) ;
void	SetCurFont( DWORD index ) ;
DWORD	GetCurFont( void ) ;

#ifdef __cplusplus
} ;
#endif
