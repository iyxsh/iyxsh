#!/bin/bash

# 确保日志目录存在
mkdir -p ../log 2>/dev/null

# 定义日志文件路径
LOG_FILE="../log/stop_service.log"

# 记录日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

log "================ 开始停止服务 ================"

# 检测操作系统类型
OS_TYPE="linux"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    OS_TYPE="windows"
    log "检测到 Windows 环境"
fi

# 定义主程序进程名和 LibreOffice 服务进程模式
MAIN_PROCESS="libreofficefun"
LIBREOFFICE_PATTERN="soffice.*headless.*port=2002"

# 根据操作系统类型设置不同的命令
if [ "$OS_TYPE" = "windows" ]; then
    # Windows 环境下使用 PowerShell 命令
    PGREP_CMD="powershell -Command '& { 
        try { 
            Get-Process | Where-Object { $_.ProcessName -match "${1/-f /}" } | Select-Object -ExpandProperty Id 
        } catch { 
            echo ""
        } 
    }'"
    PKILL_TERM_CMD="powershell -Command '& { 
        try { 
            Get-Process | Where-Object { $_.ProcessName -match "${1/-TERM -f /}" } | ForEach-Object { 
                $_.CloseMainWindow() | Out-Null 
            } 
        } catch { 
            echo ""
        } 
    }'"
    PKILL_KILL_CMD="powershell -Command '& { 
        try { 
            Get-Process | Where-Object { $_.ProcessName -match "${1/-9 -f /}" } | Stop-Process -Force 
        } catch { 
            echo ""
        } 
    }'"
else
    # Linux/Unix 环境下使用标准命令
    PGREP_CMD="pgrep -f"
    PKILL_TERM_CMD="pkill -TERM -f"
    PKILL_KILL_CMD="pkill -9 -f"
fi

# 进程检查函数
check_process() {
    local process_pattern=$1
    
    if [ "$OS_TYPE" = "windows" ]; then
        # Windows 环境下检查进程
        local result=$(powershell -Command "& { 
            try { 
                Get-Process | Where-Object { $_.ProcessName -match '$process_pattern' } | Select-Object -First 1 -ExpandProperty Id -ErrorAction SilentlyContinue 
            } catch { 
                echo '' 
            } 
        }" 2>/dev/null)
        
        if [ -n "$result" ] && [ "$result" != "" ]; then
            return 0  # 进程存在
        else
            return 1  # 进程不存在
        fi
    else
        # Linux/Unix 环境下检查进程
        $PGREP_CMD "$process_pattern" > /dev/null 2>&1
        return $?
    fi
}

# 优雅停止进程函数
stop_process_gracefully() {
    local process_pattern=$1
    local process_name=$2
    
    log "尝试优雅停止 $process_name 进程..."
    
    if check_process "$process_pattern"; then
        log "检测到 $process_name 进程，尝试优雅终止..."
        
        if [ "$OS_TYPE" = "windows" ]; then
            # Windows 环境下的优雅停止方式
            powershell -Command "Get-Process | Where-Object { $_.ProcessName -match '$process_pattern' } | ForEach-Object { $_.CloseMainWindow() }" > /dev/null 2>&1
        else
            # Linux/Unix 环境下的优雅停止方式
            $PKILL_TERM_CMD "$process_pattern" > /dev/null 2>&1
        fi
        
        # 等待进程优雅退出
        log "等待 3 秒让 $process_name 进程优雅退出..."
        sleep 3
        
        # 再次检查进程是否已停止
        if check_process "$process_pattern"; then
            log "警告: $process_name 未能优雅退出，强制终止..."
            
            if [ "$OS_TYPE" = "windows" ]; then
                # Windows 环境下的强制终止方式
                powershell -Command "Get-Process | Where-Object { $_.ProcessName -match '$process_pattern' } | Stop-Process -Force" > /dev/null 2>&1
            else
                # Linux/Unix 环境下的强制终止方式
                $PKILL_KILL_CMD "$process_pattern" > /dev/null 2>&1
            fi
            
            # 再次等待
            sleep 1
            
            # 最后检查
            if check_process "$process_pattern"; then
                log "错误: 无法终止 $process_name 进程"
                return 1
            else
                log "✓ $process_name 进程已被强制终止"
                return 0
            fi
        else
            log "✓ $process_name 进程已优雅停止"
            return 0
        fi
    else
        log "✓ $process_name 进程未运行"
        return 0
    fi
}

# 尝试优雅停止主程序
stop_process_gracefully "$MAIN_PROCESS" "主程序"
MAIN_PROCESS_STATUS=$?

# 停止 LibreOffice 服务
stop_process_gracefully "$LIBREOFFICE_PATTERN" "LibreOffice 服务"
LO_SERVICE_STATUS=$?

# 再次检查所有相关进程
log "最终检查相关进程..."
REMAINING_PROCESSES=0

if check_process "$MAIN_PROCESS"; then
    log "错误: $MAIN_PROCESS 进程仍然在运行"
    REMAINING_PROCESSES=1
fi

if check_process "$LIBREOFFICE_PATTERN"; then
    log "错误: LibreOffice 服务进程仍然在运行"
    REMAINING_PROCESSES=1
fi

# 检查是否有其他相关进程（如文件队列处理）
if [ "$OS_TYPE" = "windows" ]; then
    FILE_QUEUE_PROCESSES=$(powershell -Command "& { 
        try { 
            Get-Process | Where-Object { $_.ProcessName -match '$MAIN_PROCESS' -and $_.CommandLine -match 'FileQueue' } | Select-Object -ExpandProperty Id 
        } catch { 
            echo '' 
        } 
    }" 2>/dev/null)
else
    FILE_QUEUE_PROCESSES=$(pgrep -f "$MAIN_PROCESS.*FileQueue" 2>/dev/null)
fi

if [ -n "$FILE_QUEUE_PROCESSES" ]; then
    log "警告: 检测到文件队列相关进程，尝试终止..."
    if [ "$OS_TYPE" = "windows" ]; then
        powershell -Command "Get-Process -Id $FILE_QUEUE_PROCESSES | Stop-Process -Force" > /dev/null 2>&1
    else
        pkill -9 -f "$MAIN_PROCESS.*FileQueue" > /dev/null 2>&1
    fi
    # 等待进程终止
    sleep 1
fi

if [ $REMAINING_PROCESSES -eq 0 ]; then
    log "✓ 所有相关进程已成功停止"
else
    log "警告: 部分进程未能完全停止，可能需要手动检查"
    if [ "$OS_TYPE" = "windows" ]; then
        log "建议使用 Task Manager 或 PowerShell 命令 'Get-Process | Where-Object { $_.ProcessName -match \"soffice|libreoffice\" }' 查看残留进程"
    else
        log "建议使用 'ps -ef | grep libreoffice' 查看残留进程"
    fi
fi

# 清理临时文件
log "清理临时文件..."
TEMP_DIR="../temp"
if [ -d "$TEMP_DIR" ]; then
    # 检查并确保有删除权限
    if [ -w "$TEMP_DIR" ]; then
        # 先尝试安全删除文件，保留目录结构
        find "$TEMP_DIR" -type f -delete 2>/dev/null
        # 删除空目录
        find "$TEMP_DIR" -type d -empty -delete 2>/dev/null
        log "✓ 临时文件已清理完成"
    else
        log "警告: 没有权限清理临时文件，跳过此步骤"
        log "建议手动清理临时目录: $TEMP_DIR"
    fi
else
    log "✓ 临时文件目录不存在，无需清理"
fi

log "服务停止流程已完成"
log "================ 停止服务结束 ================"

# 返回适当的退出码
exit $REMAINING_PROCESSES