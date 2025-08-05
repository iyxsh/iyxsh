#!/bin/sh
export URE_BOOTSTRAP=/usr/local/lib/libreoffice/program/fundamentalrc

soffice --headless --accept="socket,host=127.0.0.1,port=2002;urp;" &

# 启动主程序
echo "启动主程序..."
./libreofficefun &

echo "所有服务启动流程已完成。"