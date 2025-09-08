#!/bin/bash

# 生产环境部署脚本
# 使用方法: ./deploy.sh [install|build|configure|start|stop|restart|status|clean]

# 设置严格模式，遇到错误立即退出
set -euo pipefail

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
    # 尝试检测LibreOffice安装路径
    if [ -d "C:\\Program Files\\LibreOffice" ]; then
        LIBREOFFICE_PATH="C:\\Program Files\\LibreOffice"
    elif [ -d "C:\\Program Files (x86)\\LibreOffice" ]; then
        LIBREOFFICE_PATH="C:\\Program Files (x86)\\LibreOffice"
    else
        LIBREOFFICE_PATH="C:\\Program Files\\LibreOffice" # 默认路径
        log "WARNING" "未检测到LibreOffice安装路径，使用默认路径"
    fi
    # 将Windows路径转换为file:/// URL格式 (使用正斜杠)
    local LO_URL_PATH
    LO_URL_PATH=${LIBREOFFICE_PATH//\\/\/}
    LO_URL_PATH=${LO_URL_PATH//\:/\%3A}
    URE_BOOTSTRAP="file:///$LO_URL_PATH/program/fundamentalrc"
    
    # 设置sed命令为gnu-sed（如果可用）
    if command -v gsed &> /dev/null; then
        SED="gsed"
    else
        SED="sed"
    fi
elif [[ "$OS_TYPE" == "FreeBSD" ]]; then
    # FreeBSD环境
    PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
    BUILD_DIR="$PROJECT_DIR/build"
    BIN_DIR="$PROJECT_DIR/bin"
    LOG_DIR="$PROJECT_DIR/log"
    CONFIG_FILE="$BIN_DIR/config.json"
    SERVICE_NAME="$PROJECT_NAME"
    SYSTEMD_SERVICE_FILE=""  # FreeBSD不使用systemd
    
    # LibreOffice 相关配置（FreeBSD）
    # 尝试检测LibreOffice安装路径
    if [ -d "/usr/local/lib/libreoffice" ]; then
        LIBREOFFICE_PATH="/usr/local/lib/libreoffice"
    else
        LIBREOFFICE_PATH="/usr/local/lib/libreoffice" # 默认路径
        log "WARNING" "未检测到LibreOffice安装路径，使用默认路径"
    fi
    URE_BOOTSTRAP="file://$LIBREOFFICE_PATH/program/fundamentalrc"
    SED="sed"
else
    # Linux环境
    PROJECT_DIR="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
    BUILD_DIR="$PROJECT_DIR/build"
    BIN_DIR="$PROJECT_DIR/bin"
    LOG_DIR="$PROJECT_DIR/log"
    CONFIG_FILE="$BIN_DIR/config.json"
    SERVICE_NAME="$PROJECT_NAME"
    SYSTEMD_SERVICE_FILE="/etc/systemd/system/$PROJECT_NAME.service"
    
    # LibreOffice 相关配置（Linux）
    # 尝试检测LibreOffice安装路径
    if [ -d "/usr/local/lib/libreoffice" ]; then
        LIBREOFFICE_PATH="/usr/local/lib/libreoffice"
    elif [ -d "/usr/lib/libreoffice" ]; then
        LIBREOFFICE_PATH="/usr/lib/libreoffice"
    else
        LIBREOFFICE_PATH="/usr/local/lib/libreoffice" # 默认路径
        log "WARNING" "未检测到LibreOffice安装路径，使用默认路径"
    fi
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
    # 确保日志目录存在
    mkdir -p "$LOG_DIR" || true
    
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
    
    # 输出到控制台（如果终端支持）
    if [ -t 1 ]; then
        echo -e "${color}[${timestamp}] [${level}] ${message}${reset}"
    else
        echo "[${timestamp}] [${level}] ${message}"
    fi
    
    # 输出到日志文件
    echo "[${timestamp}] [${level}] ${message}" >> "$LOG_DIR/deploy.log"
}

# 检查命令是否存在
exists() {
    command -v "$1" >/dev/null 2>&1
}

# 创建必要的目录
create_directories() {
    log "INFO" "创建必要的目录..."
    mkdir -p "$BUILD_DIR" "$LOG_DIR" "$PROJECT_DIR/data" "$PROJECT_DIR/data/work" "$PROJECT_DIR/data/default" || {
        log "ERROR" "无法创建必要的目录，请检查权限。"
        return 1
    }
    chmod -R 755 "$LOG_DIR" 2>/dev/null || true
    chmod -R 755 "$PROJECT_DIR/data" 2>/dev/null || true
    log "SUCCESS" "必要目录创建完成。"
}

# 安装依赖
install_dependencies() {
    log "INFO" "检查并安装系统依赖..."
    
    # 检查操作系统
    if [ -f /etc/debian_version ]; then
        # Debian/Ubuntu
        if exists apt-get; then
            apt-get update || {
                log "ERROR" "更新软件源失败。"
                return 1
            }
            apt-get install -y cmake build-essential g++ openssl libssl-dev dos2unix || {
                log "ERROR" "安装基本依赖失败。"
                return 1
            }
            apt-get install -y libreoffice libreoffice-dev || {
                log "ERROR" "安装LibreOffice失败。"
                return 1
            }
        else
            log "ERROR" "未找到apt-get命令，无法安装依赖。"
            return 1
        fi
    elif [ -f /etc/redhat-release ]; then
        # CentOS/RHEL
        if exists yum; then
            yum install -y cmake gcc-c++ openssl-devel dos2unix || {
                log "ERROR" "安装基本依赖失败。"
                return 1
            }
            yum install -y libreoffice libreoffice-devel || {
                log "ERROR" "安装LibreOffice失败。"
                return 1
            }
        elif exists dnf; then
            dnf install -y cmake gcc-c++ openssl-devel dos2unix || {
                log "ERROR" "安装基本依赖失败。"
                return 1
            }
            dnf install -y libreoffice libreoffice-devel || {
                log "ERROR" "安装LibreOffice失败。"
                return 1
            }
        else
            log "ERROR" "未找到yum或dnf命令，无法安装依赖。"
            return 1
        fi
    elif [ -f /etc/SuSE-release ] || [ -f /etc/os-release ] && grep -q "SUSE" /etc/os-release; then
        # SUSE
        if exists zypper; then
            zypper install -y cmake gcc-c++ libopenssl-devel dos2unix || {
                log "ERROR" "安装基本依赖失败。"
                return 1
            }
            zypper install -y libreoffice libreoffice-devel || {
                log "ERROR" "安装LibreOffice失败。"
                return 1
            }
        else
            log "ERROR" "未找到zypper命令，无法安装依赖。"
            return 1
        fi
    elif [ $(uname) = "FreeBSD" ]; then
        # FreeBSD
        if exists pkg; then
            pkg install -y cmake gcc openssl dos2unix || {
                log "ERROR" "安装基本依赖失败。"
                return 1
            }
            pkg install -y libreoffice || {
                log "ERROR" "安装LibreOffice失败。"
                return 1
            }
        else
            log "ERROR" "未找到pkg命令，无法安装依赖。"
            return 1
        fi
    else
        log "WARNING" "不支持的操作系统，无法自动安装依赖。请手动安装所需依赖。"
        log "INFO" "所需依赖：cmake, g++, openssl, libreoffice"
        return 0 # 在不支持的系统上不中断流程，但提示手动安装
    fi
    
    # 检查依赖是否安装成功
    local missing_deps=()
    if ! exists cmake; then missing_deps+=('cmake'); fi
    if ! exists g++ && ! exists gcc; then missing_deps+=('g++/gcc'); fi
    if ! exists openssl; then missing_deps+=('openssl'); fi
    if ! exists soffice && ! exists libreoffice; then missing_deps+=('soffice/libreoffice'); fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        log "ERROR" "以下依赖安装失败或不可用: ${missing_deps[*]}"
        log "INFO" "请手动安装这些依赖。"
        return 1
    fi
    
    log "SUCCESS" "系统依赖安装成功。"
}

# 清理文件格式
sanitize_files() {
    if exists dos2unix; then
        log "INFO" "清理文件格式（转换为Unix格式）..."
        # 使用find命令查找并转换文件格式，忽略.git目录和二进制文件
        find "$PROJECT_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "*.txt" -o -name "*.cpp" -o -name "*.sh" -o -name "*.json" -o -name "*.md" \) \
            -not -path "*/.git/*" -exec dos2unix --keep-bom {} \;
        log "SUCCESS" "文件格式清理完成。"
    else
        log "WARNING" "未找到dos2unix命令，跳过文件格式清理。"
    fi
}

# 构建项目
build_project() {
    # 检查CMakeLists.txt是否存在以及bin目录中的可执行文件是否不存在
    if [ -f "$PROJECT_DIR/CMakeLists.txt" ] && [ ! -f "$BIN_DIR/$PROJECT_NAME" ]; then
        log "INFO" "开始构建项目..."
        
        # 确保构建目录存在
        mkdir -p "$BUILD_DIR" || {
            log "ERROR" "无法创建构建目录。"
            return 1
        }
        cd "$BUILD_DIR" || {
            log "ERROR" "无法进入构建目录。"
            return 1
        }
        
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
        cmake -DCMAKE_BUILD_TYPE=Release ..
        if [ $? -ne 0 ]; then
            log "ERROR" "CMake配置失败。"
            return 1
        fi
        
        log "INFO" "编译项目..."
        # 根据可用CPU核心数设置并行编译任务数
        local cpu_cores=2
        if exists nproc; then
            cpu_cores=$(nproc 2>/dev/null || echo 2)
        elif exists sysctl && [ $(uname) = "Darwin" ]; then
            cpu_cores=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
        fi
        
        cmake --build . -j $cpu_cores
        if [ $? -ne 0 ]; then
            log "ERROR" "项目编译失败。"
            return 1
        fi
        
        # 确保bin目录存在
        mkdir -p "$BIN_DIR" || {
            log "ERROR" "无法创建bin目录。"
            return 1
        }
        
        # 复制可执行文件到bin目录
        cp -f "$BUILD_DIR/$PROJECT_NAME" "$BIN_DIR/" || {
            log "ERROR" "无法复制可执行文件到bin目录。"
            return 1
        }
        chmod +x "$BIN_DIR/$PROJECT_NAME" 2>/dev/null || true
        
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



# 配置生产环境参数
configure_production() {
    log "INFO" "配置生产环境参数..."
    
    # 确保配置文件存在
    if [ ! -f "$CONFIG_FILE" ]; then
        log "ERROR" "配置文件 $CONFIG_FILE 不存在。"
        return 1
    fi
    
    # 配置文件路径 - 在Windows环境下确保使用正确的路径格式
    local CONFIG_FILE_PATH
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下使用cygpath转换为Windows路径（如果可用）
        if exists cygpath; then
            CONFIG_FILE_PATH="$(cygpath -w "$CONFIG_FILE")"
        else
            CONFIG_FILE_PATH="$CONFIG_FILE"
        fi
    else
        # Unix/Linux环境
        CONFIG_FILE_PATH="$CONFIG_FILE"
    fi
    
    # 备份原始配置文件
    if [ ! -f "$CONFIG_FILE_PATH.bak" ]; then
        log "INFO" "备份原始配置文件到 $CONFIG_FILE_PATH.bak"
        cp "$CONFIG_FILE_PATH" "$CONFIG_FILE_PATH.bak" 2>/dev/null
        if [ $? -ne 0 ]; then
            log "WARNING" "无法备份配置文件，但将继续执行。"
        fi
    fi
    
    # 统一使用临时文件方法来修改配置，避免不同版本sed的兼容性问题
    local TEMP_FILE
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下的临时文件路径
        if exists cygpath; then
            TEMP_FILE="$(cygpath -w "$CONFIG_FILE.tmp")"
        else
            TEMP_FILE="$CONFIG_FILE.tmp"
        fi
        log "INFO" "在Windows环境下更新配置参数..."
    else
        # Unix/Linux环境下的临时文件路径
        TEMP_FILE="$CONFIG_FILE.tmp"
        log "INFO" "在Unix/Linux环境下更新配置参数..."
    fi
    
    # 修改配置参数，使用更安全的方式处理可能的空格和特殊字符
    local config_keys=(
        "log_level":0:1
        "maxOpenDocuments":10:20
    )
    
    # 复制原始文件到临时文件
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # 在Windows环境下使用cmd.exe的copy命令来处理Windows路径
        cmd.exe /c copy "$CONFIG_FILE_PATH" "$TEMP_FILE" >nul 2>&1 || {
            log "ERROR" "无法创建临时配置文件。"
            return 1
        }
    else
        # 在Unix/Linux环境下使用cp命令
        cp "$CONFIG_FILE_PATH" "$TEMP_FILE" 2>/dev/null || {
            log "ERROR" "无法创建临时配置文件。"
            return 1
        }
    fi
    
    # 逐一修改配置参数
    for key in "${config_keys[@]}"; do
        local param_name="${key%%:*}"
        local old_value="${key#*:}"
        old_value="${old_value%%:*}"
        local new_value="${key##*:}"
        
        # 根据不同操作系统使用不同的sed参数格式
        if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
            # Windows环境下使用带备份的格式
            if $SED -i .bak "s/\"$param_name\": $old_value/\"$param_name\": $new_value/g" "$TEMP_FILE"; then
                # 删除备份文件
                rm -f "$TEMP_FILE.bak" 2>/dev/null
                log "INFO" "成功将 $param_name 从 $old_value 修改为 $new_value"
            else
                log "WARNING" "无法修改参数 $param_name，配置文件格式可能不标准。"
            fi
        elif [[ "$OS_TYPE" == "FreeBSD" ]]; then
            # FreeBSD环境下使用特定的格式（需要空格分隔-i和空字符串）
            if $SED -i '' "s/\"$param_name\": $old_value/\"$param_name\": $new_value/g" "$TEMP_FILE"; then
                log "INFO" "成功将 $param_name 从 $old_value 修改为 $new_value"
            else
                log "WARNING" "无法修改参数 $param_name，配置文件格式可能不标准。"
            fi
        else
            # Linux环境下使用的格式
            if $SED -i'' "s/\"$param_name\": $old_value/\"$param_name\": $new_value/g" "$TEMP_FILE"; then
                log "INFO" "成功将 $param_name 从 $old_value 修改为 $new_value"
            else
                log "WARNING" "无法修改参数 $param_name，配置文件格式可能不标准。"
            fi
        fi
    done
    
    # 将临时文件替换回原配置文件
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # 在Windows环境下使用cmd.exe的move命令来处理Windows路径
        cmd.exe /c move /Y "$TEMP_FILE" "$CONFIG_FILE_PATH" >nul 2>&1 || {
            log "ERROR" "无法更新配置文件，可能是权限问题。"
            rm -f "$TEMP_FILE" 2>/dev/null
            return 1
        }
        log "SUCCESS" "生产环境参数配置完成。"
    else
        # 在Unix/Linux环境下使用mv命令
        if mv -f "$TEMP_FILE" "$CONFIG_FILE_PATH" 2>/dev/null; then
            log "SUCCESS" "生产环境参数配置完成。"
        else
            log "ERROR" "无法更新配置文件，可能是权限问题。"
            rm -f "$TEMP_FILE" 2>/dev/null
            return 1
        fi
    fi
}

# 启动服务
start_service() {
    # 确保bin目录存在
    if [ ! -d "$BIN_DIR" ]; then
        log "ERROR" "bin目录不存在，请先构建项目。"
        return 1
    fi
    
    # 确保bin目录下的脚本文件有执行权限
    log "INFO" "确保bin目录下的脚本文件有执行权限..."
    chmod +x "$BIN_DIR"/*.sh 2>/dev/null || {
        log "WARNING" "无法为脚本设置执行权限，可能是权限问题。"
    }
    
    # 根据操作系统类型决定启动方式
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        log "INFO" "在Windows环境下，直接启动服务..."
        cd "$BIN_DIR" || {
            log "ERROR" "无法进入bin目录。"
            return 1
        }
        # 检查start.sh是否存在且可执行
        if [ -x "./start.sh" ]; then
            bash ./start.sh
        else
            log "ERROR" "未找到可执行的start.sh脚本。"
            return 1
        fi
    elif [[ "$OS_TYPE" == "FreeBSD" ]]; then
        log "INFO" "在FreeBSD环境下，直接启动服务..."
        cd "$BIN_DIR" || {
            log "ERROR" "无法进入bin目录。"
            return 1
        }
        # 检查start.sh是否存在且可执行
        if [ -x "./start.sh" ]; then
            bash ./start.sh
        else
            log "ERROR" "未找到可执行的start.sh脚本。"
            return 1
        fi
    else
        # Linux环境
        if exists systemctl && [ -f "$SYSTEMD_SERVICE_FILE" ]; then
            log "INFO" "使用systemd启动服务..."
            systemctl start "$SERVICE_NAME"
            sleep 2
            systemctl status "$SERVICE_NAME" --no-pager
        else
            log "INFO" "直接启动服务..."
            cd "$BIN_DIR" || {
                log "ERROR" "无法进入bin目录。"
                return 1
            }
            # 检查start.sh是否存在且可执行
            if [ -x "./start.sh" ]; then
                bash ./start.sh
            else
                log "ERROR" "未找到可执行的start.sh脚本。"
                return 1
            fi
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
    # 根据操作系统类型决定停止方式
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        log "INFO" "在Windows环境下，直接停止服务..."
        cd "$BIN_DIR" 2>/dev/null || {
            log "WARNING" "无法进入bin目录，尝试直接停止进程。"
        }
        # 检查stop.sh是否存在且可执行
        if [ -x "./stop.sh" ]; then
            bash ./stop.sh
        else
            log "WARNING" "未找到stop.sh脚本，尝试直接终止进程。"
        fi
    elif [[ "$OS_TYPE" == "FreeBSD" ]]; then
        log "INFO" "在FreeBSD环境下，直接停止服务..."
        cd "$BIN_DIR" 2>/dev/null || {
            log "WARNING" "无法进入bin目录，尝试直接停止进程。"
        }
        # 检查stop.sh是否存在且可执行
        if [ -x "./stop.sh" ]; then
            bash ./stop.sh
        else
            log "WARNING" "未找到stop.sh脚本，尝试直接终止进程。"
        fi
    else
        # Linux环境
        if exists systemctl && [ -f "$SYSTEMD_SERVICE_FILE" ]; then
            log "INFO" "使用systemd停止服务..."
            systemctl stop "$SERVICE_NAME"
        else
            log "INFO" "直接停止服务..."
            cd "$BIN_DIR" 2>/dev/null || {
                log "WARNING" "无法进入bin目录，尝试直接停止进程。"
            }
            # 检查stop.sh是否存在且可执行
            if [ -x "./stop.sh" ]; then
                bash ./stop.sh
            else
                log "WARNING" "未找到stop.sh脚本，尝试直接终止进程。"
            fi
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
            # 尝试优雅终止，如果失败则强制终止
            pkill -f "$PROJECT_NAME" 2>/dev/null || pkill -9 -f "$PROJECT_NAME" 2>/dev/null
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
    # 确保构建目录存在
    if [ ! -d "$BUILD_DIR" ]; then
        log "INFO" "构建目录不存在，无需清理。"
        return 0
    fi
    
    # 根据操作系统使用不同的清理命令
    if [[ "$OS_TYPE" == *"MINGW"* || "$OS_TYPE" == *"CYGWIN"* ]]; then
        # Windows环境下的清理命令
        cd "$BUILD_DIR" || {
            log "ERROR" "无法进入构建目录。"
            return 1
        }
        # 使用rm -rf ./* 而不是rm -rf "$BUILD_DIR/*"，因为Windows路径可能有问题
        rm -rf ./* 2>/dev/null || {
            log "ERROR" "清理构建文件失败，可能是权限问题。"
            return 1
        }
    else
        # Unix/Linux环境下的清理命令
        rm -rf "$BUILD_DIR/*" 2>/dev/null || {
            log "ERROR" "清理构建文件失败，可能是权限问题。"
            return 1
        }
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
    mkdir -p "$LOG_DIR" || {
        log "WARNING" "无法创建日志目录，日志将无法保存到文件。"
    }
    
    case "$1" in
        "install")
            create_directories
            sanitize_files
            install_dependencies
            # SSL证书生成已禁用，如需启用请取消注释下一行
            # generate_ssl_certificates
            ;;
        "build")
            build_project
            ;;
        "configure")
            configure_production
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
            # SSL证书生成已禁用，如需启用请取消注释下一行
            # generate_ssl_certificates
            build_project
            configure_production
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