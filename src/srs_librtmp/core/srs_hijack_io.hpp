/*
The MIT License (MIT)

Copyright (c) 2013-2015 SRS(ossrs)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SRS_HIJACK_IO_H
#define SRS_HIJACK_IO_H

/*
#include <srs_hijack_io.hpp>
*/

#include <srs_core.hpp>

#include <string>

/*************************************************************
**************************************************************
* IO hijack, use your specified io functions.
**************************************************************
*************************************************************/
// the void* will convert to your handler for io hijack.
typedef void* srs_hijack_io_t;
#ifdef SRS_HIJACK_IO
    #ifndef _WIN32
        // for iovec.
        #include <sys/uio.h>
    #endif
    /**
    * get the hijack io object in rtmp protocol sdk.
    * @remark, user should never provides this method, srs-librtmp provides it.
    */
    extern srs_hijack_io_t srs_hijack_io_get(void* rtmp);
#endif
// define the following macro and functions in your module to hijack the io.
// the example @see https://github.com/winlinvip/st-load
// which use librtmp but use its own io(use st also).
#ifdef SRS_HIJACK_IO
    /**
    * create hijack.
    * @return NULL for error; otherwise, ok.
    */
    extern srs_hijack_io_t srs_hijack_io_create();
    /**
    * destroy the context, user must close the socket.
    */
    extern void srs_hijack_io_destroy(srs_hijack_io_t ctx);
    /**
    * create socket, not connect yet.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_create_socket(srs_hijack_io_t ctx);
    /**
    * set the socket connect timeout.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_set_connect_timeout(srs_hijack_io_t ctx, int64_t timeout_us);
    /**
    * connect socket at server_ip:port.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_connect(srs_hijack_io_t ctx, const char* server_ip, int port);
    /**
    * read from socket.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_read(srs_hijack_io_t ctx, void* buf, size_t size, ssize_t* nread);
    /**
    * set the socket recv timeout.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_set_recv_timeout(srs_hijack_io_t ctx, int64_t timeout_us);
    /**
    * get the socket recv timeout.
    * @return 0, success; otherswise, failed.
    */
    extern int64_t srs_hijack_io_get_recv_timeout(srs_hijack_io_t ctx);
    /**
    * get the socket recv bytes.
    * @return 0, success; otherswise, failed.
    */
    extern int64_t srs_hijack_io_get_recv_bytes(srs_hijack_io_t ctx);
    /**
    * set the socket send timeout.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_set_send_timeout(srs_hijack_io_t ctx, int64_t timeout_us);
    /**
    * get the socket send timeout.
    * @return 0, success; otherswise, failed.
    */
    extern int64_t srs_hijack_io_get_send_timeout(srs_hijack_io_t ctx);
    /**
    * get the socket send bytes.
    * @return 0, success; otherswise, failed.
    */
    extern int64_t srs_hijack_io_get_send_bytes(srs_hijack_io_t ctx);
    /**
    * writev of socket.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_writev(srs_hijack_io_t ctx, const iovec *iov, int iov_size, ssize_t* nwrite);
    /**
    * whether the timeout is never timeout.
    * @return 0, success; otherswise, failed.
    */
    extern bool srs_hijack_io_is_never_timeout(srs_hijack_io_t ctx, int64_t timeout_us);
    /**
    * read fully, fill the buf exactly size bytes.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_read_fully(srs_hijack_io_t ctx, void* buf, size_t size, ssize_t* nread);
    /**
    * write bytes to socket.
    * @return 0, success; otherswise, failed.
    */
    extern int srs_hijack_io_write(srs_hijack_io_t ctx, void* buf, size_t size, ssize_t* nwrite);
#endif

/*************************************************************
**************************************************************
* Windows SRS-LIBRTMP solution
**************************************************************
*************************************************************/
// for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
#ifdef _WIN32
    // for time.
    #define _CRT_SECURE_NO_WARNINGS
    #include <time.h>
    int gettimeofday(struct timeval* tv, struct timezone* tz);
    #define PRId64 "lld"
    
    // for inet helpers.
    typedef int socklen_t;
    const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
    
    // for mkdir().
    #include<direct.h>
    
    // for open().
    typedef int mode_t;
    #define S_IRUSR 0
    #define S_IWUSR 0
    #define S_IXUSR 0
    #define S_IRGRP 0
    #define S_IWGRP 0
    #define S_IXGRP 0
    #define S_IROTH 0
    #define S_IXOTH 0
    
    // for file seek.
    #include <io.h>
    #include <fcntl.h>
    #define open _open
    #define close _close
    #define lseek _lseek
    #define write _write
    #define read _read
    
    // for pid.
    typedef int pid_t;
    pid_t getpid(void);
    
    // for socket.
    ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
    typedef int64_t useconds_t;
    int usleep(useconds_t usec);
    int socket_setup();
    int socket_cleanup();
    
    // others.
    #define snprintf _snprintf
#endif


#endif
