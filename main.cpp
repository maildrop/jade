#include <tchar.h>
#include <windows.h>
#include <process.h>
#include <commctrl.h>

#include <iostream>
#include <type_traits>
#include <utility>
#include <locale>
#include <thread>

#include "verify.h"

#if defined( _MSC_VER )
#pragma comment (lib , "Ole32.lib" )
#pragma comment (lib , "User32.lib")
#pragma comment (lib , "Gdi32.lib")
#pragma comment (lib , "Comctl32.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#endif /* defined( _MSC_VER ) */

struct entry_argument_t{
  HINSTANCE hInstance;
  int argc;
  char** argv;
};

namespace wh{
  template<typename WindowClassTraits_t>
  struct WindowClassTemplate{
  private:
    template<typename T>
    static auto has_wndproc_f(T&&) ->
      decltype( T::wndProc( std::declval<HWND>() , std::declval<UINT>() , std::declval<WPARAM>() , std::declval<LPARAM>() ) , 
                std::declval<std::true_type>() );
    static auto has_wndproc_f( ... ) -> decltype( std::declval<std::false_type>() );

#if ( __cplusplus < 201703L )
    template<bool b>
    static inline LRESULT wndProc_p( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
    {
      return ::DefWindowProc( hWnd , uMsg , wParam, lParam );
    }
    
    template<>
    static inline LRESULT wndProc_p<true>( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
    {
      return WindowClassTraits_t::wndProc( hWnd, uMsg ,wParam , lParam);
    }
#endif /* ( __cplusplus < 201703L ) */

  public:
    static LRESULT wndProc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
    {
#if ( __cplusplus < 201703L )
      return wndProc_p<std::common_type<decltype( has_wndproc_f(std::declval<Driver>() ) )>::type::value>( hWnd , uMsg , wParam , lParam );
#else /* ( __cplusplus < 201703L ) */
      if constexpr (std::common_type<decltype( has_wndproc_f(std::declval<Driver>() ) )>::type::value){
        return Driver::wndProc( hWnd , uMsg , wParam , lParam );
      }else{
        return DefWindowProc( hWnd , uMsg , wParam, lParam );
      }
#endif /* ( __cplusplus < 201703L ) */
    }
  };
};

/** initialize CRT debug flags*/
static inline void crt_debug_setup(void);
/** initialize windows common controls. */
static int common_control_initialize();

/** application entry point */
static inline unsigned int
entry_point( HINSTANCE hInstance , int argc , char** argv );
static inline LRESULT
service_window_proc( HWND hWnd , UINT uMsg , WPARAM wParam ,LPARAM lParam );
static
HWND& getServiceHWND();

enum{
  WM_APP_BEGIN = (WM_APP+1),
  WM_APP_NULL,
  WM_APP_QUIT,
  WM_APP_STARTUP,
  WM_APP_SHUTDOWN,
  WM_APP_END
};

static
HWND& getServiceHWND()
{
  static HWND hWnd = 0;
  return hWnd;
}



static inline LRESULT
ui_window_proc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
  switch( uMsg ){
  case WM_PAINT:
    {
#if 0
      PAINTSTRUCT paintStruct = {0};
      HDC hDC = BeginPaint( hWnd , &paintStruct );
      VERIFY( hDC );
      if( hDC ){

        
      }
      VERIFY( EndPaint( hWnd , &paintStruct ) );
#else
      VERIFY( ValidateRect( hWnd , NULL ) );
#endif
    }
    return ::DefWindowProc( hWnd , uMsg , wParam , lParam );
  case WM_SETCURSOR:
    if(LOWORD(lParam) == HTCLIENT){
      (void)(SetCursor((HCURSOR)GetClassLongPtr( hWnd , GCLP_HCURSOR )));
      return 1;
    }
    return ::DefWindowProc( hWnd, uMsg , wParam , lParam );
  case WM_DESTROY:
    PostQuitMessage( 0 );
    return ::DefWindowProc( hWnd , uMsg , wParam , lParam );
  default:
    return ::DefWindowProc( hWnd , uMsg , wParam , lParam );
  }
}


static inline LRESULT
service_window_proc( HWND hWnd , UINT uMsg , WPARAM wParam ,LPARAM lParam )
{
  static std::thread uithread{};
  
  switch( uMsg ){
  case WM_NCCREATE:
    return ::DefWindowProc( hWnd, uMsg , wParam , lParam );
  case WM_DESTROY:
    PostQuitMessage(0);
    return DefWindowProc( hWnd , uMsg, wParam , lParam );
  case WM_APP_QUIT:
    VERIFY( DestroyWindow( hWnd ) );
    return 1;
  case WM_APP_SHUTDOWN:
    uithread.join();
    VERIFY( PostMessage( hWnd , WM_APP_QUIT , 0 , 0 ) );
    return 1;
  case WM_APP_STARTUP:
    // common-controls manifest+ check 
    // MessageBoxEx(NULL, TEXT("Hello world"),TEXT("エラー"), MB_OK,MAKELANGID( LANG_NEUTRAL,SUBLANG_DEFAULT ));
    uithread = std::thread{ [](){
      HRESULT const hr = CoInitializeEx( nullptr ,  COINIT_APARTMENTTHREADED );
      VERIFY( S_OK == hr );
      if( SUCCEEDED( hr ) ){
        HWND hWnd = CreateWindowEx( 0L,
                                    TEXT("wh-window-class-2b525389-5b3e-4f3e-ab44-c2a3ffe343bc"),
                                    TEXT("jade"),
                                    WS_OVERLAPPEDWINDOW ,
                                    CW_USEDEFAULT , CW_USEDEFAULT ,
                                    1024 , 800 ,
                                    NULL,NULL, GetModuleHandle( NULL ) , nullptr );
        
        VERIFY( hWnd );
        if( hWnd ){
          
          (void)ShowWindow( hWnd , SW_SHOW );
          VERIFY( InvalidateRect( hWnd , nullptr , TRUE ) );

          MSG msg = {0};
          BOOL bRet;
          while( 0 != (bRet = GetMessage( &msg, NULL , 0, 0 )) ){ 
            if (bRet == -1){
              // handle the error and possibly exit
              assert( !"GetMessage() failed");
              break;
            }else{
              TranslateMessage(&msg); 
              DispatchMessage(&msg); 
            }
          }
        }
      }
      PostMessage( getServiceHWND() , WM_APP_SHUTDOWN , 0 ,0 );
      return 0;
    }};
    return 1;
  default:
    return ::DefWindowProc( hWnd, uMsg , wParam , lParam );
  }
}


static inline unsigned int
entry_point( HINSTANCE hInstance , int argc , char** argv )
{
  std::cout << hInstance << std::endl;
  (void)(argc);
  (void)(argv);

  struct RegisterdClass{
    ATOM atom;
    RegisterdClass(ATOM&& atom) : atom(std::move(atom))
    {
    }
    RegisterdClass(const RegisterdClass& ) = delete;
    RegisterdClass& operator=( const RegisterdClass& ) = delete;
    
    ~RegisterdClass()
    {
      VERIFY( UnregisterClass( (LPCTSTR)( atom ), GetModuleHandle(NULL) ));
    }

    operator LPCTSTR() const
    {
      return (LPCTSTR)(atom);
    }
    
    operator bool() const
    {
      return (atom ? true : false );
    }
    operator ATOM() const
    {
      return atom;
    }
  };
  
  const WNDCLASSEX wndClassEx = {
    sizeof( WNDCLASSEX ),
    0,service_window_proc ,0,0, GetModuleHandle(NULL),
    (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED),
    (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED),
    (HBRUSH)GetStockObject(WHITE_BRUSH),
    nullptr ,
    TEXT("wh-window-class-05e3a2f6-3161-457a-b87a-1ac711b8dd15"),
    (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR) };
  RegisterdClass regClass{ RegisterClassEx( &wndClassEx ) };
  VERIFY( static_cast<bool>(regClass) );

  const WNDCLASSEX uiWndClsEx = {
    sizeof( WNDCLASSEX ),
    0,ui_window_proc ,0,0, GetModuleHandle(NULL),
    (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED),
    (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED),
    (HBRUSH)GetStockObject(DKGRAY_BRUSH),
    nullptr ,
    TEXT("wh-window-class-2b525389-5b3e-4f3e-ab44-c2a3ffe343bc"),
    (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR) };
  RegisterdClass uiWndClass{ RegisterClassEx( &uiWndClsEx ) };
  VERIFY( static_cast<bool>( uiWndClass ));

  if( regClass ){
    HWND const hWnd =
      (getServiceHWND() = CreateWindowEx(0L, 
                                         (LPCTSTR)regClass ,
                                         TEXT("wh-service-window"),
                                         0L,
                                         CW_USEDEFAULT , 0,
                                         CW_USEDEFAULT , 0,
                                         HWND_MESSAGE ,
                                         0, 
                                         GetModuleHandle( NULL ),
                                         0 ) );
    if( hWnd ){
      PostMessage( hWnd , WM_APP_STARTUP , 0 ,0  );
      {
        MSG msg = {0};
        BOOL bRet;
        while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0){ 
          if (bRet == -1){
            // handle the error and possibly exit
            break;
          }else{
            //TranslateMessage(&msg); 
            DispatchMessage(&msg); 
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

static int common_control_initialize()
{
  // 今 InitCommonControls() を呼び出しているので、 Comctl32.dll は暗黙的にリンクしている
  // しかしながら、 求められているのは InitCommonControlsEx であるので、そちらが存在していればそれを使う 
  HMODULE comctl32 = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Comctl32.dll", &comctl32)) {
    return EXIT_FAILURE;
  }

  assert(comctl32 != nullptr);
  if (comctl32) {
    { // InitCommonControlsEx を使用して初期化を試みる
      typename std::add_pointer< decltype(InitCommonControlsEx) >::type lpfnInitCommonControlsEx =
        reinterpret_cast<typename std::add_pointer< decltype(InitCommonControlsEx) >::type>(GetProcAddress(comctl32, "InitCommonControlsEx"));
      // InitCommonControlsEx が見つかった場合
      if (lpfnInitCommonControlsEx) {
        const INITCOMMONCONTROLSEX initcommoncontrolsex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
        if (!lpfnInitCommonControlsEx(&initcommoncontrolsex)) {
          assert(!" InitCommonControlsEx(&initcommoncontrolsex) ");
          return EXIT_FAILURE;
        }
        return 0;
      }
    }
    { //InitCommonControls を使用して初期化を試みる
      ::InitCommonControls();
      return 0;
    }
  }
  return 1;
}

static inline void crt_debug_setup(void)
{
#if defined(_DEBUG)
  using dbgflag_t = std::make_unsigned<decltype( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) )>::type ;
  dbgflag_t dbgflag = static_cast<dbgflag_t>( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) );
  
  (void)(_CrtSetDbgFlag( static_cast<int>(dbgflag) ));
  _CrtSetReportMode( _CRT_ASSERT , _CRTDBG_MODE_WNDW );
#endif /* defined(_DEBUG) */
}

int main(int argc,char* argv[])
{
  do { 
    HANDLE const processHeapHandle = ::GetProcessHeap(); 
    VERIFY( processHeapHandle != NULL );
    if( processHeapHandle ){
      VERIFY( HeapSetInformation(processHeapHandle, HeapEnableTerminationOnCorruption, NULL, 0) );
#if defined(_MSC_VER)
      { // MSVCRT のヒープに対しても、破壊をチェックするように設定
        // MSVCRT の _get_heap_handle() は intptr_t を返す
        HANDLE crtHeapHandle = reinterpret_cast<HANDLE>( _get_heap_handle()); 
        VERIFY( crtHeapHandle != NULL );
        if( crtHeapHandle && processHeapHandle != crtHeapHandle ){
          VERIFY( HeapSetInformation(crtHeapHandle, HeapEnableTerminationOnCorruption, NULL, 0) );
          
          // 追加でLow-Fragmentation-Heap を使用するように指示する。
          ULONG heapInformaiton(2L); // Low Fragmention Heap を指示
          VERIFY( HeapSetInformation(crtHeapHandle, HeapCompatibilityInformation, 
                                     &heapInformaiton, sizeof( heapInformaiton)));
          
        }
      }
#endif /* defined( _MSC_VER ) */
    }
  }while( false );

  (void)(std::locale::global( std::locale("")));
  crt_debug_setup();
  
  do{
    HMODULE kernel32ModuleHandle = GetModuleHandle( _T("kernel32")); // kernel32 は 実行中アンロードされないからこれでよし
    VERIFY( kernel32ModuleHandle );
    if( ! kernel32ModuleHandle ){
      break;
    }
    FARPROC const pFarProc = GetProcAddress( kernel32ModuleHandle , "SetDllDirectoryW");
    if( ! pFarProc ){
      break;
    }
    decltype(SetDllDirectoryW) * const pSetDllDirectoryW(reinterpret_cast<decltype(SetDllDirectoryW)*>(pFarProc) );
    VERIFY( pSetDllDirectoryW(L"") ); /* サーチパスからカレントワーキングディレクトリを排除する */
  }while( false );

  HRESULT const hr = CoInitializeEx( nullptr ,  COINIT_MULTITHREADED );
  VERIFY( S_OK == hr );
  if( !SUCCEEDED( hr )){
    return 3;
  }else{
    struct ComRAII{
      ~ComRAII(){
        CoUninitialize();
      }
    } comRaii;

    VERIFY( 0 == common_control_initialize() );

    int result = 0;
    entry_argument_t entry_argument{ GetModuleHandle( NULL ), argc , argv };
      
    uintptr_t thread_handle = _beginthreadex(nullptr , 0 ,  [](void* arg)->unsigned{
      entry_argument_t * const entry_argument = static_cast<entry_argument_t*>( arg );
      VERIFY( GetModuleHandle( NULL ) == entry_argument->hInstance );
      return entry_point( entry_argument->hInstance ,
                          entry_argument->argc ,
                          entry_argument->argv );
    },static_cast<void*>(&entry_argument),0, nullptr );
    if( thread_handle ){
      for( ;; ){
        DWORD const dw(WaitForSingleObject( HANDLE(thread_handle), INFINITE ));
        switch( dw ){
        case WAIT_OBJECT_0:
          {
           DWORD exitCode = 0;
           VERIFY( GetExitCodeThread( HANDLE( thread_handle ) , &exitCode ) );
           result = (int)( exitCode );
          }
          goto THREAD_TERMINATED;
        case WAIT_ABANDONED:
          goto THREAD_TERMINATED;
        case WAIT_TIMEOUT:
          continue;
        case WAIT_FAILED:
          assert( !"WaitForSingleObject return WAIT_FAILURE" );
          goto THREAD_TERMINATED;
        default:
          break;
        }
      }
  THREAD_TERMINATED:
      VERIFY( CloseHandle( HANDLE(thread_handle) ) );
    }
    result = EXIT_SUCCESS;
    return result;
  }
}
