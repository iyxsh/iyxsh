#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>  // For pthread functions
#include <time.h>     // For clock_gettime

// 添加获取本机IP所需的头文件
#ifdef _WIN32
    #include <winsock2.h>
    #include <iphlpapi.h>
    #include <io.h> // 用于Windows平台上的write函数
    #define STDERR_FILENO 2 // 定义标准错误文件描述符
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <netdb.h>
    #include <unistd.h>
#endif
#include "./apihandle/handle.h"
#include "./logger/logger.h"
#include "./config/json_config.h"
#include "./filemanager/filequeue.h"
#include "./filemanager/LibreOfficeService.h"
#include "./filemanager/template_index_cache.h"
#include "./filemanager/apiops.h"
#include "./error/error_codes.h"
#include "./filemanager/utils.h"
#include <csignal> // 用于signal函数
#include <cstdlib> // 用于set_terminate和exit函数
#include <cxxabi.h> // 用于获取异常类型名称
#include <string.h> // 用于memset和strlen函数

// 新增外部声明
extern void deal_filestatus_request(RequestBody requestbody, ResponseBody *responsebody);
extern void deal_opendata_request(RequestBody requestbody, ResponseBody *responsebody);
#include <iostream>
/* ======= dbmanager.c  ======= */
// extern GridCircle *gridcircldata; // json文件默认配置读取
extern char *defaultdbname;

// HTTPS配置
const char *CERT_FILE = "server.crt";
const char *KEY_FILE = "server.key";
const char *CA_FILE = "ca.crt";

// 新增全局变量用于资源管理
static int server_fd = -1;
static SSL_CTX *global_ctx = NULL;
volatile sig_atomic_t g_running = 1; // 控制主循环的全局标志
std::vector<pthread_t> client_threads; // 存储客户端线程ID
std::mutex client_threads_mutex; // 保护client_threads的互斥锁
volatile sig_atomic_t g_server_stopping = 0; // 服务器停止标志，用于通知客户端线程

// 前置声明
namespace filemanager {
    class DocumentManager;
    class TemplateIndexCacheManager;
}

// 全局单例清理函数
void cleanup_global_singletons() {
    try {
        logger_log_info("Cleaning up global singletons");
        
        // 由于我们不能在这里直接访问单例的getInstance()方法
        // 我们需要确保在程序退出时，所有资源已经被正确释放
        // 这里我们主要是等待所有线程完成，并确保重要的资源已经被释放
        
        // 强制刷新所有未写入的日志
        fflush(stdout);
        fflush(stderr);
        
        // 在Windows平台上，我们可以尝试强制释放一些可能导致问题的资源
#ifdef _WIN32
        logger_log_info("Running Windows-specific cleanup");
        // 等待一小段时间，确保所有线程都有机会完成
        Sleep(100);
        // 强制刷新所有未写入的磁盘操作
        _flushall();
#else
        // 在POSIX平台上
        logger_log_info("Running POSIX-specific cleanup");
        // 等待一小段时间，确保所有线程都有机会完成
        usleep(100000); // 100毫秒
        // 强制刷新所有未写入的磁盘操作
        fflush(NULL);
#endif
        
        logger_log_info("Global singleton cleanup completed");
    } catch (const std::exception& e) {
        logger_log_error("Exception during global singleton cleanup: %s", e.what());
    }
}

// 全局未捕获异常处理器
void uncaught_exception_handler() {
    try {
        // 尝试重新抛出当前异常以获取其类型信息
        std::exception_ptr current_exception = std::current_exception();
        if (current_exception) {
            std::rethrow_exception(current_exception);
        }
        logger_log_error("Unknown uncaught exception (no type information available)");
    } catch (const std::__1::system_error& e) {
        // Note: In libc++, std::system_error is an alias for std::__1::system_error
        logger_log_error("Uncaught std::__1::system_error in global handler: %s", e.what());
        logger_log_error("Error code: %d, Category: %s", e.code().value(), e.code().category().name());
    } catch (const std::exception& e) {
        logger_log_error("Uncaught std::exception in global handler: %s", e.what());
        // 尝试获取异常的实际类型名称
        try {
            throw;
        } catch (const std::exception& ex) {
            const char* name = typeid(ex).name();
            int status = 0;
            char* demangled = abi::__cxa_demangle(name, 0, 0, &status);
            if (status == 0 && demangled) {
                logger_log_error("Exception type: %s", demangled);
                free(demangled);
            } else {
                logger_log_error("Exception type: %s", name);
            }
        }
    } catch (...) {
        logger_log_error("Uncaught unknown exception in global handler");
    }
    
    // 确保日志刷新
    fflush(stdout);
    fflush(stderr);
    
    // 在Windows上，这里可以添加更多的清理操作
    // 注意：在全局异常处理器中不要做太多复杂的操作
}

// 定义路由处理函数类型
typedef void (*route_handler)(SSL *ssl, RequestBody RequestBody);

// 路由表结构体
struct Route
{
    const char *method;
    const char *path;
    route_handler handler;
};

// 获取本机IP地址的跨平台函数
bool getLocalIP(char *ip, size_t ip_len) {
    if (ip == NULL || ip_len < INET_ADDRSTRLEN) {
        return false;
    }

#ifdef _WIN32
    // Windows平台实现
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        fprintf(stderr, "gethostname failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    // 使用更现代的getaddrinfo替代过时的gethostbyname
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL, *ptr = NULL;
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        fprintf(stderr, "getaddrinfo failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    // 遍历所有返回的地址
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ptr->ai_addr;
            const char *temp_ip = inet_ntoa(addr->sin_addr);
            // 跳过环回地址
            if (strcmp(temp_ip, "127.0.0.1") != 0) {
                strncpy(ip, temp_ip, ip_len - 1);
                ip[ip_len - 1] = '\0';
                freeaddrinfo(result);
                WSACleanup();
                return true;
            }
        }
    }

    // 如果没有找到非环回地址
    fprintf(stderr, "No non-loopback IPv4 address found\n");
    freeaddrinfo(result);
    WSACleanup();
    return false;
#else
    // Linux/Unix平台实现
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return false;
    }

    // 遍历所有网络接口
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        // 只处理IPv4地址
        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }

            // 跳过环回地址
            if (strcmp(host, "127.0.0.1") != 0) {
                strncpy(ip, host, ip_len - 1);
                ip[ip_len - 1] = '\0';
                freeifaddrs(ifaddr);
                return true;
            }
        }
    }

    // 如果没有找到非环回地址
    fprintf(stderr, "No non-loopback IPv4 address found\n");
    freeifaddrs(ifaddr);
    return false;
#endif
}

/********************** HTTPS初始化 ***********************/
SSL_CTX *create_ssl_context()
{
    SSL_CTX *ctx = NULL;
    
    try {
        // 初始化OpenSSL库
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        
        // 创建SSL上下文，使用TLS服务器方法
        ctx = SSL_CTX_new(TLS_server_method());
        
        if (!ctx) {
            fprintf(stderr, "Failed to create SSL context: %s\n", ERR_error_string(ERR_get_error(), NULL));
            return NULL;
        }

        // 添加证书验证
        if (!SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
        {
            fprintf(stderr, "证书加载失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
            SSL_CTX_free(ctx);
            return NULL;
        }
        
        if (!SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
        {
            fprintf(stderr, "私钥加载失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
            SSL_CTX_free(ctx);
            return NULL;
        }
        
        if (!SSL_CTX_check_private_key(ctx))
        {
            fprintf(stderr, "证书与私钥不匹配\n");
            SSL_CTX_free(ctx);
            return NULL;
        }
        
        // 加载CA证书链（必须包含Nginx使用的根证书）(验证客户端)
        if (!SSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
        {
            fprintf(stderr, "无法加载CA证书链: %s\n", ERR_error_string(ERR_get_error(), NULL));
            SSL_CTX_free(ctx);
            return NULL;
        }

        // 配置证书链验证深度
        SSL_CTX_set_verify_depth(ctx, 4); // 适应多级证书

        // 设置加密套件
        // TLS 1.3加密套件
        const char *tls13_ciphers = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256";
        SSL_CTX_set_ciphersuites(ctx, tls13_ciphers);
        
        // TLS 1.2及以下加密套件
        const char *legacy_ciphers = "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256";
        if (SSL_CTX_set_cipher_list(ctx, legacy_ciphers) != 1) {
            fprintf(stderr, "Failed to set cipher list: %s\n", ERR_error_string(ERR_get_error(), NULL));
        }

        // 初始化会话ID上下文（修复SSL_ERROR_SESSION_ID_CONTEXT）
        const char session_id_ctx[] = "DINGDIAN_CTX";
        if (!SSL_CTX_set_session_id_context(ctx, (const unsigned char *)session_id_ctx, sizeof(session_id_ctx) - 1))
        {
            fprintf(stderr, "Failed to set session ID context\n");
            SSL_CTX_free(ctx);
            return NULL;
        }

        // 安全配置优化
        // 禁用SSLv2和SSLv3
        SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
        
        // 启用会话缓存，提高性能
        SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(ctx, 128); // 设置会话缓存大小
        
        // 设置会话超时时间（300秒）
        SSL_CTX_set_timeout(ctx, 300);

        // 添加X.509扩展（必须包含服务器IP）
        X509 *cert = SSL_CTX_get0_certificate(ctx);
        if (!cert) {
            fprintf(stderr, "Failed to get certificate from SSL context\n");
            SSL_CTX_free(ctx);
            return NULL;
        }
        
        // 获取本机IP地址
        char local_ip[INET_ADDRSTRLEN];
        bool ip_obtained = getLocalIP(local_ip, sizeof(local_ip));
        
        // 如果无法获取本机IP，使用回退地址
        if (!ip_obtained) {
            const char *fallback_ip = "127.0.0.1";
            fprintf(stderr, "Failed to get local IP address, using fallback: %s\n", fallback_ip);
            strncpy(local_ip, fallback_ip, sizeof(local_ip) - 1);
            local_ip[sizeof(local_ip) - 1] = '\0';
        }
        
        // 构建正确格式的subjectAltName字符串
        char san_str[256];
        snprintf(san_str, sizeof(san_str), "IP:%s", local_ip);
        
        X509_EXTENSION *ext = X509V3_EXT_conf_nid(NULL, NULL,
                                              NID_subject_alt_name,
                                              san_str);
        if (!ext) {
            fprintf(stderr, "Failed to create X509 extension: %s\n", ERR_error_string(ERR_get_error(), NULL));
            SSL_CTX_free(ctx);
            return NULL;
        }
        
        if (!X509_add_ext(cert, ext, -1)) {
            fprintf(stderr, "Failed to add extension to certificate: %s\n", ERR_error_string(ERR_get_error(), NULL));
            X509_EXTENSION_free(ext);
            SSL_CTX_free(ctx);
            return NULL;
        }
        
        X509_EXTENSION_free(ext);
        
        // 启用客户端证书验证（可选）
        // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

        return ctx;
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception in create_ssl_context: %s\n", e.what());
        if (ctx) {
            SSL_CTX_free(ctx);
        }
        return NULL;
    } catch (...) {
        fprintf(stderr, "Unknown exception in create_ssl_context\n");
        if (ctx) {
            SSL_CTX_free(ctx);
        }
        return NULL;
    }
}

/********************** HTTP处理函数 ***********************/
void send_response(SSL *ssl, ResponseBody responsebody)
{
    if (strlen(responsebody.body) > MAX_RESPONSE_SIZE)
    {
        char header[512];
        int len = snprintf(header, sizeof(header),
                           "HTTP/1.1 %d %s\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: Content-Type, Authorization, X-Current-Page, X-Page-Size\r\n"
                           "Access-Control-Max-Age: 86400\r\n"
                           "Content-Type: %s; charset=utf-8\r\n"
                           "Content-Length: %zu\r\n\r\n",
                           STATUS_BAD_REQUEST,
                           "Response too large",
                           "text/plain",
                           strlen("{\"error\":\"Response too large, please use paging or split your request.\"}"));
        SSL_write(ssl, header, len);
        SSL_write(ssl, "{\"error\":\"Response too large, please use paging or split your request.\"}", strlen("{\"error\":\"Response too large, please use paging or split your request.\"}"));
        return;
    }
    char header[MAX_RESPONSE_SIZE];
    int len = snprintf(header, sizeof(header),
                       "HTTP/1.1 %d %s\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                       "Access-Control-Allow-Headers: Content-Type, Authorization, X-Current-Page, X-Page-Size\r\n"
                       "Access-Control-Max-Age: 86400\r\n"
                       "Content-Type: %s; charset=utf-8\r\n"
                       "X-Total-Count: %d\r\n"  // 总数据量
                       "X-Current-Page: %d\r\n" // 当前页码
                       "X-Page-Size: %d\r\n"    // 每页条数
                       "Content-Length: %zu\r\n\r\n",
                       responsebody.status,
                       responsebody.errmsg,
                       responsebody.content_type,
                       responsebody.total_count,
                       responsebody.current_page,
                       responsebody.page_size,
                       strlen(responsebody.body));
    int written = SSL_write(ssl, header, len);
    if (written <= 0)
    {
        ERR_print_errors_fp(stderr);
        return;
    }
    written = SSL_write(ssl, responsebody.body, strlen(responsebody.body));
    if (written <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
}

// 专门处理OPTIONS预检请求的函数
void handle_options_request(SSL *ssl, RequestBody /*requestbody*/)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    responsebody.status = 204; // No Content
    strcpy(responsebody.content_type, "text/plain");
    responsebody.body[0] = '\0'; // 空响应体
    send_response(ssl, responsebody);
}

void handle_default_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_default_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

// GET /api/newfile 接口处理函数
void handle_newfile_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_newfile_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

// POST /api/updatefile 接口处理函数
void handle_updatefile_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_updatefile_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_editfile_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_editfile_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_filedata_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_filedata_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

// 新增接口处理函数
void handle_opendata_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_opendata_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_filestatus_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_filestatus_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_filelist_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_filelist_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_deletefile_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_deletefile_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_addworksheet_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_addworksheet_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_removeworksheet_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_removeworksheet_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_renameworksheet_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_renameworksheet_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_sheetlist_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_sheetlist_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_sheetdata_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_sheetdata_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

void handle_renamefile_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_renamefile_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

// 接口路由
struct Route routes[] = {
    {"GET", "/api/default", handle_default_request},
    {"GET", "/api/filelist", handle_filelist_request},
    {"POST", "/api/newfile", handle_newfile_request},
    {"POST", "/api/updatefile", handle_updatefile_request},
    {"POST", "/api/editfile", handle_editfile_request},
    {"POST", "/api/filedata", handle_filedata_request},
    {"GET", "/api/opendata", handle_opendata_request},      // 新增GET接口
    {"POST", "/api/filestatus", handle_filestatus_request}, // 新增POST接口
    {"POST", "/api/deletefile", handle_deletefile_request},
    {"POST", "/api/addworksheet", handle_addworksheet_request},
    {"POST", "/api/removeworksheet", handle_removeworksheet_request},
    {"POST", "/api/renameworksheet", handle_renameworksheet_request},
    {"POST", "/api/sheetlist", handle_sheetlist_request},
    {"POST", "/api/sheetdata", handle_sheetdata_request},
    {"POST", "/api/renamefile", handle_renamefile_request},
    // 添加通用OPTIONS处理路由
    {"OPTIONS", "/api/default", handle_options_request},
    {"OPTIONS", "/api/filelist", handle_options_request},
    {"OPTIONS", "/api/newfile", handle_options_request},
    {"OPTIONS", "/api/updatefile", handle_options_request},
    {"OPTIONS", "/api/editfile", handle_options_request},
    {"OPTIONS", "/api/filedata", handle_options_request},
    {"OPTIONS", "/api/opendata", handle_options_request},
    {"OPTIONS", "/api/filestatus", handle_options_request},
    {"OPTIONS", "/api/deletefile", handle_options_request},
    {"OPTIONS", "/api/addworksheet", handle_options_request},
    {"OPTIONS", "/api/removeworksheet", handle_options_request},
    {"OPTIONS", "/api/renameworksheet", handle_options_request},
    {"OPTIONS", "/api/sheetlist", handle_options_request},
    {"OPTIONS", "/api/sheetdata", handle_options_request},
    {"OPTIONS", "/api/renamefile", handle_options_request},
    {NULL, NULL, NULL}};

void handle_request(SSL *ssl, int /*client_socket*/)
{
    char buffer[BUFFER_SIZE]; // 读取接收到的内容
    int total_read = 0;
    int bytes_read = 0;

    while (1)
    {
        bytes_read = SSL_read(ssl, buffer + total_read, sizeof(buffer) - total_read - 1);
        if (bytes_read <= 0)
        {
            int err = SSL_get_error(ssl, bytes_read);
            if (err == SSL_ERROR_ZERO_RETURN)
            {
                printf("Client closed connection gracefully\n");
            }
            else if (err == SSL_ERROR_SYSCALL)
            {
                if (errno == ECONNRESET)
                {
                    fprintf(stderr, "Connection reset by peer\n");
                }
                else if (errno == ETIMEDOUT)
                {
                    fprintf(stderr, "Read operation timed out\n");
                }
                else
                {
                    fprintf(stderr, "System error: %s (errno=%d)\n", strerror(errno), errno);
                }
            }
            else if (err == SSL_ERROR_SSL)
            {
                unsigned long ec = ERR_get_error();
                fprintf(stderr, "SSL Protocol Error: %s\n", ERR_reason_error_string(ec));
                fprintf(stderr, "Error stack:\n");
                while ((ec = ERR_get_error()) != 0)
                {
                    fprintf(stderr, "- %s\n", ERR_reason_error_string(ec));
                }
            }
            else
            {
                fprintf(stderr, "Unknown SSL error: %d\n", err);
            }
            break;
        }
        total_read += bytes_read;
        if (total_read >= static_cast<int>(sizeof(buffer) - 1) || strstr(buffer, "\r\n\r\n"))
            break;
    }

    // 判断请求体大小
    if (total_read > static_cast<int>(MAX_REQUEST_SIZE))
    {
        ResponseBody responsebody;
        ResponseBodyInit(&responsebody);
        responsebody.status = STATUS_BAD_REQUEST;
        sprintf(responsebody.errmsg, "%s", ErrorCodeManager::getErrorMessage(STATUS_BAD_REQUEST).c_str());
        sprintf(responsebody.content_type, "text/plain");
        sprintf(responsebody.body, "{\"error\":\"Request body too large, please use paging or split your request.\"}");
        send_response(ssl, responsebody);
        return;
    }
    if (total_read == 0)
    {
        if (SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)
        {
            printf("Clean SSL shutdown received\n");
        }
        else
        {
            fprintf(stderr, "Empty request from client (possible network issue)\n");
        }
        return;
    }
    buffer[total_read] = '\0';
    // 修复请求头日志输出
    printf("Received full request (%d bytes):\n%.*s\n", total_read, total_read, buffer);
    
    // 检查服务器是否正在关闭
    if (g_server_stopping)
    {
        logger_log_info("服务器正在关闭，中止请求处理");
        ResponseBody responsebody;
        ResponseBodyInit(&responsebody);
        responsebody.status = STATUS_SERVICE_UNAVAILABLE;
        sprintf(responsebody.errmsg, "%s", ErrorCodeManager::getErrorMessage(STATUS_SERVICE_UNAVAILABLE).c_str());
        sprintf(responsebody.content_type, "text/plain");
        sprintf(responsebody.body, "{\"error\":\"Service is shutting down\"}");
        send_response(ssl, responsebody);
        return;
    }

    // 解析请求行时添加错误检查
    RequestBody requestbody; // 取buffer中指定内容
    RequestBodyInit(&requestbody);
    if (sscanf(buffer, "%15s %255s %15s", requestbody.method, requestbody.path, requestbody.protocol) != 3)
    {
        ResponseBody responsebody;
        ResponseBodyInit(&responsebody);
        responsebody.status = STATUS_BAD_REQUEST;
        memset(responsebody.errmsg, 0, sizeof(responsebody.errmsg));
        sprintf(responsebody.errmsg, "%s", ErrorCodeManager::getErrorMessage(STATUS_BAD_REQUEST).c_str());
        memset(responsebody.content_type, 0, sizeof(responsebody.content_type));
        sprintf(responsebody.content_type, "text/plain");
        memset(responsebody.body, 0, sizeof(responsebody.body));
        sprintf(responsebody.body, "{\"error\":\"Invalid request\"}");
        send_response(ssl, responsebody);
        return;
    }

    // 修复后的头解析逻辑
    char *header_start = strstr(buffer, "\r\n") + 2; // 跳过请求行
    while (header_start && header_start < buffer + total_read)
    {
        char *line_end = strstr(header_start, "\r\n");
        if (!line_end)
            break;

        // 提取单行头
        char header_line[256];
        strncpy(header_line, header_start, line_end - header_start);
        header_line[line_end - header_start] = '\0';

        // 解析键值对
        char *colon = strchr(header_line, ':');
        if (colon)
        {
            *colon = '\0';
            char *value = colon + 1;
            while (*value == ' ')
                value++; // 跳过空格
            // 忽略大小写比较
            if (strcasecmp(header_line, "X-Current-Page") == 0)
            {
                requestbody.current_page = atoi(value);
            }
            else if (strcasecmp(header_line, "X-Page-Size") == 0)
            {
                requestbody.page_size = atoi(value);
            }
        }

        header_start = line_end + 2; // 移动到下一行头
        if (strncmp(header_start, "\r\n", 2) == 0)
            break; // 遇到空行停止
    }
    printf("currentPage:%d pageSize:%d\n", requestbody.current_page, requestbody.page_size);
    // 新增：正确解析请求正文
    requestbody.body_start = strstr(buffer, "\r\n\r\n");
    if (requestbody.body_start)
    {
        requestbody.body_start += 4; // 跳过空行
        int content_length = 0;

        // 获取Content-Length
        char *cl_header = strstr(buffer, "Content-Length:");
        if (cl_header)
        {
            content_length = atoi(cl_header + 15);
            // 验证实际正文长度
            if (strlen(requestbody.body_start) < static_cast<size_t>(content_length))
            {
                ResponseBody responsebody;
                ResponseBodyInit(&responsebody);
                responsebody.status = STATUS_BAD_REQUEST;
                sprintf(responsebody.errmsg, "%s", ErrorCodeManager::getErrorMessage(STATUS_BAD_REQUEST).c_str());
                memset(responsebody.content_type, 0, sizeof(responsebody.content_type));
                sprintf(responsebody.content_type, "text/plain");
                memset(responsebody.body, 0, sizeof(responsebody.body));
                sprintf(responsebody.body, "{\"error\":\"Incomplete body\"}");
                send_response(ssl, responsebody);
                return;
            }
        }
    }
    else
    {
        // 如果没有找到正文分隔符，将body_start设置为空字符串
        static const char empty_string[] = "";
        requestbody.body_start = const_cast<char *>(empty_string);
    }
    printf("method:%s,path:%s,protocol:%s,body_start:%s\n", requestbody.method, requestbody.path, requestbody.protocol, requestbody.body_start);
    // 查找匹配路由
    for (int i = 0; routes[i].method != NULL; i++)
    {
        if (strcmp(requestbody.method, routes[i].method) == 0 &&
            strcmp(requestbody.path, routes[i].path) == 0)
        {
            // 传递正确的请求正文
            routes[i].handler(ssl, requestbody);
            return;
        }
    }
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    responsebody.status = STATUS_BAD_REQUEST;
    sprintf(responsebody.errmsg, "%s", ErrorCodeManager::getErrorMessage(STATUS_BAD_REQUEST).c_str());
    memset(responsebody.content_type, 0, sizeof(responsebody.content_type));
    sprintf(responsebody.content_type, "text/plain");
    memset(responsebody.body, 0, sizeof(responsebody.body));
    sprintf(responsebody.body, "{\"error\":\"Endpoint not found\"}");
    send_response(ssl, responsebody);
}

/********************** 线程处理函数 **********************/
void *client_thread(void *arg)
{
    SSL *ssl = (SSL *)arg;
    int fd = SSL_get_fd(ssl);
    
    // 为客户端线程添加完整的异常处理机制
    try {
        struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        // 检查服务器是否正在关闭
        if (g_server_stopping)
        {
            logger_log_info("服务器正在关闭，客户端线程将退出");
            // 不处理请求，直接跳到清理部分
        }
        
        // Add connection metadata logging
        struct sockaddr_in peer_addr;
        socklen_t len = sizeof(peer_addr);
        getpeername(fd, (struct sockaddr *)&peer_addr, &len);
        
        // 在线程内部捕获所有可能的异常，防止异常逃逸导致程序崩溃
        try {
            handle_request(ssl, fd);
        } catch (const std::__1::system_error& e) {
            logger_log_error("System error in client thread: %s", e.what());
            // 可以选择发送错误响应给客户端
        } catch (const std::exception& e) {
            logger_log_error("Standard exception in client thread: %s", e.what());
            // 可以选择发送错误响应给客户端
        } catch (...) {
            logger_log_error("Unknown exception in client thread");
            // 可以选择发送错误响应给客户端
        }
        
        // 添加错误检查的关闭流程
        try {
            if (SSL_is_init_finished(ssl))
            {
                int shutdown_ret;
                do
                {
                    shutdown_ret = SSL_shutdown(ssl);
                    if (shutdown_ret < 0)
                    {
                        int err = SSL_get_error(ssl, shutdown_ret);
                        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                        {
                            continue; // Retry shutdown
                        }
                        break;
                    }
                } while (shutdown_ret != 1);
            }
        } catch (...) {
            // 忽略SSL关闭过程中的异常
            logger_log_warn("Exception during SSL shutdown");
        }
    } catch (...) {
        // 最外层异常捕获，确保线程不会因为任何未预期的异常而崩溃整个程序
        logger_log_error("Fatal exception in client thread initialization");
    }
    
    // 确保资源正确释放，即使发生异常
    try {
        // 从client_threads向量中移除当前线程ID
        std::lock_guard<std::mutex> lock(client_threads_mutex);
        auto it = std::find(client_threads.begin(), client_threads.end(), pthread_self());
        if (it != client_threads.end()) {
            client_threads.erase(it);
        }
        
        SSL_free(ssl);
        close(fd); // 确保套接字关闭
    } catch (...) {
        // 忽略资源释放过程中的异常
    }
    
    return NULL;
}

// 由于我们现在使用更安全的signal_handler实现，这个函数已不再使用
// 保留它是为了向后兼容，但实际信号处理现在由cleanup函数处理
void signal_handler(int /*sig*/)
{
    // 在新版本中，这个函数实际上不会被调用
    // 所有信号处理都在cleanup函数中完成
    g_running = 0;
}

// 资源清理函数 - 在main函数中正常调用
void cleanup_resources()
{
    // 添加日志记录，记录清理开始
    logger_log_info("开始清理资源");
    
    // 通知所有客户端线程服务器正在关闭
    g_server_stopping = 1;
    
    // 等待所有客户端线程完成
    std::vector<pthread_t> threads_to_join;
    {
        std::lock_guard<std::mutex> lock(client_threads_mutex);
        threads_to_join = client_threads;
        logger_log_info("等待 %zu 个客户端线程完成...", threads_to_join.size());
    }
    
    for (pthread_t thread : threads_to_join) {
        // 设置超时等待，避免无限阻塞
        const int join_timeout = 5; // 5秒
        
        // 平台兼容的线程超时等待实现
        #ifdef _WIN32
        // Windows平台：使用普通join但添加超时逻辑
        // 注意：在Windows上，pthread_join是阻塞的，这里我们无法真正实现超时等待
        // 但我们已经设置了g_server_stopping标志，线程应该会自行退出
        pthread_join(thread, NULL);
        logger_log_debug("客户端线程已完成: %lu", (unsigned long)thread);
        #else
        // POSIX平台：实现自定义超时机制，不依赖pthread_timedjoin_np
        // 使用单独的线程进行等待并通过条件变量实现超时
        struct ThreadJoinData {
            pthread_t thread_to_wait;  // 要等待的线程
            pthread_mutex_t mutex;     // 互斥锁
            pthread_cond_t cond;       // 条件变量
            bool thread_exited;        // 线程是否已退出
        } join_data;
        
        // 初始化数据结构
        pthread_mutex_init(&join_data.mutex, NULL);
        pthread_cond_init(&join_data.cond, NULL);
        join_data.thread_to_wait = thread;
        join_data.thread_exited = false;
        
        // 创建等待线程
        pthread_t waiter_thread;
        pthread_create(&waiter_thread, NULL, [](void* arg) {
            ThreadJoinData* data = static_cast<ThreadJoinData*>(arg);
            
            // 等待目标线程结束
            pthread_join(data->thread_to_wait, NULL);
            
            // 线程结束后通知主线程
            pthread_mutex_lock(&data->mutex);
            data->thread_exited = true;
            pthread_cond_signal(&data->cond);
            pthread_mutex_unlock(&data->mutex);
            
            return (void*)NULL;
        }, &join_data);
        
        // 设置超时时间
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += join_timeout;
        
        // 主线程等待，带超时机制
        pthread_mutex_lock(&join_data.mutex);
        
        // 如果线程还没退出，则等待条件变量或超时
        if (!join_data.thread_exited) {
            int wait_result = pthread_cond_timedwait(&join_data.cond, &join_data.mutex, &timeout);
            
            if (wait_result == ETIMEDOUT) {
                logger_log_warn("等待客户端线程完成超时");
            } else if (wait_result == 0 && join_data.thread_exited) {
                logger_log_debug("客户端线程已完成: %lu", (unsigned long)thread);
            }
        } else {
            logger_log_debug("客户端线程已完成: %lu", (unsigned long)thread);
        }
        
        pthread_mutex_unlock(&join_data.mutex);
        
        // 清理资源
        pthread_cond_destroy(&join_data.cond);
        pthread_mutex_destroy(&join_data.mutex);
        
        // 等待waiter_thread完成，避免资源泄漏
        pthread_join(waiter_thread, NULL);
        #endif
    }
    
    // 关闭服务器套接字
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
        logger_log_info("服务器套接字已关闭");
    }

    // 释放SSL上下文
    if (global_ctx)
    {
        SSL_CTX_free(global_ctx);
        global_ctx = NULL;
        logger_log_info("SSL上下文已释放");
    }

    // 添加更多资源清理
    // 首先释放 LibreOffice 连接，避免在停止其他组件时使用已释放的连接
    try {
        filemanager::LibreOfficeService::shutdown();
        logger_log_info("LibreOffice连接已关闭");
    }
    // 捕获system_error异常 (处理libc++中的实现)
    catch (const std::__1::system_error& e) {
        logger_log_error("System error during LibreOffice shutdown: %s", e.what());
        std::cerr << "System error during LibreOffice shutdown: " << e.what() << std::endl;
    }
    // 捕获标准库中的异常
    catch (const std::exception& e) {
        logger_log_error("Standard exception during LibreOffice shutdown: %s", e.what());
        std::cerr << "Standard exception during LibreOffice shutdown: " << e.what() << std::endl;
    }
    // 捕获所有其他异常
    catch (...) {
        logger_log_error("Unknown exception during LibreOffice shutdown");
        std::cerr << "Unknown exception during LibreOffice shutdown" << std::endl;
    }

    // 然后停止文件队列管理器的任务处理器
    filemanager::FileQueueManager::getInstance().stopTaskProcessor();
    logger_log_info("文件队列管理器已停止");

    // 停止模板监控线程，添加异常处理
    try {
        filemanager::TemplateIndexCacheManager::getInstance().stopMonitorThread();
        logger_log_info("模板监控线程已停止");
    }
    // 捕获system_error异常 (处理libc++中的实现)
    catch (const std::__1::system_error& e) {
        logger_log_error("System error during template monitor shutdown: %s", e.what());
        std::cerr << "System error during template monitor shutdown: " << e.what() << std::endl;
    }
    // 捕获标准库中的异常
    catch (const std::exception& e) {
        logger_log_error("Standard exception during template monitor shutdown: %s", e.what());
        std::cerr << "Standard exception during template monitor shutdown: " << e.what() << std::endl;
    }
    // 捕获所有其他异常
    catch (...) {
        logger_log_error("Unknown exception during template monitor shutdown");
        std::cerr << "Unknown exception during template monitor shutdown" << std::endl;
    }

    // 释放配置资源
    try {
        json_config_free();
        logger_log_info("配置资源已释放");
    }
    // 捕获system_error异常 (处理libc++中的实现)
    catch (const std::__1::system_error& e) {
        logger_log_error("System error during config free: %s", e.what());
        std::cerr << "System error during config free: " << e.what() << std::endl;
    }
    // 捕获标准库中的异常
    catch (const std::exception& e) {
        logger_log_error("Standard exception during config free: %s", e.what());
        std::cerr << "Standard exception during config free: " << e.what() << std::endl;
    }
    // 捕获所有其他异常
    catch (...) {
        logger_log_error("Unknown exception during config free");
        std::cerr << "Unknown exception during config free" << std::endl;
    }

    logger_log_info("资源清理完成，退出程序");
    
    // 清理全局单例资源
    cleanup_global_singletons();
    
    // 最后关闭日志系统，添加异常处理
    try {
        // 确保所有日志消息都已刷新到磁盘
        fflush(stdout);
        fflush(stderr);
        
        logger_close();
    }
    // 捕获system_error异常 (处理libc++中的实现)
    catch (const std::__1::system_error& e) {
        std::cerr << "System error during logger close: " << e.what() << std::endl;
    }
    // 捕获标准库中的异常
    catch (const std::exception& e) {
        std::cerr << "Standard exception during logger close: " << e.what() << std::endl;
    }
    // 捕获所有其他异常
    catch (...) {
        std::cerr << "Unknown exception during logger close" << std::endl;
    }
}

// 外部C风格接口的信号处理函数
extern "C" void cleanup(int sig)
{
    // 为了避免在信号处理程序中调用非信号安全的函数
    // 我们只设置退出标志，然后在主循环中处理资源清理
    static volatile sig_atomic_t handled = 0;
    
    // 避免重复处理信号，防止递归调用
    if (handled) {
        return;
    }
    handled = 1;
    
    // 立即记录信号，但不使用复杂的日志函数
    // 在信号处理程序中只能使用异步信号安全的函数
    const char* sig_name = "unknown";
    switch(sig) {
        case SIGINT: sig_name = "SIGINT"; break;
        case SIGTERM: sig_name = "SIGTERM"; break;
        case SIGSEGV: sig_name = "SIGSEGV"; break;
        case SIGABRT: sig_name = "SIGABRT"; break;
        default: break;
    }
    
    // 只使用异步信号安全的函数
#ifdef _WIN32
    _write(STDERR_FILENO, "Received signal: ", 17);
    _write(STDERR_FILENO, sig_name, strlen(sig_name));
    _write(STDERR_FILENO, ", preparing to exit\n", 20);
#else
    write(STDERR_FILENO, "Received signal: ", 17);
    write(STDERR_FILENO, sig_name, strlen(sig_name));
    write(STDERR_FILENO, ", preparing to exit\n", 20);
#endif
    
    // 设置退出标志
    g_running = 0;
}

int main()
{
    // 首先设置全局未捕获异常处理器，确保在任何可能抛出异常的操作之前完成设置
    std::set_terminate(uncaught_exception_handler);
    
    try {
        // 注册信号处理
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = cleanup;
        
        // 设置信号掩码，阻止在处理信号时收到其他信号
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGINT);
        sigaddset(&sa.sa_mask, SIGTERM);
        sigaddset(&sa.sa_mask, SIGSEGV);
        sigaddset(&sa.sa_mask, SIGABRT);
        
        // 注册信号处理器
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        
        // 对于致命信号，我们只记录信息并退出，不进行复杂的清理
        struct sigaction fatal_sa;
        memset(&fatal_sa, 0, sizeof(fatal_sa));
        fatal_sa.sa_handler = SIG_DFL; // 使用默认处理程序
        sigaction(SIGSEGV, &fatal_sa, NULL);
        sigaction(SIGABRT, &fatal_sa, NULL);
    } catch (const std::exception& e) {
        logger_log_error("Exception during signal handler setup: %s", e.what());
        std::cerr << "Exception during signal handler setup: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        logger_log_error("Unknown exception during signal handler setup");
        std::cerr << "Unknown exception during signal handler setup" << std::endl;
        return 1;
    }
    
    try {

    // 初始化日志
    logger_init("../log/server.log");

    // 初始化配置
    if (json_config_init(NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize configuration\n");
        return 1;
    }

    // 初始化 LibreOffice 连接
    std::cout << "Attempting to initialize LibreOffice connection..." << std::endl;
    try {
        if (!filemanager::LibreOfficeService::initialize())
        {
            // 记录详细的错误信息到日志
            logger_log_warn("========================================");
            logger_log_warn("WARNING: Failed to initialize LibreOffice connection!");
            logger_log_warn("This may be caused by:");
            logger_log_warn("1. LibreOffice is not running in headless mode");
            logger_log_warn("2. LibreOffice is not accepting connections on port 2002");
            logger_log_warn("3. URE_BOOTSTRAP environment variable is not set correctly");
            logger_log_warn("");
            logger_log_warn("To start LibreOffice in headless mode, run:");
            logger_log_warn("soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\" --nofirststartwizard");
            logger_log_warn("========================================");

            // 继续运行服务，但会有一些功能受限
            logger_log_warn("Service will continue to run, but LibreOffice-related features will not work until connection is established.");
        }
        else
        {
            logger_log_info("LibreOffice connection initialized successfully.");
        }
    }
    catch (const std::system_error& e) {
        logger_log_error("System error during LibreOffice initialization: %s", e.what());
        std::cerr << "System error during LibreOffice initialization: " << e.what() << std::endl;
        // 继续运行服务，但会有一些功能受限
        logger_log_warn("Service will continue to run, but LibreOffice-related features will not work until connection is established.");
    }
    catch (const std::exception& e) {
        logger_log_error("Standard exception during LibreOffice initialization: %s", e.what());
        std::cerr << "Standard exception during LibreOffice initialization: " << e.what() << std::endl;
        // 继续运行服务，但会有一些功能受限
        logger_log_warn("Service will continue to run, but LibreOffice-related features will not work until connection is established.");
    }
    catch (...) {
        logger_log_error("Unknown exception during LibreOffice initialization");
        std::cerr << "Unknown exception during LibreOffice initialization" << std::endl;
        // 继续运行服务，但会有一些功能受限
        logger_log_warn("Service will continue to run, but LibreOffice-related features will not work until connection is established.");
    }

    // 初始化文件队列管理器并启动任务处理器
    filemanager::FileQueueManager::getInstance().startTaskProcessor();
    logger_log_info("File queue manager started");

    // 初始化文件管理器模板缓存
    try {
        rtl::OUString filePath;
        rtl::OUString defaultFileName;
        rtl::OUString sheetName;
        filemanager::getDefaultData(filePath, defaultFileName, sheetName);
        filemanager::TemplateIndexCacheManager::getInstance().getTemplateIndex(filePath+defaultFileName, sheetName);
        filemanager::TemplateIndexCacheManager::getInstance().monitorTemplateFile(filemanager::convertOUStringToString(filePath+defaultFileName), filemanager::convertOUStringToString(sheetName));
        logger_log_info("模板缓存初始化成功");
    }
    catch (const css::uno::Exception &e) {
        std::string errorMsg = rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr();
        logger_log_error("UNO异常 during template cache initialization: %s", errorMsg.c_str());
    }
    catch (const std::exception &e) {
        logger_log_error("标准异常 during template cache initialization: %s", e.what());
    }
    catch (...) {
        logger_log_error("未知异常 during template cache initialization");
    }

    // 启动服务
    int port = json_config_get_int("port");
    if (port == 0)
    {
        logger_log_warn("警告: 无法从配置文件中读取端口配置，使用默认端口8443");
        port = 8443; // 默认端口
    }

    //设置日志级别
    switch(json_config_get_int("log_level"))
    {
        case 0:
            logger_set_level(LogLevel::LOG_LEVEL_DEBUG);
            break;
        case 1:
            logger_set_level(LogLevel::LOG_LEVEL_INFO);
            break;
        case 2:
            logger_set_level(LogLevel::LOG_LEVEL_WARN);
            break;
        case 3:
            logger_set_level(LogLevel::LOG_LEVEL_ERROR);
            break;
        default:
            logger_set_level(LogLevel::LOG_LEVEL_INFO);
            break;
    }

    // 创建SSL上下文
    global_ctx = create_ssl_context();
    if (!global_ctx) {
        logger_log_error("Failed to create SSL context");
        cleanup_resources();
        return 1;
    }

    // 创建TCP套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        logger_log_error("Socket creation failed");
        perror("socket creation failed");
        cleanup_resources();
        return 1;
    }

    // 设置套接字选项
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_len = sizeof(addr),
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {INADDR_ANY},
        .sin_zero = {0}};

    // 绑定地址
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        logger_log_error("Bind failed");
        perror("bind failed");
        cleanup_resources();
        return 1;
    }

    // 监听连接
    if (listen(server_fd, 10) < 0)
    {
        logger_log_error("Listen failed");
        perror("listen failed");
        cleanup_resources();
        return 1;
    }

    logger_log_info("Server listening on port %d", port);

    while (g_running)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        // 设置非阻塞模式检查是否需要退出
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
        
        int select_result = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result < 0)
        {
            // 发生错误，但不是因为信号中断
            if (errno != EINTR)
            {
                perror("select failed");
                break;
            }
            continue;
        }
        else if (select_result == 0)
        {
            // 超时，检查是否需要退出
            continue;
        }

        // 有新连接
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        SSL *ssl = SSL_new(global_ctx);
        if (!ssl)
        {
            fprintf(stderr, "SSL_new failed\n");
            close(client_fd);
            continue;
        }

        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            // 添加清理代码
            SSL_free(ssl);
            close(client_fd); // 关闭无效连接
        }
        else
        {
            pthread_t tid;
            if (pthread_create(&tid, NULL, client_thread, ssl) != 0)
            {
                logger_log_error("Failed to create client thread");
                perror("pthread_create failed");
                SSL_free(ssl);
                close(client_fd);
            }
            else
            {
                // 将线程ID添加到全局列表中，以便在服务器关闭时跟踪
                std::lock_guard<std::mutex> lock(client_threads_mutex);
                client_threads.push_back(tid);
                logger_log_debug("Created client thread with ID: %lu", (unsigned long)tid);
            }
        }
    }

    // 正常退出时的清理
    cleanup_resources();
    return 0;
} catch (const std::__1::system_error& e) {
    // 捕获libc++系统错误异常
    logger_log_error("Caught std::__1::system_error during program execution: %s", e.what());
    logger_log_error("Error code: %d, Category: %s", e.code().value(), e.code().category().name());
    // 清理全局单例资源
    cleanup_global_singletons();
    return 1;
} catch (const std::exception& e) {
    // 捕获其他标准异常
    logger_log_error("Caught std::exception during program execution: %s", e.what());
    // 清理全局单例资源
    cleanup_global_singletons();
    return 1;
} catch (...) {
    // 捕获未知异常
    logger_log_error("Caught unknown exception during program execution");
    // 清理全局单例资源
    cleanup_global_singletons();
    return 1;
}
}