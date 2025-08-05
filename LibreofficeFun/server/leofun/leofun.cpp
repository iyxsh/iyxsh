#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include "./apihandle/handle.h"
#include "./filemanager/filemanager.h"
#include "./logger/logger.h"
#include "./config/json_config.h"

/* ======= dbmanager.c  ======= */
//extern GridCircle *gridcircldata; // json文件默认配置读取
extern char *defaultdbname;

// HTTPS配置
const char *CERT_FILE = "server.crt";
const char *KEY_FILE = "server.key";
const char *CA_FILE = "ca.crt";

// 新增全局变量用于资源管理
static int server_fd = -1;
static SSL_CTX *global_ctx = NULL;

// 定义路由处理函数类型
typedef void (*route_handler)(SSL *ssl, RequestBody RequestBody);

// 路由表结构体
struct Route
{
    const char *method;
    const char *path;
    route_handler handler;
};

/********************** HTTPS初始化 ***********************/
SSL_CTX *create_ssl_context()
{
    SSL_CTX *ctx;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(TLS_server_method());

    // 添加证书验证
    if (!SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
    {
        fprintf(stderr, "证书加载失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
        exit(EXIT_FAILURE);
    }
    if (!SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
    {
        fprintf(stderr, "私钥加载失败: %s\n", ERR_error_string(ERR_get_error(), NULL));
        exit(EXIT_FAILURE);
    }
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "证书与私钥不匹配\n");
        exit(EXIT_FAILURE);
    }
    // 加载CA证书链（必须包含Nginx使用的根证书）(验证客户端)
    if (!SSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
    {
        fprintf(stderr, "无法加载CA证书链: %s\n", ERR_error_string(ERR_get_error(), NULL));
        exit(EXIT_FAILURE);
    }

    // Configure certificate chain verification depth
    SSL_CTX_set_verify_depth(ctx, 4); // 适应多级证书

    // 设置加密套件（增加TLSv1.3支持）
    const char *ciphers = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:ECDHE-ECDSA-AES256-GCM-SHA384";
    SSL_CTX_set_ciphersuites(ctx, ciphers);
    // 新增：初始化会话ID上下文（修复SSL_ERROR_SESSION_ID_CONTEXT）
    const char session_id_ctx[] = "DINGDIAN_CTX";
    if (!SSL_CTX_set_session_id_context(ctx, (const unsigned char *)session_id_ctx, sizeof(session_id_ctx) - 1))
    {
        fprintf(stderr, "Failed to set session ID context\n");
        exit(EXIT_FAILURE);
    }

    // 添加X.509扩展（必须包含服务器IP）
    X509 *cert = SSL_CTX_get0_certificate(ctx);
    X509_NAME *subject = X509_get_subject_name(cert);

    // 添加Subject Alternative Name扩展
    ASN1_IA5STRING *ia5 = ASN1_IA5STRING_new();
    ASN1_STRING_set(ia5, "192.168.146.128", strlen("192.168.146.128"));

    X509_EXTENSION *ext = X509V3_EXT_conf_nid(NULL, NULL,
                                              NID_subject_alt_name,
                                              (char *)ia5);
    X509_add_ext(cert, ext, -1);

    ASN1_IA5STRING_free(ia5);
    X509_EXTENSION_free(ext);
    // 启用客户端证书验证（可选）
    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    return ctx;
}

/********************** HTTP处理函数 ***********************/
void send_response(SSL *ssl, ResponseBody responsebody)
{
    char header[512];
    int len = snprintf(header, sizeof(header),
                       "HTTP/1.1 %d %s\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                       "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                       "Access-Control-Max-Age: 86400\r\n"
                       "Content-Type: %s; charset=utf-8\r\n"
                       "X-Total-Count: %d\r\n"  // 总数据量
                       "X-Current-Page: %d\r\n" // 当前页码
                       "X-Page-Size: %d\r\n"    // 每页条数
                       "Content-Length: %zu\r\n\r\n",
                       responsebody.status,
                       responsebody.status == 200 ? "OK" : "Not Found",
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

// GET /api/get-gridcircle-default 接口处理函数
void handle_style_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_style_request(requestbody, &responsebody);
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

void handle_filelist_request(SSL *ssl, RequestBody requestbody)
{
    ResponseBody responsebody;
    ResponseBodyInit(&responsebody);
    deal_filelist_request(requestbody, &responsebody);
    send_response(ssl, responsebody);
}

// 接口路由
struct Route routes[] = {
    {"GET", "/api/filelist", handle_filelist_request},
    {"POST", "/api/get-gridcircle-default", handle_style_request},
    {"POST", "/api/newfile", handle_newfile_request},
    {"POST", "/api/updatefile", handle_updatefile_request},
    {"POST", "/api/editfile", handle_editfile_request},
    {"POST", "/api/filedata", handle_filedata_request},
    {NULL, NULL, NULL}};

void handle_request(SSL *ssl, int client_socket)
{
    char buffer[BUFFER_SIZE];//读取接收到的内容
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
        if (total_read >= sizeof(buffer) - 1 || strstr(buffer, "\r\n\r\n"))
            break;
    }

    // Add connection state check
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

    // 解析请求行时添加错误检查
    RequestBody requestbody;//取buffer中指定内容
    RequestBodyInit(&requestbody);
    if (sscanf(buffer, "%15s %255s %15s", requestbody.method, requestbody.path, requestbody.protocol) != 3)
    {
        ResponseBody responsebody;
        ResponseBodyInit(&responsebody);
        responsebody.status = STATUS_BAD_REQUEST;
        memset(responsebody.content_type, 0, sizeof(responsebody.content_type));
        sprintf(responsebody.content_type, "text/plain");
        memset(responsebody.body, 0, sizeof(responsebody.body));
        sprintf(responsebody.body, "{\"error\":\"Invalid request\"}");
        send_response(ssl, responsebody);
        return;
    }

    // 修复后的头解析逻辑
    char *header_start = strstr(buffer, "\r\n") + 2; // 跳过请求行
    while (header_start && header_start < buffer + total_read) {
        char *line_end = strstr(header_start, "\r\n");
        if (!line_end) break;
        
        // 提取单行头
        char header_line[256];
        strncpy(header_line, header_start, line_end - header_start);
        header_line[line_end - header_start] = '\0';

        // 解析键值对
        char *colon = strchr(header_line, ':');
        if (colon) {
            *colon = '\0';
            char *value = colon + 1;
            while (*value == ' ') value++; // 跳过空格
            //忽略大小写比较
            if (strcasecmp(header_line, "X-Current-Page") == 0) {
                requestbody.current_page = atoi(value);
            } else if (strcasecmp(header_line, "X-Page-Size") == 0) {
                requestbody.page_size = atoi(value);
            }
        }

        header_start = line_end + 2; // 移动到下一行头
        if (strncmp(header_start, "\r\n", 2) == 0) break; // 遇到空行停止
    }
printf("currentPage:%d pageSize:%d\n",requestbody.current_page,requestbody.page_size);
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
            if (strlen(requestbody.body_start) < content_length)
            {
                ResponseBody responsebody;
                ResponseBodyInit(&responsebody);
                responsebody.status = STATUS_BAD_REQUEST;
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
        //body_start ? body_start : "";
        memcpy(requestbody.body_start,"",1);
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
    struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    // Add connection metadata logging
    struct sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);
    getpeername(fd, (struct sockaddr *)&peer_addr, &len);
    // printf("New connection from %s:%d\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
    handle_request(ssl, fd);
    // 添加错误检查的关闭流程
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
    SSL_free(ssl);
    close(fd); // 确保套接字关闭
    return NULL;
}

// 新增信号处理函数
void cleanup_resources(int sig)
{
    // 关闭服务器套接字
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
    }

    // 释放SSL上下文
    if (global_ctx)
    {
        SSL_CTX_free(global_ctx);
        global_ctx = NULL;
    }

    // 关闭日志系统
    logger_close();

    // 释放配置资源
    json_config_free();

    exit(EXIT_FAILURE);
}

int main()
{

    // 注册信号处理
    signal(SIGINT, cleanup_resources);
    signal(SIGTERM, cleanup_resources);
    signal(SIGSEGV, cleanup_resources);

    json_config_init("config.json");

    const char *logdata = json_config_get_string("logdata");
    // 初始化日志模块
    logger_init(logdata);
    logger_log("Server started");
    // 默认设置 单独一个文件存放
    //filedefault();

    int port = json_config_get_int("port");

    // 创建SSL上下文
    global_ctx = create_ssl_context();

    // 创建TCP套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {INADDR_ANY}};

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("HTTPS服务器运行在端口 %d...\n", port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        SSL *ssl = SSL_new(global_ctx);
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
            pthread_create(&tid, NULL, client_thread, ssl);
            pthread_detach(tid);
        }
    }

    // 正常退出时的清理
    cleanup_resources(0);
    //freepointer();
    return 0;
}