#!/bin/bash

# 定义日志文件路径
LOG_FILE="../log/start_service.log"

# 记录日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# 确保日志目录存在
mkdir -p "$(dirname "$LOG_FILE")"

log "================ 开始启动服务 ================"

# 确保 setcore.sh 有执行权限
log "确保 setcore.sh 有执行权限"
chmod +x ./setcore.sh 2>/dev/null

# 执行 core 文件配置脚本
log "执行 core 文件配置脚本"
bash ./setcore.sh

# 重新执行停止流程，确保完全退出
log "重新执行停止流程，确保完全退出"
bash stop.sh 

# 复制主程序
log "复制主程序文件"
cp ../build/libreofficefun .

# 确保主程序有执行权限
log "确保主程序有执行权限"
chmod +x ./libreofficefun 2>/dev/null

# 函数：启动 LibreOffice 服务
start_libreoffice() {
    local lo_path=$1
    local use_bootstrap=$2
    local bootstrap_path=$3
    local os_type=$4
    
    if [ "$use_bootstrap" = true ] && [ -n "$bootstrap_path" ]; then
        export URE_BOOTSTRAP="$bootstrap_path"
        log "设置 URE_BOOTSTRAP 环境变量: $URE_BOOTSTRAP"
    else
        unset URE_BOOTSTRAP
        log "不使用 URE_BOOTSTRAP 环境变量"
    fi
    
    # 启动 LibreOffice
    log "执行 LibreOffice 启动命令: soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\" --nologo --norestore --nolockcheck"
    if [ "$os_type" = "windows" ] && [ -n "$lo_path" ]; then
        log "在 Windows 环境下使用完整路径启动 LibreOffice: $lo_path/soffice.exe"
        "$lo_path/soffice.exe" --headless --accept="socket,host=127.0.0.1,port=2002;urp;" --nologo --norestore --nolockcheck &
    else
        log "使用默认方式启动 LibreOffice"
        # 检查soffice命令是否存在
        if command -v soffice >/dev/null 2>&1; then
            log "找到soffice命令: $(command -v soffice)"
            soffice --headless --accept="socket,host=127.0.0.1,port=2002;urp;" --nologo --norestore --nolockcheck &
        elif command -v libreoffice >/dev/null 2>&1; then
            log "未找到soffice命令，但找到libreoffice命令: $(command -v libreoffice)"
            libreoffice --headless --accept="socket,host=127.0.0.1,port=2002;urp;" --nologo --norestore --nolockcheck &
        else
            log "错误: 未找到soffice或libreoffice命令，请确认LibreOffice已正确安装"
            return 1
        fi
    fi
    
    # 不依赖返回的PID，返回启动命令的退出状态
    return $?
}

# 检测操作系统类型并设置正确的环境变量和启动命令
OS_TYPE="linux"
LO_PATH=""
URE_BOOTSTRAP_VALUE=""

# 检测FreeBSD
if [[ "$OSTYPE" == "freebsd"* ]]; then
    OS_TYPE="freebsd"
    log "检测到 FreeBSD 环境"
    
    # FreeBSD 环境下的特殊处理
    log "检测LibreOffice安装路径..."
    if command -v soffice >/dev/null 2>&1; then
        SOFFICE_PATH=$(command -v soffice)
        LO_INSTALL_DIR=$(dirname $(dirname $SOFFICE_PATH))
        log "找到soffice命令: $SOFFICE_PATH"
        log "LibreOffice安装目录: $LO_INSTALL_DIR"
    fi
    
    # 检查FreeBSD下常见的LibreOffice安装路径
    if [ -d /usr/local/lib/libreoffice/program ]; then
        URE_BOOTSTRAP_VALUE="file:///usr/local/lib/libreoffice/program/fundamentalrc"
        log "使用LibreOffice路径: /usr/local/lib/libreoffice/program"
    else
        # 尝试自动检测fundamentalrc文件位置
        FUNDAMENTALRC_PATH=$(find /usr -name "fundamentalrc" 2>/dev/null | head -1)
        if [ -n "$FUNDAMENTALRC_PATH" ]; then
            URE_BOOTSTRAP_VALUE="file://$FUNDAMENTALRC_PATH"
            log "自动检测到fundamentalrc文件: $FUNDAMENTALRC_PATH"
        else
            URE_BOOTSTRAP_VALUE="file:///usr/local/lib/libreoffice/program/fundamentalrc"
            log "警告: 未找到fundamentalrc文件，使用默认路径"
        fi
    fi
    
    # 启动 LibreOffice
    start_libreoffice "" true "$URE_BOOTSTRAP_VALUE" "$OS_TYPE"
    LO_START_STATUS=$?
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    OS_TYPE="windows"
    log "检测到 Windows 环境"
    
    # 在 Windows 环境下，使用 Windows 风格路径
    # 自动查找 LibreOffice 安装路径
    log "自动查找 LibreOffice 安装路径"
    
    # 标准路径检查 - 在bash中Windows路径需要特殊处理
    if [ -d "$PROGRAMFILES/LibreOffice/program" ]; then
        LO_PATH="$PROGRAMFILES/LibreOffice/program"
    elif [ -d "$PROGRAMFILES(x86)/LibreOffice/program" ]; then
        LO_PATH="$PROGRAMFILES(x86)/LibreOffice/program"
    fi
    
    # 检查注册表（通过 PowerShell）来获取 LibreOffice 安装路径
    if [ -z "$LO_PATH" ]; then
        log "通过注册表检查 LibreOffice 安装路径"
        REG_RESULT=$(powershell -Command "& { 
            try { 
                Get-ItemProperty 'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreOffice*' | Select-Object -ExpandProperty InstallLocation -ErrorAction SilentlyContinue 
            } catch { 
                echo '' 
            } 
        }" 2>/dev/null)
        
        if [ -n "$REG_RESULT" ]; then
            # 将 PowerShell 返回的路径转换为适合 bash 使用的格式
            LO_PATH_WIN=$(echo "$REG_RESULT" | tr -d '\r' | sed 's/\\/\\\\/g')
            LO_PATH="$LO_PATH_WIN\\program"
        fi
    fi
    
    if [ -z "$LO_PATH" ]; then
        log "检查常见的自定义安装路径"
        if [ -d "D:/Program Files/LibreOffice/program" ]; then
            LO_PATH="D:/Program Files/LibreOffice/program"
        elif [ -d "E:/LibreOffice/program" ]; then
            LO_PATH="E:/LibreOffice/program"
        fi
    fi
    
    if [ -n "$LO_PATH" ]; then
        log "检测到 LibreOffice 安装路径: $LO_PATH"
        # 在 bash 中使用 Windows 路径时，需要转换为适合 URL 的格式
        LO_PATH_FOR_URL=$(echo "$LO_PATH" | sed 's/\\/\//g' | sed 's/://g')
        URE_BOOTSTRAP_VALUE="file:///$LO_PATH_FOR_URL/fundamentalrc"
        
        # 启动 LibreOffice
        start_libreoffice "$LO_PATH" true "$URE_BOOTSTRAP_VALUE" "$OS_TYPE"
        LO_PID=$?
    else
        log "警告: 未找到 LibreOffice 安装路径，使用默认配置"
        URE_BOOTSTRAP_VALUE="file:///usr/local/lib/libreoffice/program/fundamentalrc"
        
        # 启动 LibreOffice
        start_libreoffice "" true "$URE_BOOTSTRAP_VALUE" "$OS_TYPE"
        LO_PID=$?
    fi
else
    OS_TYPE="linux"
    log "检测到 Linux/Unix 环境"
    # 检测LibreOffice安装路径
    log "检测LibreOffice安装路径..."
    if command -v soffice >/dev/null 2>&1; then
        SOFFICE_PATH=$(command -v soffice)
        LO_INSTALL_DIR=$(dirname $(dirname $SOFFICE_PATH))
        log "找到soffice命令: $SOFFICE_PATH"
        log "LibreOffice安装目录: $LO_INSTALL_DIR"
    fi
    
    # 检查常见的LibreOffice安装路径
    if [ -d /usr/lib/libreoffice/program ]; then
        URE_BOOTSTRAP_VALUE="file:///usr/lib/libreoffice/program/fundamentalrc"
        log "使用LibreOffice路径: /usr/lib/libreoffice/program"
    elif [ -d /usr/local/lib/libreoffice/program ]; then
        URE_BOOTSTRAP_VALUE="file:///usr/local/lib/libreoffice/program/fundamentalrc"
        log "使用LibreOffice路径: /usr/local/lib/libreoffice/program"
    else
        # 尝试自动检测fundamentalrc文件位置
        FUNDAMENTALRC_PATH=$(find /usr -name "fundamentalrc" 2>/dev/null | head -1)
        if [ -n "$FUNDAMENTALRC_PATH" ]; then
            URE_BOOTSTRAP_VALUE="file://$FUNDAMENTALRC_PATH"
            log "自动检测到fundamentalrc文件: $FUNDAMENTALRC_PATH"
        else
            URE_BOOTSTRAP_VALUE="file:///usr/local/lib/libreoffice/program/fundamentalrc"
            log "警告: 未找到fundamentalrc文件，使用默认路径"
        fi
    fi
    
    # 启动 LibreOffice
    start_libreoffice "" true "$URE_BOOTSTRAP_VALUE" "$OS_TYPE"
    LO_START_STATUS=$?
fi

# 等待 LibreOffice 服务启动
log "等待 LibreOffice 服务启动..."
sleep 5  # 增加等待时间，确保服务有足够时间启动

# 检查 LibreOffice 是否成功启动 - 使用更可靠的检测方法
log "检查 LibreOffice 服务状态..."

# 方法1: 检查端口是否监听
PORT_LISTENING=false
if command -v netstat >/dev/null 2>&1; then
    if netstat -an | grep :2002 | grep LISTEN >/dev/null; then
        PORT_LISTENING=true
    fi
elif command -v ss >/dev/null 2>&1; then
    if ss -an | grep :2002 | grep LISTEN >/dev/null; then
        PORT_LISTENING=true
    fi
fi

# 方法2: 检查LibreOffice进程是否存在
PROCESS_RUNNING=false
if pgrep -f "soffice.*headless.*port=2002" >/dev/null 2>&1; then
    PROCESS_RUNNING=true
    ACTUAL_PID=$(pgrep -f "soffice.*headless.*port=2002" | head -1)
elif pgrep -f "libreoffice.*headless.*port=2002" >/dev/null 2>&1; then
    PROCESS_RUNNING=true
    ACTUAL_PID=$(pgrep -f "libreoffice.*headless.*port=2002" | head -1)
fi

# 综合判断
if [ "$PORT_LISTENING" = true ] || [ "$PROCESS_RUNNING" = true ]; then
    if [ "$PROCESS_RUNNING" = true ]; then
        log "✓ LibreOffice 服务已成功启动 (实际PID: $ACTUAL_PID)"
    else
        log "✓ LibreOffice 服务已成功启动 (端口2002已监听)"
    fi
    # 显示端口监听状态
    if command -v netstat >/dev/null 2>&1; then
        netstat -an | grep :2002
    elif command -v ss >/dev/null 2>&1; then
        ss -an | grep :2002
    fi
else
    log "错误: LibreOffice 服务启动失败，尝试使用备用配置..."
    # 备用启动方式 - 不依赖 URE_BOOTSTRAP
    
    start_libreoffice "$LO_PATH" false "" "$OS_TYPE"
    LO_START_STATUS=$?
    log "备用配置启动状态: $LO_START_STATUS"
    
    # 再次等待
    sleep 5
    
    # 再次检查
    PORT_LISTENING=false
    if command -v netstat >/dev/null 2>&1; then
        if netstat -an | grep :2002 | grep LISTEN >/dev/null; then
            PORT_LISTENING=true
        fi
    elif command -v ss >/dev/null 2>&1; then
        if ss -an | grep :2002 | grep LISTEN >/dev/null; then
            PORT_LISTENING=true
        fi
    fi
    
    PROCESS_RUNNING=false
    if pgrep -f "soffice.*headless.*port=2002" >/dev/null 2>&1; then
        PROCESS_RUNNING=true
        ACTUAL_PID=$(pgrep -f "soffice.*headless.*port=2002" | head -1)
    elif pgrep -f "libreoffice.*headless.*port=2002" >/dev/null 2>&1; then
        PROCESS_RUNNING=true
        ACTUAL_PID=$(pgrep -f "libreoffice.*headless.*port=2002" | head -1)
    fi
    
    if [ "$PORT_LISTENING" = true ] || [ "$PROCESS_RUNNING" = true ]; then
        if [ "$PROCESS_RUNNING" = true ]; then
            log "✓ LibreOffice 服务已成功启动（备用配置） (实际PID: $ACTUAL_PID)"
        else
            log "✓ LibreOffice 服务已成功启动（备用配置） (端口2002已监听)"
        fi
    else
        log "错误: LibreOffice 服务启动失败"
        log "请检查 LibreOffice 是否已正确安装"
        log "尝试手动执行命令: soffice --headless --accept='socket,host=127.0.0.1,port=2002;urp;'"
        log "或: libreoffice --headless --accept='socket,host=127.0.0.1,port=2002;urp;'"
        log "检查是否安装了LibreOffice: dpkg -l | grep libreoffice (Debian/Ubuntu) 或 rpm -qa | grep libreoffice (CentOS/RHEL)"
    fi
fi

# 检查主程序文件是否存在
if [ ! -f "./libreofficefun" ]; then
    log "错误: 主程序文件 libreofficefun 不存在"
    log "服务启动流程失败"
    log "================ 启动服务结束 ================"
    exit 1
fi

# 启动主程序
log "启动主程序..."
./libreofficefun &
MAIN_PID=$?
log "主程序已启动 (PID: $MAIN_PID)"

# 检查主程序是否成功启动
if ps -p $MAIN_PID > /dev/null; then
    log "✓ 主程序启动成功"
else
    log "错误: 主程序启动失败"
    # 尝试强制终止可能残留的 LibreOffice 进程
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
        powershell -Command "Stop-Process -Name soffice -Force -ErrorAction SilentlyContinue"
    else
        pkill -f "soffice.*headless.*port=2002" 2>/dev/null
    fi
fi

log "所有服务启动流程已完成。"
log "================ 启动服务结束 ================"
exit 0
