#include <tchar.h>
#include <windows.h>
#include <process.h>
#include <commctrl.h>

#include <iostream>
#include <type_traits>
#include <locale>

#include "verify.h"

#if defined( _MSC_VER )
#pragma comment (lib , "Ole32.lib" )
#pragma comment (lib , "User32.lib")
#pragma comment (lib , "Comctl32.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#endif /* defined( _MSC_VER ) */

struct entry_argument_t{
  HINSTANCE hInstance;
  int argc;
  char** argv;
};

/** initialize CRT debug flags*/
static inline void crt_debug_setup(void);
/** initialize windows common controls. */
static int common_control_initialize();

/** application entry point */
static inline unsigned int
entry_point( HINSTANCE hInstance , int argc , char** argv );

static inline unsigned int
entry_point( HINSTANCE hInstance , int argc , char** argv )
{
  std::cout << hInstance << std::endl;
  (void)(argc);
  (void)(argv);

  MessageBox(NULL, TEXT("hello world"),
             TEXT("Hello World"), MB_OK );
  
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
