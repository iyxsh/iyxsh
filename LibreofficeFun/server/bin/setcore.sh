#!/bin/bash

# 配置 core dump 环境的专用脚本
# 该脚本由 start.sh 调用，用于设置 core 文件生成环境

# 配置 core 文件生成
echo "正在配置 core 文件生成环境..."

# 设置 core 文件大小不受限制
ulimit -c unlimited

# 检查 core 文件限制是否设置成功
CORE_LIMIT=$(ulimit -c)
if [ "$CORE_LIMIT" = "unlimited" ] || [ "$CORE_LIMIT" -gt 0 ]; then
    echo "✓ core 文件大小限制已设置为: $CORE_LIMIT"
else
    echo "✗ 无法设置 core 文件大小限制，请检查系统配置"
fi

# 设置 core 文件保存路径和命名格式
CORE_DIR="/var/core"
if [ ! -d "$CORE_DIR" ]; then
    echo "创建 core 文件保存目录: $CORE_DIR"
    mkdir -p "$CORE_DIR" || {
        echo "✗ 无法创建 core 文件保存目录，将使用当前目录"
        CORE_DIR="."
    }
fi

echo "设置 core 文件保存路径和格式..."
if [ -f "/proc/sys/kernel/core_pattern" ]; then
    # 保存原始配置以便恢复
    ORIG_CORE_PATTERN=$(cat /proc/sys/kernel/core_pattern)
    echo "原始 core_pattern: $ORIG_CORE_PATTERN"
    
    # 设置新的 core 文件格式
    echo "$CORE_DIR/core.%e.%p.%t" > /proc/sys/kernel/core_pattern 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "✓ core 文件格式已设置为: $CORE_DIR/core.program.pid.timestamp"
    else
        echo "✗ 无法设置 core 文件格式，可能需要 root 权限"
        echo "尝试使用临时环境变量..."
        export CORE_PATTERN="$CORE_DIR/core.%e.%p.%t"
    fi
fi

echo "启用 PID 作为 core 文件名的一部分..."
if [ -f "/proc/sys/kernel/core_uses_pid" ]; then
    echo 1 > /proc/sys/kernel/core_uses_pid 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "✓ core_uses_pid 已启用"
    else
        echo "✗ 无法启用 core_uses_pid，可能需要 root 权限"
    fi
fi

echo "core 文件配置完成\n"