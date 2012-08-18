/*---------------------------------------------------------------------------
    http.h - http component

    Generated from http.icl by icl_gen using GSL/4.
    
    Copyright (c) 1996-2009 iMatix Corporation
    All rights reserved.
    
    This file is licensed under the BSD license as follows:
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of iMatix Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY IMATIX CORPORATION "AS IS" AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL IMATIX CORPORATION BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    These header files use macros to implement a split inclusion in which all
    safe definitions (those that do not depend on the presence of other
    definitions) are done first, and all unsafe definitions are done in a
    second pass through the same headers.

    The first header file included from the main C program defines itself as
    the "root" and thus controls the inclusion of the safe/unsafe pieces of
    the other headers.
 *---------------------------------------------------------------------------*/

#if !defined (ICL_IMPORT_HEADERS) || (ICL_IMPORT_HEADERS == 1)
# ifndef INCLUDE_HTTP_SAFE
#   define INCLUDE_HTTP_SAFE
#   define INCLUDE_HTTP_ACTIVE
#   if !defined (ICL_IMPORT_HEADERS)
#       define ICL_IMPORT_HTTP_ROOT
#       define ICL_IMPORT_HEADERS 1
#   endif
#   ifdef __cplusplus
extern "C" {
#   endif

#   ifdef __cplusplus
}
#   endif

#   include "http_access_context.h"
#   include "http_access_dos.h"
#   include "http_access_logger.h"
#   include "http_access_module.h"
#   include "http_access_policy.h"
#   include "http_access_refresh.h"
#   include "http_address.h"
#   include "http_agent.h"
#   include "http_config.h"
#   include "http_driver_cgi.h"
#   include "http_driver_context.h"
#   include "http_driver_file.h"
#   include "http_driver_module.h"
#   include "http_driver_null.h"
#   include "http_request.h"
#   include "http_static.h"
#   include "http_response.h"
#   include "http_server.h"
#   ifdef __cplusplus
extern "C" {
#   endif

#   ifdef __cplusplus
}
#   endif
#   undef INCLUDE_HTTP_ACTIVE
#   if defined (ICL_IMPORT_HTTP_ROOT)
#       undef  ICL_IMPORT_HEADERS
#       define ICL_IMPORT_HEADERS 2
#   endif
# endif
#endif

#if !defined (ICL_IMPORT_HEADERS) || (ICL_IMPORT_HEADERS == 2)
# ifndef INCLUDE_HTTP_UNSAFE
#   define INCLUDE_HTTP_UNSAFE
#   define INCLUDE_HTTP_ACTIVE

#   include "http_access_context.h"
#   include "http_access_dos.h"
#   include "http_access_logger.h"
#   include "http_access_module.h"
#   include "http_access_policy.h"
#   include "http_access_refresh.h"
#   include "http_address.h"
#   include "http_agent.h"
#   include "http_config.h"
#   include "http_driver_cgi.h"
#   include "http_driver_context.h"
#   include "http_driver_file.h"
#   include "http_driver_module.h"
#   include "http_driver_null.h"
#   include "http_request.h"
#   include "http_static.h"
#   include "http_response.h"
#   include "http_server.h"

#   ifdef __cplusplus
extern "C" {
#   endif
#   ifdef __cplusplus
}
#   endif
#   ifdef __cplusplus
extern "C" {
#   endif

#   ifdef __cplusplus
}
#   endif
#   undef INCLUDE_HTTP_ACTIVE
#   if defined (ICL_IMPORT_HTTP_ROOT)
#       undef  ICL_IMPORT_HEADERS
#       define ICL_IMPORT_HEADERS 3
#   endif
# endif
#endif

#if !defined (ICL_IMPORT_HEADERS) || (ICL_IMPORT_HEADERS == 3)
# if !defined (INCLUDE_HTTP_INLINE)
#   define INCLUDE_HTTP_INLINE
#   define INCLUDE_HTTP_ACTIVE

#   include "http_access_context.h"
#   include "http_access_dos.h"
#   include "http_access_logger.h"
#   include "http_access_module.h"
#   include "http_access_policy.h"
#   include "http_access_refresh.h"
#   include "http_address.h"
#   include "http_agent.h"
#   include "http_config.h"
#   include "http_driver_cgi.h"
#   include "http_driver_context.h"
#   include "http_driver_file.h"
#   include "http_driver_module.h"
#   include "http_driver_null.h"
#   include "http_request.h"
#   include "http_static.h"
#   include "http_response.h"
#   include "http_server.h"

#   ifdef __cplusplus
extern "C" {
#   endif

#if (defined (BASE_TRACE)   || defined (BASE_TRACE_HTTP) )
void
    http_dump (icl_os_thread_t thread, apr_time_t time, qbyte info);
#endif

#   ifdef __cplusplus
}
#   endif
#   undef INCLUDE_HTTP_ACTIVE
#   if defined (ICL_IMPORT_HTTP_ROOT)
#       undef ICL_IMPORT_HEADERS
#       undef ICL_IMPORT_HTTP_ROOT
#   endif
# endif
#endif
