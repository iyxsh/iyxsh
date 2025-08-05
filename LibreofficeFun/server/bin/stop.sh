#!/bin/sh

echo "正在停止服务..."

# 停止 libreofficefun 主程序
echo "停止 libreofficefun 主程序..."
pkill -f libreofficefun 2>/dev/null

# 停止 LibreOffice 服务
echo "停止 LibreOffice 服务..."
pkill -f "soffice.*headless.*port=2002" 2>/dev/null

# 等待一段时间确保进程完全退出
sleep 2

# 检查是否还有相关进程在运行
echo "检查剩余相关进程..."
if pgrep -f libreofficefun > /dev/null 2>&1; then
    echo "警告: libreofficefun 进程可能仍在运行"
else
    echo "✓ libreofficefun 主程序已停止"
fi

if pgrep -f "soffice.*headless.*port=2002" > /dev/null 2>&1; then
    echo "警告: LibreOffice 服务进程可能仍在运行"
else
    echo "✓ LibreOffice 服务已停止"
fi

echo "服务停止流程已完成。"