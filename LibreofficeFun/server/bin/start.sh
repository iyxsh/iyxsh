#!/bin/sh
sh stop.sh # 重新执行停止流程，确保完全退出
export URE_BOOTSTRAP=file:///usr/local/lib/libreoffice/program/fundamentalrc
soffice --headless --accept="socket,host=127.0.0.1,port=2002;urp;" &
echo "启动主程序..."
./libreofficefun &

echo "所有服务启动流程已完成。"
