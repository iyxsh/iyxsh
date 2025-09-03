#!/bin/bash

# 设置颜色变量
green="\033[0;32m"
red="\033[0;31m"
yellow="\033[0;33m"
reset="\033[0m"

# 定义日志函数
log() {
    local level=$1
    local message=$2
    local timestamp
    
    # 兼容各种Unix-like环境的日期格式
    if date_cmd_result=$(date +"%Y-%m-%d %H:%M:%S" 2>&1); then
        # 标准格式 (Linux, Git Bash)
        timestamp="$date_cmd_result"
    elif date_cmd_result=$(date -j +"%Y-%m-%d %H:%M:%S" 2>&1); then
        # BSD格式 (macOS, FreeBSD)
        timestamp="$date_cmd_result"
    elif date_cmd_result=$(date '+%Y-%m-%d %H:%M:%S' 2>&1); then
        # 带引号的格式 (某些特殊环境)
        timestamp="$date_cmd_result"
    else
        # 终极回退方案：使用纪元时间戳确保唯一性
        timestamp="$(date +%s)"
    fi
    
    case $level in
        "info")
            echo -e "$timestamp ${green}[INFO]${reset} $message"
            ;;
        "error")
            echo -e "$timestamp ${red}[ERROR]${reset} $message"
            ;;
        "warning")
            echo -e "$timestamp ${yellow}[WARNING]${reset} $message"
            ;;
        *)
            echo -e "$timestamp [DEBUG] $message"
            ;;
    esac
}

# 检查命令是否存在
exists() {
    command -v "$1" > /dev/null 2>&1
}

# 创建打包目录
create_package_dir() {
    if [ -d "$PACKAGE_DIR" ]; then
        log warning "打包目录 $PACKAGE_DIR 已存在，将被清空"
        rm -rf "$PACKAGE_DIR"/*
    else
        mkdir -p "$PACKAGE_DIR"
        log info "已创建打包目录: $PACKAGE_DIR"
    fi
}

# 复制文件到打包目录
copy_files() {
    log info "开始复制文件..."
    
    # 创建必要的目录结构
    mkdir -p "$PACKAGE_DIR/bin" "$PACKAGE_DIR/deploy" "$PACKAGE_DIR/data/default"
    
    # 复制bin目录下的指定文件
    local bin_files=("config.json" "start.sh" "stop.sh")
    for file in "${bin_files[@]}"; do
        if [ -f "$BIN_DIR/$file" ]; then
            cp "$BIN_DIR/$file" "$PACKAGE_DIR/bin/"
            log info "已复制: $BIN_DIR/$file -> $PACKAGE_DIR/bin/"
        else
            log warning "文件不存在，跳过: $BIN_DIR/$file"
        fi
    done

    # 复制data目录下的所有内容
    if [ -d "$DATA_DIR" ]; then
        cp -r "$DATA_DIR"/default/default.ods "$PACKAGE_DIR/data/default/"
        log info "已复制: $DATA_DIR/default/default.ods -> $PACKAGE_DIR/data/default/"
    else
        log error "目录不存在: $DATA_DIR"
        return 1
    fi
    
    # 复制deploy目录下的所有内容
    if [ -d "$DEPLOY_DIR" ]; then
        cp -r "$DEPLOY_DIR"/* "$PACKAGE_DIR/deploy/"
        log info "已复制: $DEPLOY_DIR/* -> $PACKAGE_DIR/deploy/"
    else
        log error "目录不存在: $DEPLOY_DIR"
        return 1
    fi
    
    # 尝试复制build目录下的libreofficefun可执行文件
    if [ -f "$BUILD_DIR/libreofficefun" ]; then
        cp "$BUILD_DIR/libreofficefun" "$PACKAGE_DIR/bin/"
        log info "已复制: $BUILD_DIR/libreofficefun -> $PACKAGE_DIR/bin/"
        chmod +x "$PACKAGE_DIR/bin/libreofficefun"
        log info "已设置执行权限: $PACKAGE_DIR/bin/libreofficefun"
    else
        log warning "libreofficefun可执行文件不存在于$BUILD_DIR目录，跳过复制。编译后再运行此脚本可包含该文件。"
    fi
    
    return 0
}

# 创建压缩包
create_archive() {
    # 创建一个简单的时间戳，兼容各种环境
    # 使用当前时间的秒数作为基础，确保唯一性
    local timestamp
    local date_cmd_result
    local date_error
    
    # 尝试多种date命令格式，使用最兼容的方式
    if date_cmd_result=$(date +%Y%m%d_%H%M%S 2>&1); then
        # 标准格式 (Linux, Git Bash)
        timestamp="$date_cmd_result"
    elif date_cmd_result=$(date -j +%Y%m%d_%H%M%S 2>&1); then
        # BSD格式 (macOS, FreeBSD)
        timestamp="$date_cmd_result"
    elif date_cmd_result=$(date '+%Y%m%d_%H%M%S' 2>&1); then
        # 带引号的格式 (某些特殊环境)
        timestamp="$date_cmd_result"
    else
        # 终极回退方案：使用纪元时间戳确保唯一性
        log warning "无法使用标准date命令格式化时间，使用纪元时间戳作为回退"
        if date_cmd_result=$(date +%s 2>&1); then
            timestamp="$date_cmd_result"
        else
            # 最保险的回退方案：使用固定字符串加随机数
            timestamp="fallback_$(tr -dc 'a-zA-Z0-9' < /dev/urandom | head -c 8)"
            log warning "纪元时间戳命令也失败，使用随机字符串作为最终回退: $timestamp"
        fi
    fi
    
    local archive_name="libreofficefun_package_${timestamp}.tar.gz"
    local archive_path="$OUTPUT_DIR/$archive_name"
    
    log info "开始创建压缩包: $archive_path"
    
    cd "$PACKAGE_DIR/.." && tar -czf "$archive_path" "$(basename "$PACKAGE_DIR")"
    
    if [ $? -eq 0 ]; then
        log info "压缩包创建成功: $archive_path"
        log info "压缩包大小: $(du -h "$archive_path" | cut -f1)"
        return 0
    else
        log error "压缩包创建失败"
        return 1
    fi
}

# 清理临时目录
cleanup() {
    if [ -d "$PACKAGE_DIR" ]; then
        rm -rf "$PACKAGE_DIR"
        log info "已清理临时打包目录: $PACKAGE_DIR"
    fi
}

# 主函数
main() {
    # 定义路径
    local PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local BIN_DIR="$PROJECT_DIR/bin"
    local DATA_DIR="$PROJECT_DIR/data"
    local DEPLOY_DIR="$PROJECT_DIR/deploy"
    local BUILD_DIR="$PROJECT_DIR/build"
    local OUTPUT_DIR="$PROJECT_DIR"
    local PACKAGE_DIR="$PROJECT_DIR/package_temp"
    
    log info "LibreofficeFun 打包脚本"
    log info "项目目录: $PROJECT_DIR"
    
    # 检查必要命令
    if ! exists tar; then
        log error "tar命令不存在，请安装tar工具"
        exit 1
    fi
    
    # 创建打包目录
    create_package_dir || exit 1
    
    # 复制文件
    if ! copy_files; then
        log error "文件复制失败，退出打包流程"
        cleanup
        exit 1
    fi
    
    # 创建压缩包
    if ! create_archive; then
        log error "压缩包创建失败，退出打包流程"
        cleanup
        exit 1
    fi
    
    # 清理临时目录
    cleanup
    
    log info "打包流程完成！"
    return 0
}

# 执行主函数
main