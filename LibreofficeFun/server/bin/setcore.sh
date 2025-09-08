#!/bin/bash

# 确保日志目录存在
mkdir -p ../log 2>/dev/null

# 定义日志文件路径
LOG_FILE="../log/setcore.log"

# 记录日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

log "================ 开始配置 core 文件生成环境 ================"

# 检测操作系统类型
OS_TYPE="linux"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    OS_TYPE="windows"
    log "检测到 Windows 环境"
fi

# 根据操作系统类型执行不同的配置
if [ "$OS_TYPE" = "windows" ]; then
    # Windows 环境下的配置
    log "在 Windows 环境下配置 core dump 设置..."
    
    # 检查是否有管理员权限
    log "检查管理员权限..."
    ADMIN_STATUS=$(powershell -Command "& { 
        try { 
            \$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
            \$currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
        } catch { 
            echo 'false' 
        } 
    }" 2>/dev/null)
    
    if [ "$ADMIN_STATUS" = "True" ]; then
        log "✓ 当前有管理员权限，可以配置系统级核心转储设置"
        
        # 启用 Windows 的应用程序崩溃转储
        log "启用 Windows 应用程序崩溃转储..."
        powershell -Command "& { 
            New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps' -Name DumpFolder -Value 'e:/data/CodeUnit/LibreofficeFun/server/core_dumps' -PropertyType String -Force
            New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps' -Name DumpType -Value 2 -PropertyType DWord -Force
            New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps' -Name DumpCount -Value 10 -PropertyType DWord -Force
        }" > /dev/null 2>&1
        
        if [ $? -eq 0 ]; then
            log "✓ Windows 崩溃转储配置已完成，转储文件将保存到: e:/data/CodeUnit/LibreofficeFun/server/core_dumps"
            log "注意: 此配置需要重启程序才能生效"
        else
            log "✗ 无法设置 Windows 崩溃转储配置，可能需要系统重启或更高级别的权限"
        fi
        
        # 创建 core_dumps 目录
        log "创建 core_dumps 目录..."
        mkdir -p "../core_dumps" 2>/dev/null
        if [ $? -eq 0 ]; then
            log "✓ core_dumps 目录创建成功"
        else
            log "✗ 无法创建 core_dumps 目录"
        fi
    else
        log "警告: 当前没有管理员权限，无法设置系统级核心转储配置"
        log "建议以管理员身份运行此脚本，或手动配置 Windows 错误报告设置"
    fi
    
    # 使用 MinGW/Cygwin 的 ulimit 命令尝试设置 core 文件大小（如果可用）
    if command -v ulimit > /dev/null 2>&1; then
        log "尝试设置 core 文件大小不受限制..."
        ulimit -c unlimited 2>/dev/null
        
        CORE_LIMIT=$(ulimit -c 2>/dev/null)
        if [ -n "$CORE_LIMIT" ] && { [ "$CORE_LIMIT" = "unlimited" ] || [ "$CORE_LIMIT" -gt 0 ]; }; then
            log "✓ core 文件大小限制已设置为: $CORE_LIMIT"
        else
            log "✗ 无法设置 core 文件大小限制，请检查系统配置"
        fi
    fi
else
    # Linux/Unix 环境下的配置
    log "在 Linux/Unix 环境下配置 core 文件生成环境..."
    
    # 保存原始设置以便可能的恢复
    ORIG_CORE_PATTERN=""
    ORIG_CORE_USES_PID=""
    ORIG_CORE_LIMIT="$(ulimit -c 2>/dev/null)"
    
    # 设置 core 文件大小不受限制
    log "设置 core 文件大小不受限制..."
    ulimit -c unlimited 2>/dev/null
    
    # 检查 core 文件限制是否设置成功
    CORE_LIMIT=$(ulimit -c 2>/dev/null)
    if [ -n "$CORE_LIMIT" ] && { [ "$CORE_LIMIT" = "unlimited" ] || [ "$CORE_LIMIT" -gt 0 ]; }; then
        log "✓ core 文件大小限制已设置为: $CORE_LIMIT"
    else
        log "✗ 无法设置 core 文件大小限制，请检查系统配置"
    fi
    
    # 设置 core 文件保存路径和命名格式
    CORE_DIR="../core_dumps"
    if [ ! -d "$CORE_DIR" ]; then
        log "创建 core 文件保存目录: $CORE_DIR"
        mkdir -p "$CORE_DIR" 2>/dev/null
        if [ $? -ne 0 ]; then
            log "✗ 无法创建 core 文件保存目录，将使用当前目录"
            CORE_DIR="."
        else
            log "✓ core 文件保存目录创建成功"
        fi
    fi
    
    # 设置目录权限（确保可写入）
    log "设置 core 文件目录权限..."
    chmod 770 "$CORE_DIR" 2>/dev/null
    if [ $? -ne 0 ]; then
        log "警告: 无法设置 core 文件目录权限，可能影响 core 文件生成"
    fi
    
    # 设置 core 文件保存路径和格式
    if [ -f "/proc/sys/kernel/core_pattern" ]; then
        # 保存原始配置以便恢复
        ORIG_CORE_PATTERN=$(cat /proc/sys/kernel/core_pattern 2>/dev/null)
        log "原始 core_pattern: $ORIG_CORE_PATTERN"
        
        # 设置新的 core 文件格式
        log "设置 core 文件格式..."
        echo "$CORE_DIR/core.%e.%p.%t" > /proc/sys/kernel/core_pattern 2>/dev/null
        if [ $? -eq 0 ]; then
            log "✓ core 文件格式已设置为: $CORE_DIR/core.program.pid.timestamp"
        else
            log "✗ 无法设置 core 文件格式，可能需要 root 权限"
            log "尝试使用临时环境变量..."
            export CORE_PATTERN="$CORE_DIR/core.%e.%p.%t"
        fi
    fi
    
    # 启用 PID 作为 core 文件名的一部分
    if [ -f "/proc/sys/kernel/core_uses_pid" ]; then
        ORIG_CORE_USES_PID=$(cat /proc/sys/kernel/core_uses_pid 2>/dev/null)
        log "启用 PID 作为 core 文件名的一部分..."
        echo 1 > /proc/sys/kernel/core_uses_pid 2>/dev/null
        if [ $? -eq 0 ]; then
            log "✓ core_uses_pid 已启用"
        else
            log "✗ 无法启用 core_uses_pid，可能需要 root 权限"
        fi
    fi
    
    # 添加清理函数以在脚本退出时恢复原始设置（可选）
    cleanup_core_settings() {
        log "恢复原始 core 文件设置..."
        
        # 恢复 core 文件大小限制
        if [ -n "$ORIG_CORE_LIMIT" ]; then
            ulimit -c "$ORIG_CORE_LIMIT" 2>/dev/null
        fi
        
        # 恢复 core_pattern
        if [ -f "/proc/sys/kernel/core_pattern" ] && [ -n "$ORIG_CORE_PATTERN" ]; then
            echo "$ORIG_CORE_PATTERN" > /proc/sys/kernel/core_pattern 2>/dev/null
        fi
        
        # 恢复 core_uses_pid
        if [ -f "/proc/sys/kernel/core_uses_pid" ] && [ -n "$ORIG_CORE_USES_PID" ]; then
            echo "$ORIG_CORE_USES_PID" > /proc/sys/kernel/core_uses_pid 2>/dev/null
        fi
        
        log "原始 core 文件设置已恢复"
    }
    
    # 注册清理函数（可选，取消下一行注释以启用）
    # trap cleanup_core_settings EXIT
fi

log "core 文件配置完成"
log "================ core 文件配置结束 ================"

# 返回成功状态码
exit 0