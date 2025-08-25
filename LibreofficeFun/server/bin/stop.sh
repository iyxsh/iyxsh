#!/bin/bash

# 定义日志文件路径
LOG_FILE="../log/stop_service.log"

# 记录日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

log "================ 开始停止服务 ================"

# 定义主程序进程名
MAIN_PROCESS="libreofficefun"
# 定义 LibreOffice 服务进程模式
LIBREOFFICE_PATTERN="soffice.*headless.*port=2002"

# 尝试优雅停止主程序（发送 SIGTERM 信号）
log "尝试优雅停止 $MAIN_PROCESS 主程序..."
if pgrep -f "$MAIN_PROCESS" > /dev/null 2>&1; then
    log "检测到 $MAIN_PROCESS 进程，发送 SIGTERM 信号..."
    pkill -TERM -f "$MAIN_PROCESS" 2>/dev/null
    
    # 等待主程序优雅退出
    log "等待 3 秒让主程序优雅退出..."
    sleep 3
    
    # 检查主程序是否已停止
    if pgrep -f "$MAIN_PROCESS" > /dev/null 2>&1; then
        log "警告: $MAIN_PROCESS 未能优雅退出，强制终止..."
        pkill -9 -f "$MAIN_PROCESS" 2>/dev/null
    else
        log "✓ $MAIN_PROCESS 主程序已优雅停止"
    fi
else
    log "✓ $MAIN_PROCESS 主程序未运行"
fi

# 停止 LibreOffice 服务
log "停止 LibreOffice 服务..."
if pgrep -f "$LIBREOFFICE_PATTERN" > /dev/null 2>&1; then
    log "检测到 LibreOffice 服务进程，发送 SIGTERM 信号..."
    pkill -TERM -f "$LIBREOFFICE_PATTERN" 2>/dev/null
    
    # 等待 LibreOffice 服务退出
    log "等待 2 秒让 LibreOffice 服务退出..."
    sleep 2
    
    # 检查 LibreOffice 服务是否已停止
    if pgrep -f "$LIBREOFFICE_PATTERN" > /dev/null 2>&1; then
        log "警告: LibreOffice 服务未能优雅退出，强制终止..."
        pkill -9 -f "$LIBREOFFICE_PATTERN" 2>/dev/null
    else
        log "✓ LibreOffice 服务已优雅停止"
    fi
else
    log "✓ LibreOffice 服务未运行"
fi

# 再次检查所有相关进程
log "最终检查相关进程..."
REMAINING_PROCESSES=0

if pgrep -f "$MAIN_PROCESS" > /dev/null 2>&1; then
    log "错误: $MAIN_PROCESS 进程仍然在运行"
    REMAINING_PROCESSES=1
fi

if pgrep -f "$LIBREOFFICE_PATTERN" > /dev/null 2>&1; then
    log "错误: LibreOffice 服务进程仍然在运行"
    REMAINING_PROCESSES=1
fi

# 检查是否有其他相关进程（如文件队列处理）
FILE_QUEUE_PROCESSES=$(pgrep -f "$MAIN_PROCESS.*FileQueue")
if [ -n "$FILE_QUEUE_PROCESSES" ]; then
    log "警告: 检测到文件队列相关进程，尝试终止..."
    pkill -f "$MAIN_PROCESS.*FileQueue" 2>/dev/null
fi

if [ $REMAINING_PROCESSES -eq 0 ]; then
    log "✓ 所有相关进程已成功停止"
else
    log "警告: 部分进程未能完全停止，可能需要手动检查"
    log "建议使用 'ps -ef | grep libreoffice' 查看残留进程"
fi

# 清理临时文件（可选）
if [ -d "../temp" ]; then
    log "清理临时文件..."
    rm -rf ../temp/* 2>/dev/null
fi

log "服务停止流程已完成"
log "================ 停止服务结束 ================"

# 返回适当的退出码
if [ $REMAINING_PROCESSES -eq 0 ]; then
    exit 0
else
    exit 1
fi