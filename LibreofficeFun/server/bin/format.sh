#!/bin/bash
# 获取脚本所在目录的绝对路径
script_dir=$(cd "$(dirname "$0")" && pwd)
echo "脚本目录: $script_dir"


# 指定需要转换的目录（当前目录）
TARGET_DIR="$script_dir/../../"

# 查找并转换文本文件（保持与readme.txt中的命令一致）
echo "开始转换文本文件格式..."
find "$TARGET_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "*.txt" -o -name "*.cpp" -o -name "*.sh" -o -name "*.json" -o -name "*.md" \) \
    -exec dos2unix --keep-bom {} \;

echo "文本文件格式转换完成！"

# 为所有.sh文件添加执行权限
echo "开始为.sh文件添加执行权限..."
find "$TARGET_DIR" -type f -name "*.sh" -exec chmod +x {} \;

echo "执行权限添加完成！"
echo "操作完成！已处理目录: $TARGET_DIR"
