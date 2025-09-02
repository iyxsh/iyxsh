#!/bin/bash

# 生产环境部署脚本
# 使用方法: ./deploy.sh [install|build|configure|start|stop|restart|status|clean]

# 配置项
PROJECT_NAME="libreofficefun"

# 检测操作系统类型
OS_TYPE="$(uname -s)"

# 根据操作系统设置不同的路径和配置
if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
    # Windows环境（Git Bash或Cygwin）
    PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd -W)"
    # 将路径转换为Windows格式
    PROJECT_DIR="${PROJECT_DIR////\\}"
    BUILD_DIR="$PROJECT_DIR\\build"
    BIN_DIR="$PROJECT_DIR\\bin"
    LOG_DIR="$PROJECT_DIR\\log"
    CONFIG_FILE="$BIN_DIR\\config.json"
    SERVICE_NAME="$PROJECT_NAME"
    SYSTEMD_SERVICE_FILE=""
    
    # LibreOffice 相关配置（Windows）
    LIBREOFFICE_PATH="C:\\Program Files\\LibreOffice"
    URE_BOOTSTRAP="file:///$LIBREOFFICE_PATH/program/fundamentalrc"
    
    # 设置sed命令为gnu-sed（如果可用）
    if command -v gsed &> /dev/null; then
        SED="gsed"
    else
        SED="sed"
    fi
else
    # Unix/Linux环境
    PROJECT_DIR="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
    BUILD_DIR="$PROJECT_DIR/build"
    BIN_DIR="$PROJECT_DIR/bin"
    LOG_DIR="$PROJECT_DIR/log"
    CONFIG_FILE="$BIN_DIR/config.json"
    SERVICE_NAME="$PROJECT_NAME"
    SYSTEMD_SERVICE_FILE="/etc/systemd/system/$PROJECT_NAME.service"
    
    # LibreOffice 相关配置（Unix/Linux）
    LIBREOFFICE_PATH="/usr/local/lib/libreoffice"
    URE_BOOTSTRAP="file://$LIBREOFFICE_PATH/program/fundamentalrc"
    SED="sed"
fi

# 颜色定义
green="\033[0;32m"
red="\033[0;31m"
yellow="\033[0;33m"
blue="\033[0;34m"
reset="\033[0m"

# 日志函数
log() {
    local timestamp="$(date '+%Y-%m-%d %H:%M:%S')"
    local level="$1"
    local message="$2"
    local color=""
    
    case $level in
        "INFO") color=$blue; ;;
        "SUCCESS") color=$green; ;;
        "WARNING") color=$yellow; ;;
        "ERROR") color=$red; ;;
        *) color=$reset; ;;
    esac
    
    echo -e "${color}[${timestamp}] [${level}] ${message}${reset}"
    echo "[${timestamp}] [${level}] ${message}" >> "$LOG_DIR/deploy.log"
}

# 检查命令是否存在
exists() {
    command -v "$1" >/dev/null 2>&1
}

# 创建必要的目录
create_directories() {
    log "INFO" "创建必要的目录..."
    mkdir -p "$BUILD_DIR" "$LOG_DIR" "$PROJECT_DIR/data" "$PROJECT_DIR/data/work" "$PROJECT_DIR/data/default"
    chmod -R 755 "$LOG_DIR"
    chmod -R 755 "$PROJECT_DIR/data"
}

# 安装依赖
install_dependencies() {
    log "INFO" "检查并安装系统依赖..."
    
    # 检查操作系统
    if [ -f /etc/debian_version ]; then
        # Debian/Ubuntu
        apt-get update
        apt-get install -y cmake build-essential g++ openssl libssl-dev dos2unix
        apt-get install -y libreoffice libreoffice-dev
    elif [ -f /etc/redhat-release ]; then
        # CentOS/RHEL
        yum install -y cmake gcc-c++ openssl-devel dos2unix
        yum install -y libreoffice libreoffice-devel
    elif [ -f /etc/SuSE-release ] || [ -f /etc/os-release ] && grep -q "SUSE" /etc/os-release; then
        # SUSE
        zypper install -y cmake gcc-c++ libopenssl-devel dos2unix
        zypper install -y libreoffice libreoffice-devel
    elif [ $(uname) = "FreeBSD" ]; then
        # FreeBSD
        pkg install -y cmake gcc openssl dos2unix
        pkg install -y libreoffice
    else
        log "ERROR" "不支持的操作系统，无法自动安装依赖。"
        exit 1
    fi
    
    # 检查依赖是否安装成功
    if ! exists cmake || ! exists g++ || ! exists openssl || ! exists soffice; then
        log "ERROR" "依赖安装失败，请手动安装缺少的依赖。"
        exit 1
    fi
    
    log "SUCCESS" "系统依赖安装成功。"
}

# 清理文件格式
sanitize_files() {
    log "INFO" "清理文件格式（转换为Unix格式）..."
    find "$PROJECT_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "*.txt" -o -name "*.cpp" -o -name "*.sh" -o -name "*.json" -o -name "*.md" \) \
        -exec dos2unix --keep-bom {} \;
    log "SUCCESS" "文件格式清理完成。"
}

# 构建项目
build_project() {
    # 检查CMakeLists.txt是否存在以及bin目录中的可执行文件是否不存在
    if [ -f "$PROJECT_DIR/CMakeLists.txt" ] && [ ! -f "$BIN_DIR/$PROJECT_NAME" ]; then
        log "INFO" "开始构建项目..."
        
        # 确保构建目录存在
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        # 清理旧的构建内容，确保全新构建
        log "INFO" "清理旧的构建内容..."
        if [ "$(ls -A .)" ]; then
            # 如果目录不为空，删除所有内容
            rm -rf * 2>/dev/null
            if [ $? -ne 0 ]; then
                log "WARNING" "无法完全清理构建目录，但将继续构建。"
            else
                log "INFO" "构建目录清理完成。"
            fi
        fi
        
        # 使用CMake构建项目
        log "INFO" "运行CMake配置..."
        cmake ..
        if [ $? -ne 0 ]; then
            log "ERROR" "CMake配置失败。"
            exit 1
        fi
        
        log "INFO" "编译项目..."
        cmake --build . -j $(nproc 2>/dev/null || echo 2)
        if [ $? -ne 0 ]; then
            log "ERROR" "项目编译失败。"
            exit 1
        fi
        
        # 复制可执行文件到bin目录
        cp -f "$BUILD_DIR/$PROJECT_NAME" "$BIN_DIR/"
        chmod +x "$BIN_DIR/$PROJECT_NAME"
        
        log "SUCCESS" "项目构建成功。"
    else
        if [ ! -f "$PROJECT_DIR/CMakeLists.txt" ]; then
            log "WARNING" "未找到CMakeLists.txt文件，跳过构建过程。"
        else
            log "INFO" "可执行文件 '$BIN_DIR/$PROJECT_NAME' 已存在，跳过构建过程。"
        fi
    fi
}

# 生成SSL证书
generate_ssl_certificates() {
    if [ ! -f "$BIN_DIR/server.crt" ] || [ ! -f "$BIN_DIR/server.key" ]; then
        log "INFO" "生成SSL证书..."
        cd "$BIN_DIR"
        
        # 生成服务器证书和私钥
        openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key -out server.crt -days 3650 \
            -subj '/CN=localhost' -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
        
        # 生成CA证书
        openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 3650 \
            -subj '/CN=CA'
        
        chmod 600 server.key ca.key
        log "SUCCESS" "SSL证书生成成功。"
    else
        log "INFO" "SSL证书已存在，跳过生成步骤。"
    fi
}

# 配置系统服务
configure_system_service() {
    # 检查是否在Windows环境或SYSTEMD_SERVICE_FILE为空
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* || -z "$SYSTEMD_SERVICE_FILE" ]]; then
        log "INFO" "在Windows环境下，不支持systemd系统服务配置。"
        log "INFO" "请手动创建Windows服务或使用启动脚本运行程序。"
        return 0
    fi
    
    # 检查systemctl命令是否存在
    if ! exists systemctl; then
        log "WARNING" "未找到systemctl命令，无法配置系统服务。"
        log "INFO" "请手动启动服务: $BIN_DIR/start.sh"
        return 0
    fi
    
    log "INFO" "配置系统服务..."
    
    # 检查是否有写入/etc/systemd/system目录的权限
    if [ ! -w "$(dirname "$SYSTEMD_SERVICE_FILE")" ]; then
        log "ERROR" "没有权限写入系统服务目录，请使用sudo或root权限运行脚本。"
        return 1
    fi
    
    # 创建systemd服务文件
    cat > "$SYSTEMD_SERVICE_FILE" << EOF
[Unit]
Description=LibreofficeFun Server
After=network.target

[Service]
Type=forking
User=root
WorkingDirectory=$BIN_DIR
Environment=URE_BOOTSTRAP=$URE_BOOTSTRAP
ExecStart=$BIN_DIR/start.sh
ExecStop=$BIN_DIR/stop.sh
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
EOF
    
    if [ $? -ne 0 ]; then
        log "ERROR" "创建systemd服务文件失败。"
        return 1
    fi
    
    # 重新加载systemd配置
    systemctl daemon-reload
    if [ $? -ne 0 ]; then
        log "ERROR" "重新加载systemd配置失败。"
        return 1
    fi
    
    # 启用服务（开机自启）
    systemctl enable "$SERVICE_NAME"
    if [ $? -ne 0 ]; then
        log "ERROR" "启用系统服务失败。"
        return 1
    fi
    
    log "SUCCESS" "系统服务配置成功。"
}

# 配置生产环境参数
configure_production() {
    log "INFO" "配置生产环境参数..."
    
    # 配置文件路径 - 在Windows环境下确保使用正确的路径格式
    local CONFIG_FILE_PATH
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下使用cygpath转换为Windows路径
        CONFIG_FILE_PATH="$(cygpath -w "$CONFIG_FILE")"
    else
        # Unix/Linux环境
        CONFIG_FILE_PATH="$CONFIG_FILE"
    fi
    
    # 备份原始配置文件
    if [ ! -f "$CONFIG_FILE_PATH.bak" ]; then
        log "INFO" "备份原始配置文件到 $CONFIG_FILE_PATH.bak"
        cp "$CONFIG_FILE_PATH" "$CONFIG_FILE_PATH.bak"
        if [ $? -ne 0 ]; then
            log "WARNING" "无法备份配置文件，但将继续执行。"
        fi
    fi
    
    # 统一使用临时文件方法来修改配置，避免不同版本sed的兼容性问题
    local TEMP_FILE
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下的临时文件路径
        TEMP_FILE="$(cygpath -w "$CONFIG_FILE.tmp")"
        log "INFO" "在Windows环境下更新配置参数..."
    else
        # Unix/Linux环境下的临时文件路径
        TEMP_FILE="$CONFIG_FILE.tmp"
        log "INFO" "在Unix/Linux环境下更新配置参数..."
    fi
    
    # 修改日志级别参数
    $SED 's/"log_level": 0/"log_level": 1/g' "$CONFIG_FILE_PATH" > "$TEMP_FILE"
    if [ $? -ne 0 ]; then
        log "ERROR" "修改日志级别参数失败。"
        rm -f "$TEMP_FILE" 2>/dev/null
        return 1
    fi
    mv "$TEMP_FILE" "$CONFIG_FILE_PATH"
    
    # 修改最大打开文档数参数
    $SED 's/"maxOpenDocuments": 10/"maxOpenDocuments": 20/g' "$CONFIG_FILE_PATH" > "$TEMP_FILE"
    if [ $? -ne 0 ]; then
        log "ERROR" "修改最大打开文档数参数失败。"
        rm -f "$TEMP_FILE" 2>/dev/null
        return 1
    fi
    mv "$TEMP_FILE" "$CONFIG_FILE_PATH"
    
    log "SUCCESS" "生产环境参数配置完成。"
}

# 启动服务
start_service() {
    # 检查是否在Windows环境或SYSTEMD_SERVICE_FILE为空
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* || -z "$SYSTEMD_SERVICE_FILE" ]]; then
        log "INFO" "在Windows环境下，直接启动服务..."
        cd "$BIN_DIR"
        bash ../bin/start.sh
    else
        # 检查systemctl命令是否存在
        if exists systemctl && [ -f "$SYSTEMD_SERVICE_FILE" ]; then
            log "INFO" "使用systemd启动服务..."
            systemctl start "$SERVICE_NAME"
            sleep 2
            systemctl status "$SERVICE_NAME" --no-pager
        else
            log "INFO" "直接启动服务..."
            cd "$BIN_DIR"
            bash ../bin/start.sh
        fi
    fi
    
    log "INFO" "检查服务是否启动成功..."
    # 根据操作系统使用不同的进程检查命令
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下使用tasklist检查进程
        if tasklist | grep -i "$PROJECT_NAME" > /dev/null; then
            log "SUCCESS" "服务启动成功。"
        else
            log "ERROR" "服务启动失败，请检查日志。"
        fi
    else
        # Unix/Linux环境下使用pgrep检查进程
        if pgrep -f "$PROJECT_NAME" > /dev/null; then
            log "SUCCESS" "服务启动成功。"
        else
            log "ERROR" "服务启动失败，请检查日志。"
        fi
    fi
}

# 停止服务
stop_service() {
    # 检查是否在Windows环境或SYSTEMD_SERVICE_FILE为空
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* || -z "$SYSTEMD_SERVICE_FILE" ]]; then
        log "INFO" "在Windows环境下，直接停止服务..."
        cd "$BIN_DIR"
        bash ../bin/stop.sh
    else
        # 检查systemctl命令是否存在
        if exists systemctl && [ -f "$SYSTEMD_SERVICE_FILE" ]; then
            log "INFO" "使用systemd停止服务..."
            systemctl stop "$SERVICE_NAME"
        else
            log "INFO" "直接停止服务..."
            cd "$BIN_DIR"
            bash ../bin/stop.sh
        fi
    fi
    
    # 等待服务停止
    sleep 2
    
    log "INFO" "检查服务是否已停止..."
    # 根据操作系统使用不同的进程检查命令
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下使用tasklist检查进程
        if tasklist | grep -i "$PROJECT_NAME" > /dev/null; then
            log "WARNING" "服务进程仍然存在，可能需要强制终止。"
            # Windows环境下的强制终止命令
            taskkill /F /IM "$PROJECT_NAME.exe" 2>/dev/null || taskkill /F /FI "IMAGENAME eq $PROJECT_NAME*" 2>/dev/null
        else
            log "SUCCESS" "服务已成功停止。"
        fi
    else
        # Unix/Linux环境下使用pgrep检查进程
        if pgrep -f "$PROJECT_NAME" > /dev/null; then
            log "WARNING" "服务未能完全停止，正在尝试强制终止..."
            pkill -9 -f "$PROJECT_NAME" 2>/dev/null
        else
            log "SUCCESS" "服务已成功停止。"
        fi
    fi
}

# 重启服务
restart_service() {
    log "INFO" "重启服务..."
    stop_service
    sleep 3
    start_service
}

# 检查服务状态
status_service() {
    # 检查是否在Windows环境或SYSTEMD_SERVICE_FILE为空
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* || -z "$SYSTEMD_SERVICE_FILE" ]]; then
        log "INFO" "在Windows环境下检查服务状态..."
        # Windows环境下使用tasklist检查进程
        if tasklist | grep -i "$PROJECT_NAME" > /dev/null; then
            log "SUCCESS" "服务正在运行。"
            tasklist | grep -i "$PROJECT_NAME"
        else
            log "WARNING" "服务未运行。"
        fi
    else
        # 检查systemctl命令是否存在
        if exists systemctl && [ -f "$SYSTEMD_SERVICE_FILE" ]; then
            log "INFO" "使用systemd检查服务状态..."
            systemctl status "$SERVICE_NAME" --no-pager
        else
            # Unix/Linux环境下使用pgrep检查进程
            if pgrep -f "$PROJECT_NAME" > /dev/null; then
                log "SUCCESS" "服务正在运行。"
                ps aux | grep "$PROJECT_NAME" | grep -v grep
            else
                log "WARNING" "服务未运行。"
            fi
        fi
    fi
}

# 清理构建文件
clean_build() {
    log "INFO" "清理构建文件..."
    # 根据操作系统使用不同的清理命令
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下的清理命令
        if [ -d "$BUILD_DIR" ]; then
            cd "$BUILD_DIR"
            # 使用rm -rf ./* 而不是rm -rf "$BUILD_DIR/*"，因为Windows路径可能有问题
            rm -rf ./*
        fi
    else
        # Unix/Linux环境下的清理命令
        rm -rf "$BUILD_DIR/*"
    fi
    log "SUCCESS" "构建文件清理完成。"
}

# 显示帮助
show_help() {
    echo -e "\n${green}LibreofficeFun 部署脚本${reset}\n"
    echo "用法: ./deploy.sh [命令]"
    echo ""
    echo "命令:"
    echo "  install      - 安装所有依赖"
    echo "  build        - 构建项目"
    echo "  configure    - 配置生产环境和系统服务"
    echo "  start        - 启动服务"
    echo "  stop         - 停止服务"
    echo "  restart      - 重启服务"
    echo "  status       - 检查服务状态"
    echo "  clean        - 清理构建文件"
    echo "  all          - 执行完整部署流程（install + build + configure + start）"
    echo ""
    echo "示例: ./deploy.sh all"
}

# 主函数
main() {
    # 创建日志目录
    mkdir -p "$LOG_DIR"
    
    case "$1" in
        "install")
            create_directories
            sanitize_files
            install_dependencies
            generate_ssl_certificates
            ;;
        "build")
            build_project
            ;;
        "configure")
            configure_production
            configure_system_service
            ;;
        "start")
            start_service
            ;;
        "stop")
            stop_service
            ;;
        "restart")
            restart_service
            ;;
        "status")
            status_service
            ;;
        "clean")
            clean_build
            ;;
        "all")
            create_directories
            sanitize_files
            install_dependencies
            generate_ssl_certificates
            build_project
            configure_production
            configure_system_service
            start_service
            ;;
        *)
            show_help
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"