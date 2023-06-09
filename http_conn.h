#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>

#include "../14/locker.h"

class http_conn
{
    public:
        static const int FILENAME_LEN = 200;
        static const int READ_BUFFER_SIZE = 2048;
        static const int WRITE_BUFFER_SIZE = 1024;
        enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH};
        enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
        enum HTTP_CODE {NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};
        enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};

    public:
        http_conn() {}
        ~http_conn() {}
    
    public:
        /*初始化新接受的连接*/
        void init(int sockfd, const sockaddr_in &addr);
        void close_conn(bool read_close = true);
        void process();
        /*读写非阻塞*/
        bool read();
        bool write();
    
    private:
        void init();
        HTTP_CODE process_read();
        bool process_write(HTTP_CODE ret);

        HTTP_CODE parse_request_line(char *text);
        HTTP_CODE parse_headers(char *text);
        HTTP_CODE parse_content(char *text);
        HTTP_CODE do_request();
        char *get_line() {return m_read_buf + m_start_line;}
        LINE_STATUS parse_line();

        void unmap();
        bool add_response(const char *format, ...);
        bool add_content(const char *content);
        bool add_status_line(int status, const char *title);
        bool add_headers(int content_length);
        bool add_content_length(int content_length);
        bool add_linger();
        bool add_blank_line();
    
    public:
        /*所有socket的事件都注册到同一个epoll，所以为静态*/
        static int m_epollfd;
        static int m_user_count;

    private:
        int m_sockfd;
        sockaddr_in m_address;

        char m_read_buf[READ_BUFFER_SIZE];
        int m_read_idx;/*读缓冲中已经读入的客户数据的最后一个字节的下一个位置*/
        int m_checked_idx;/*当前正在分析的字符在读缓冲区的位置*/
        int m_start_line;/*当前正在解析的行的起始位置*/
        char m_write_buf[WRITE_BUFFER_SIZE];
        int m_write_idx;/*写缓冲区中待发送的字节数*/

        CHECK_STATE m_check_state;
        METHOD m_method;

        char m_real_file[FILENAME_LEN];/*客户请求的目标文件的完整路径，其内容等于doc_root + m_url, doc_root是网站根目录*/
        char *m_url;/*客户请求的目标文件的文件名*/
        char *m_version;/*HTTP协议版本号*/
        char *m_host;/*主机名*/
        int m_content_length;/*HTTP请求的消息体的长度*/
        bool m_linger;/*HTTP请求是否要求保持连接*/

        char *m_file_address;/*客户请求的目标文件被mmap到内存中的起始位置*/
        struct stat m_file_stat;/*目标文件的状态，判断文件是否存在、是否为目录、是否可读，并获取文件大小等*/
        struct iovec m_iv[2];/*使用writev执行写操作*/
        int m_iv_count;/*被写内存卡的数量*/
};

#endif