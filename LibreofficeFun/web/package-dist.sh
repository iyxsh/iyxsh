#!/bin/bash

# 打包脚本：支持将生产版本(dist)或开发版本(dist-dev)的内容和Nginx配置文件打包

# 确保脚本在出错时停止执行
set -e

# 默认打包生产版本
build_type="prod"

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    key="$1"
    
    case $key in
        --dev)
            build_type="dev"
            shift
            ;;
        --prod)
            build_type="prod"
            shift
            ;;
        *)
            echo "未知参数: $key"
            echo "用法: $0 [--dev|--prod]"
            exit 1
            ;;
    esac
done

# 根据构建类型设置目录
if [ "$build_type" = "dev" ]; then
    DIST_DIR="./dist-dev"
    PACK_SUFFIX="dev"
else
    DIST_DIR="./dist"
    PACK_SUFFIX="prod"
fi

# 定义变量
NGINX_CONF_PATH="/usr/local/etc/nginx/nginx.conf"
PACK_OUTPUT_DIR="./pack_output"
PACK_TIMESTAMP=$(date '+%Y%m%d-%H%M%S')
PACK_FILENAME="libreofficefun-dist-with-nginx-$PACK_SUFFIX-$PACK_TIMESTAMP.tar.gz"
PACK_PATH="$PACK_OUTPUT_DIR/$PACK_FILENAME"
LOG_FILE="./package.log"

# 清理旧的打包日志
echo "" > $LOG_FILE

# 日志函数
log_message() {
    local message=$1
    local color=$2
    local no_newline=$3
    
    # 颜色代码
    local color_code=""
    case $color in
        "green") color_code="\033[32m" ;;
        "red") color_code="\033[31m" ;;
        "yellow") color_code="\033[33m" ;;
        "cyan") color_code="\033[36m" ;;
        *) color_code="\033[0m" ;;
    esac
    
    # 写入控制台
    if [ "$no_newline" = "1" ]; then
        echo -e -n "${color_code}${message}\033[0m"
    else
        echo -e "${color_code}${message}\033[0m"
    fi
    
    # 写入日志文件（移除颜色代码）
    local plain_message=$(echo "$message" | sed 's/\x1b\[[0-9;]*[a-zA-Z]//g')
    if [ "$no_newline" = "1" ]; then
        echo -n "$plain_message" >> $LOG_FILE
    else
        echo "$plain_message" >> $LOG_FILE
    fi
}

# 检查脚本执行权限
if [ ! -x "$0" ]; then
    log_message "警告: 脚本缺少执行权限，正在添加..." "yellow"
    chmod +x "$0"
    log_message "权限已添加，请重新运行脚本。" "green"
    exit 0
fi

# 确定要显示的版本名称
if [ "$build_type" = "dev" ]; then
    build_type_name="开发版本"
else
    build_type_name="生产版本"
fi

# 欢迎信息
log_message "=====================================================" "green"
log_message "     LibreOfficeFun 项目打包脚本" "green"
log_message "打包版本: $build_type_name" "green"
log_message "打包时间: $(date '+%Y-%m-%d %H:%M:%S')" "green"
log_message "=====================================================" "green"

# 检查dist目录是否存在
log_message "\n正在检查dist目录..." "cyan"
if [ ! -d "$DIST_DIR" ]; then
    log_message "错误: dist目录不存在。请先运行构建命令。" "red"
    exit 1
fi
log_message "已找到dist目录。" "green"

# 检查Nginx配置文件是否存在
log_message "\n正在检查Nginx配置文件..." "cyan"
nginx_conf_exists=0

if [ -f "$NGINX_CONF_PATH" ]; then
    nginx_conf_exists=1
    log_message "已找到Nginx配置文件: $NGINX_CONF_PATH" "green"
else
    log_message "警告: 未找到Nginx配置文件: $NGINX_CONF_PATH" "yellow"
fi

# 创建打包输出目录
log_message "\n正在创建打包输出目录..." "cyan"
if [ ! -d "$PACK_OUTPUT_DIR" ]; then
    mkdir -p "$PACK_OUTPUT_DIR"
    log_message "已创建打包输出目录: $PACK_OUTPUT_DIR" "green"
else
    log_message "打包输出目录已存在: $PACK_OUTPUT_DIR" "green"
fi

# 创建临时打包工作目录
TMP_PACK_DIR="$PACK_OUTPUT_DIR/tmp-pack-$PACK_TIMESTAMP"
if [ -d "$TMP_PACK_DIR" ]; then
    rm -rf "$TMP_PACK_DIR"
fi
mkdir -p "$TMP_PACK_DIR"

# 复制dist目录内容到临时工作目录
log_message "\n正在复制dist目录内容..." "cyan"
cp -r "$DIST_DIR/"* "$TMP_PACK_DIR/"
log_message "已复制dist目录内容。" "green"

# 复制Nginx配置文件到临时工作目录（如果存在）
if [ $nginx_conf_exists -eq 1 ]; then
    log_message "\n正在复制Nginx配置文件..." "cyan"
    cp "$NGINX_CONF_PATH" "$TMP_PACK_DIR/"
    log_message "已复制Nginx配置文件。" "green"
else
    # 检查项目根目录是否有nginx.conf
    LOCAL_NGINX_CONF="$(pwd)/nginx.conf"
    if [ -f "$LOCAL_NGINX_CONF" ]; then
        log_message "\n正在复制本地nginx.conf文件..." "cyan"
        cp "$LOCAL_NGINX_CONF" "$TMP_PACK_DIR/"
        log_message "已复制本地nginx.conf文件。" "green"
    fi
fi

# 创建打包说明文件
README_CONTENT="LibreOfficeFun 项目打包文件
==========================

打包时间: $(date '+%Y-%m-%d %H:%M:%S')

包含内容:
----------
1. 项目构建后的所有文件（dist目录内容）
2. Nginx配置文件（nginx.conf）

使用说明:
---------
1. 解压此压缩包
2. 将dist目录内容部署到Web服务器根目录
3. 根据需要调整nginx.conf配置并部署到Nginx配置目录
4. 重启Nginx服务使配置生效

注意事项:
---------
- 确保Nginx配置中的路径与实际部署路径一致
- 生产环境请配置合适的SSL证书
- 定期备份配置文件和数据"

README_PATH="$TMP_PACK_DIR/README_PACKAGE.txt"
echo "$README_CONTENT" > "$README_PATH"
log_message "\n已创建打包说明文件。" "green"

# 打包文件
log_message "\n正在打包文件..." "cyan"

# 使用tar命令打包
if tar -czf "$PACK_PATH" -C "$TMP_PACK_DIR" .; then
    log_message "打包成功！" "green"
    log_message "打包文件路径: $PACK_PATH" "green"
    log_message "打包文件大小: $(du -h "$PACK_PATH" | cut -f1)" "green"
else
    log_message "错误: 打包失败！" "red"
    exit 1
fi

# 清理临时工作目录
log_message "\n正在清理临时工作目录..." "cyan"
rm -rf "$TMP_PACK_DIR"
log_message "已清理临时工作目录。" "green"

# 完成提示
log_message "\n=====================================================" "green"
log_message "     打包脚本执行完成！" "green"
log_message "=====================================================" "green"
log_message "\n请将 $PACK_FILENAME 部署到目标服务器。" "yellow"