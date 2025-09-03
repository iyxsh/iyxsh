# 打包脚本：支持将生产版本(dist)或开发版本(dist-dev)的内容和Nginx配置文件打包

# 确保脚本在出错时停止执行
$ErrorActionPreference = "Stop"

# 默认打包生产版本
$buildType = "prod"

# 解析命令行参数
for ($i = 0; $i -lt $args.Length; $i++) {
    switch ($args[$i]) {
        "--dev" {
            $buildType = "dev"
        }
        "--prod" {
            $buildType = "prod"
        }
        default {
            Write-Host "未知参数: $($args[$i])"
            Write-Host "用法: $($MyInvocation.MyCommand.Name) [--dev|--prod]"
            Exit 1
        }
    }
}

# 根据构建类型设置目录
if ($buildType -eq "dev") {
    $DIST_DIR = "./dist-dev"
    $packSuffix = "dev"
} else {
    $DIST_DIR = "./dist"
    $packSuffix = "prod"
}

# 定义变量
$NGINX_CONF_PATH = "/usr/local/etc/nginx/nginx.conf"
$PACK_OUTPUT_DIR = "./pack_output"
$PACK_TIMESTAMP = Get-Date -Format 'yyyyMMdd-HHmmss'
$PACK_FILENAME = "libreofficefun-dist-with-nginx-$packSuffix-$PACK_TIMESTAMP.zip"
$PACK_PATH = Join-Path -Path $PACK_OUTPUT_DIR -ChildPath $PACK_FILENAME
$LOG_FILE = "./package.log"

# 清理旧的打包日志
"" | Out-File -FilePath $LOG_FILE -Encoding utf8

# 日志函数
function Log-Message {
    param (
        [string]$Message,
        [System.ConsoleColor]$Color = [System.ConsoleColor]::White,
        [switch]$NoNewLine
    )
    
    # 写入控制台
    if ($NoNewLine) {
        Write-Host -ForegroundColor $Color $Message -NoNewLine
    } else {
        Write-Host -ForegroundColor $Color $Message
    }
    
    # 写入日志文件
    $plainMessage = $Message
    if ($NoNewLine) {
        $plainMessage | Out-File -FilePath $LOG_FILE -Append -Encoding utf8 -NoNewline
    } else {
        $plainMessage | Out-File -FilePath $LOG_FILE -Append -Encoding utf8
    }
}

# 确定要显示的版本名称
if ($buildType -eq "dev") {
    $buildTypeName = "开发版本"
} else {
    $buildTypeName = "生产版本"
}

# 欢迎信息
Log-Message "=====================================================" -Color Green
Log-Message "     LibreOfficeFun 项目打包脚本" -Color Green
Log-Message "打包版本: $buildTypeName" -Color Green
Log-Message "打包时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -Color Green
Log-Message "=====================================================" -Color Green

# 检查dist目录是否存在
Log-Message "\n正在检查dist目录..." -Color Cyan
if (-not (Test-Path -Path $DIST_DIR -PathType Container)) {
    Log-Message "错误: dist目录不存在。请先运行构建命令。" -Color Red
    exit 1
}
Log-Message "已找到dist目录。" -Color Green

# 检查Nginx配置文件是否存在（仅在Linux/macOS环境下有效）
Log-Message "\n正在检查Nginx配置文件..." -Color Cyan
$nginxConfExists = $false

# 如果是在Linux/macOS环境下运行PowerShell Core
if ($IsLinux -or $IsMacOS) {
    if (Test-Path -Path $NGINX_CONF_PATH -PathType Leaf) {
        $nginxConfExists = $true
        Log-Message "已找到Nginx配置文件: $NGINX_CONF_PATH" -Color Green
    } else {
        Log-Message "警告: 未找到Nginx配置文件: $NGINX_CONF_PATH" -Color Yellow
    }
} else {
    # Windows环境下提示用户可能需要手动复制Nginx配置
    Log-Message "注意: 在Windows环境下无法直接访问Linux路径 $NGINX_CONF_PATH" -Color Yellow
    Log-Message "      请手动准备Nginx配置文件。" -Color Yellow
}

# 创建打包输出目录
Log-Message "\n正在创建打包输出目录..." -Color Cyan
if (-not (Test-Path -Path $PACK_OUTPUT_DIR -PathType Container)) {
    New-Item -Path $PACK_OUTPUT_DIR -ItemType Directory | Out-Null
    Log-Message "已创建打包输出目录: $PACK_OUTPUT_DIR" -Color Green
} else {
    Log-Message "打包输出目录已存在: $PACK_OUTPUT_DIR" -Color Green
}

# 创建临时打包工作目录
$TMP_PACK_DIR = Join-Path -Path $PACK_OUTPUT_DIR -ChildPath "tmp-pack-$PACK_TIMESTAMP"
if (Test-Path -Path $TMP_PACK_DIR -PathType Container) {
    Remove-Item -Path $TMP_PACK_DIR -Recurse -Force
}
New-Item -Path $TMP_PACK_DIR -ItemType Directory | Out-Null

# 复制dist目录内容到临时工作目录
Log-Message "\n正在复制dist目录内容..." -Color Cyan
Copy-Item -Path "$DIST_DIR\*" -Destination $TMP_PACK_DIR -Recurse -Force
Log-Message "已复制dist目录内容。" -Color Green

# 复制Nginx配置文件到临时工作目录（如果存在）
if ($nginxConfExists) {
    Log-Message "\n正在复制Nginx配置文件..." -Color Cyan
    Copy-Item -Path $NGINX_CONF_PATH -Destination $TMP_PACK_DIR -Force
    Log-Message "已复制Nginx配置文件。" -Color Green
} else {
    # 检查项目根目录是否有nginx.conf
    $LOCAL_NGINX_CONF = Join-Path -Path $PSScriptRoot -ChildPath "nginx.conf"
    if (Test-Path -Path $LOCAL_NGINX_CONF -PathType Leaf) {
        Log-Message "\n正在复制本地nginx.conf文件..." -Color Cyan
        Copy-Item -Path $LOCAL_NGINX_CONF -Destination $TMP_PACK_DIR -Force
        Log-Message "已复制本地nginx.conf文件。" -Color Green
    }
}

# 创建打包说明文件
$README_CONTENT = @"
LibreOfficeFun 项目打包文件
==========================

打包时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

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
- 定期备份配置文件和数据
"@

$README_PATH = Join-Path -Path $TMP_PACK_DIR -ChildPath "README_PACKAGE.txt"
$README_CONTENT | Out-File -FilePath $README_PATH -Encoding utf8
Log-Message "\n已创建打包说明文件。" -Color Green

# 打包文件
Log-Message "\n正在打包文件..." -Color Cyan

# 使用PowerShell的Compress-Archive命令打包
try {
    Compress-Archive -Path "$TMP_PACK_DIR\*" -DestinationPath $PACK_PATH -Force
    Log-Message "打包成功！" -Color Green
    Log-Message "打包文件路径: $PACK_PATH" -Color Green
    Log-Message "打包文件大小: $(Get-Item $PACK_PATH).Length/1MB MB" -Color Green
} catch {
    Log-Message "错误: 打包失败: $_" -Color Red
    exit 1
}

# 清理临时工作目录
Log-Message "\n正在清理临时工作目录..." -Color Cyan
Remove-Item -Path $TMP_PACK_DIR -Recurse -Force
Log-Message "已清理临时工作目录。" -Color Green

# 完成提示
Log-Message "\n=====================================================" -Color Green
Log-Message "     打包脚本执行完成！" -Color Green
Log-Message "=====================================================" -Color Green
Log-Message "\n请将 $PACK_FILENAME 部署到目标服务器。" -Color Yellow