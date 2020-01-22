#pragma once
/**
   verify.h 
*/
#if !defined( VERIFY_H_46c54cfc_4a81_4a6c_9c42_ff3b0f1e4082_HEADER_GUARD )
#define VERIFY_H_46c54cfc_4a81_4a6c_9c42_ff3b0f1e4082_HEADER_GUARD 1

#if defined( __cplusplus )
#include <cassert>
#else /* defined( __cplusplus ) */
#include <assert.h>
#endif /* defined( __cplusplus ) */

#if !defined( VERIFY )
#if defined( NDEBUG )
#define VERIFY(_exp) (void)( _exp )
#else /* defined( NDEBUG ) */
#define VERIFY(_exp) assert( _exp )
#endif /* defined( NDEBUG ) */
#endif /* ! defined( VERIFY ) */

#if defined(_MSC_VER)
#include <crtdbg.h> 
#endif /* defined(_MSC_VER) */

#if defined( __cplusplus )
extern "C" {
#endif /* defined( __cplusplus ) */

  
#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* VERIFY_H_46c54cfc_4a81_4a6c_9c42_ff3b0f1e4082_HEADER_GUARD */
