#!/bin/bash

# 生产环境部署脚本

# 确保脚本在出错时停止执行
set -e

# 定义变量
DEPLOY_DIR="./dist"
BACKUP_DIR="./dist-backup-$(date '+%Y%m%d-%H%M%S')"
LOG_FILE="./deploy.log"
NODE_VERSION="16.0.0"  # 推荐使用的Node.js版本
NPM_VERSION="8.0.0"    # 推荐使用的npm版本
KEEP_BACKUPS=5         # 保留的备份数量

# 清理旧的部署日志
echo "" > $LOG_FILE

# 欢迎信息
echo "=====================================================" | tee -a $LOG_FILE
echo -e "\033[32m     LibreOfficeFun 项目生产环境部署脚本\033[0m" | tee -a $LOG_FILE
echo "部署时间: $(date '+%Y-%m-%d %H:%M:%S')" | tee -a $LOG_FILE
echo "=====================================================" | tee -a $LOG_FILE

# 暂时跳过单元测试 (等待测试配置完善)
echo -e "\033[33m警告: 暂时跳过单元测试。请尽快完善测试配置。\033[0m" | tee -a $LOG_FILE

# 检查脚本执行权限
if [ ! -x "$0" ]; then
    echo -e "\033[33m警告: 脚本缺少执行权限，正在添加...\033[0m" | tee -a $LOG_FILE
    chmod +x "$0"
    echo -e "\033[32m权限已添加，请重新运行脚本。\033[0m" | tee -a $LOG_FILE
    exit 0
fi

# 版本比较函数
version_ge() {
    test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1" || test "$1" = "$2"
}

# 检查Node.js环境
echo -e "\033[36m正在检查Node.js环境...\033[0m" | tee -a $LOG_FILE
if command -v node &> /dev/null; then
    NODE_VERSION_INSTALLED=$(node -v | sed 's/v//')
    echo "当前Node.js版本: $NODE_VERSION_INSTALLED" | tee -a $LOG_FILE
    
    # 版本检查逻辑
    if version_ge "$NODE_VERSION_INSTALLED" "$NODE_VERSION"; then
        echo -e "\033[32mNode.js版本符合要求。\033[0m" | tee -a $LOG_FILE
    else
        echo -e "\033[33m警告: Node.js版本 $NODE_VERSION_INSTALLED 低于推荐版本 $NODE_VERSION，可能会导致兼容性问题。\033[0m" | tee -a $LOG_FILE
    fi
else
    echo -e "\033[31m错误: 未找到Node.js。请安装Node.js $NODE_VERSION或更高版本。\033[0m" | tee -a $LOG_FILE
    exit 1
fi

# 检查npm环境
echo -e "\033[36m正在检查npm环境...\033[0m" | tee -a $LOG_FILE
if command -v npm &> /dev/null; then
    NPM_VERSION=$(npm -v)
    echo "当前npm版本: $NPM_VERSION" | tee -a $LOG_FILE
    
    # npm版本检查
    if version_ge "$NPM_VERSION" "$NPM_VERSION"; then
        echo -e "\033[32mnpm版本符合要求。\033[0m" | tee -a $LOG_FILE
    else
        echo -e "\033[33m警告: npm版本低于推荐版本 $NPM_VERSION，建议更新。\033[0m" | tee -a $LOG_FILE
    fi
else
    echo -e "\033[31m错误: 未找到npm。\033[0m" | tee -a $LOG_FILE
    exit 1
fi

# 检查磁盘空间
echo -e "\033[36m正在检查磁盘空间...\033[0m" | tee -a $LOG_FILE
available_space=$(df -h . | awk 'NR==2 {print $4}' | sed 's/G//')
if (( $(echo "$available_space < 1" | bc -l) )); then
    echo -e "\033[33m警告: 当前磁盘空间不足1GB，可能会影响构建过程。\033[0m" | tee -a $LOG_FILE
fi

# 清理并备份旧的构建目录
if [ -d "$DEPLOY_DIR" ]; then
    echo -e "\033[36m正在备份旧的构建目录到 $BACKUP_DIR...\033[0m" | tee -a $LOG_FILE
    mv "$DEPLOY_DIR" "$BACKUP_DIR" | tee -a $LOG_FILE
    echo -e "\033[32m备份完成。\033[0m" | tee -a $LOG_FILE
    
    # 清理旧备份
    echo -e "\033[36m正在清理超过 $KEEP_BACKUPS 个的旧备份...\033[0m" | tee -a $LOG_FILE
    old_backups=$(ls -d ./dist-backup-* 2>/dev/null | sort | head -n -$KEEP_BACKUPS)
    if [ -n "$old_backups" ]; then
        echo "删除旧备份: $old_backups" | tee -a $LOG_FILE
        rm -rf $old_backups
    fi
fi

# 检查操作系统类型
echo -e "\033[36m正在检查操作系统类型...\033[0m" | tee -a $LOG_FILE
OS_TYPE="$(uname -s)"
echo "操作系统: $OS_TYPE" | tee -a $LOG_FILE

# 在Linux环境下强制清理node_modules和package-lock.json
if [[ "$OS_TYPE" == *Linux* ]]; then
    echo -e "\033[36m检测到Linux环境，正在清理node_modules目录和package-lock.json...\033[0m" | tee -a $LOG_FILE
    rm -rf node_modules
    rm -f package-lock.json
    echo -e "\033[32m清理完成！\033[0m" | tee -a $LOG_FILE
fi

# 安装依赖
echo -e "\033[36m正在安装项目依赖...\033[0m" | tee -a $LOG_FILE
# 使用npm install而不是npm ci --production，因为需要安装devDependencies中的构建工具
# 在Linux环境下使用--legacy-peer-deps解决潜在的依赖冲突
INSTALL_COMMAND="npm install"
if [[ "$OS_TYPE" == *Linux* ]]; then
    INSTALL_COMMAND="npm install --legacy-peer-deps"
    echo -e "\033[33m在Linux环境下使用--legacy-peer-deps解决依赖冲突...\033[0m" | tee -a $LOG_FILE
fi

if $INSTALL_COMMAND;
then
    echo -e "\033[32m依赖安装成功！\033[0m" | tee -a $LOG_FILE
else
    echo -e "\033[31m错误: 依赖安装失败。详细日志请查看 $LOG_FILE\033[0m" | tee -a $LOG_FILE
    exit 1
fi

# 构建项目
echo -e "\033[36m正在构建生产版本...\033[0m" | tee -a $LOG_FILE
export NODE_ENV="production"

# 支持自定义环境变量
export VITE_API_BASE_URL=${VITE_API_BASE_URL:-/api}
echo "使用API基础URL: $VITE_API_BASE_URL" | tee -a $LOG_FILE

# 在构建前创建备份
if [ -d "$DEPLOY_DIR" ]; then
    echo -e "\033[36m正在备份当前构建目录...\033[0m" | tee -a $LOG_FILE
    if [ -d "$BACKUP_DIR" ]; then
        rm -rf "$BACKUP_DIR"
    fi
    mv "$DEPLOY_DIR" "$BACKUP_DIR"
    echo -e "\033[32m备份完成，备份目录: $BACKUP_DIR\033[0m" | tee -a $LOG_FILE
fi

if npm run build:prod; then
    echo -e "\033[32m生产版本构建成功！\033[0m" | tee -a $LOG_FILE
else
    echo -e "\033[31m错误: 项目构建失败。详细日志请查看 $LOG_FILE\033[0m" | tee -a $LOG_FILE
    
    # 执行回滚
    if [ -d "$BACKUP_DIR" ]; then
        echo -e "\033[33m正在回滚到之前的版本...\033[0m" | tee -a $LOG_FILE
        if [ -d "$DEPLOY_DIR" ]; then
            rm -rf "$DEPLOY_DIR"
        fi
        mv "$BACKUP_DIR" "$DEPLOY_DIR"
        echo -e "\033[32m回滚完成！\033[0m" | tee -a $LOG_FILE
    fi
    exit 1
fi

# 验证构建结果
if [ ! -d "$DEPLOY_DIR" ]; then
    echo -e "\033[31m错误: 构建目录 $DEPLOY_DIR 不存在，构建可能失败。\033[0m" | tee -a $LOG_FILE
    exit 1
fi

# 检查关键文件是否存在
key_files=("index.html" "assets" "favicon.ico")
for file in "${key_files[@]}"; do
    if [ ! -e "$DEPLOY_DIR/$file" ]; then
        echo -e "\033[33m警告: 关键文件 $DEPLOY_DIR/$file 不存在，构建可能不完整。\033[0m" | tee -a $LOG_FILE
    fi
    done

# 显示构建成功信息
BUILD_FILES_COUNT=$(find "$DEPLOY_DIR" -type f | wc -l)
echo -e "\033[32m构建成功！生成了 $BUILD_FILES_COUNT 个文件。\033[0m" | tee -a $LOG_FILE

# 显示构建文件大小统计
TOTAL_SIZE=$(du -sh "$DEPLOY_DIR" | cut -f1)
echo -e "\033[32m构建产物总大小: $TOTAL_SIZE\033[0m" | tee -a $LOG_FILE

# 生成构建摘要
cat << EOF > "$DEPLOY_DIR/BUILD_INFO.txt"
构建信息
--------
项目: LibreOfficeFun
构建时间: $(date '+%Y-%m-%d %H:%M:%S')
Node.js版本: $(node -v)
npm版本: $(npm -v)
文件数量: $BUILD_FILES_COUNT
总大小: $TOTAL_SIZE
API基础URL: $VITE_API_BASE_URL
EOF

# 显示下一步操作建议
echo -e "\n\033[33m部署建议:\n\033[0m" | tee -a $LOG_FILE
echo "1. 使用以下命令预览构建结果:\n   npm run preview\n" | tee -a $LOG_FILE
echo "2. 将 $DEPLOY_DIR 目录中的所有文件部署到您的Web服务器根目录。\n" | tee -a $LOG_FILE
echo "3. 确保您的Web服务器正确配置了SPA路由（对于Vue Router的history模式）。\n" | tee -a $LOG_FILE
echo "   - Nginx配置示例:\n     location / {\n       try_files $uri $uri/ /index.html;\n     }" | tee -a $LOG_FILE
echo "4. 部署完成后，请参考DEPLOYMENT.md文档进行验证和故障排除。" | tee -a $LOG_FILE

echo "\n=====================================================" | tee -a $LOG_FILE
echo -e "\033[32m     部署脚本执行完成！\033[0m" | tee -a $LOG_FILE
echo "详细日志已保存至: $LOG_FILE" | tee -a $LOG_FILE
echo "=====================================================" | tee -a $LOG_FILE