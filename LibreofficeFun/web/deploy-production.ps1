# 生产环境部署脚本

# 确保脚本在出错时停止执行
$ErrorActionPreference = "Stop"

# 定义变量
$DEPLOY_DIR = "./dist"
$BACKUP_DIR = "./dist-backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
$LOG_FILE = "./deploy.log"
$NODE_VERSION = "16.0.0"  # 推荐使用的Node.js版本
$NPM_VERSION = "8.0.0"    # 推荐使用的npm版本
$KEEP_BACKUPS = 5         # 保留的备份数量

# 清理旧的部署日志
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
    
    # 写入日志文件（移除颜色代码）
    $plainMessage = [regex]::Replace($Message, '\e\[[0-9;]*[a-zA-Z]', '')
    if ($NoNewLine) {
        $plainMessage | Out-File -FilePath $LOG_FILE -Append -Encoding utf8 -NoNewline
    } else {
        $plainMessage | Out-File -FilePath $LOG_FILE -Append -Encoding utf8
    }
}

# 欢迎信息
Log-Message "=====================================================" -Color Green
Log-Message "     LibreOfficeFun 项目生产环境部署脚本" -Color Green
Log-Message "部署时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -Color Green
Log-Message "=====================================================" -Color Green

# 暂时跳过单元测试 (等待测试配置完善)
Log-Message "警告: 暂时跳过单元测试。请尽快完善测试配置。" -Color Yellow

# 版本比较函数
function Compare-Versions {
    param (
        [string]$Version1,
        [string]$Version2
    )
    
    $v1Parts = $Version1 -split '\.' | ForEach-Object { [int]$_ }
    $v2Parts = $Version2 -split '\.' | ForEach-Object { [int]$_ }
    
    $maxLength = [Math]::Max($v1Parts.Length, $v2Parts.Length)
    for ($i = 0; $i -lt $maxLength; $i++) {
        $v1 = if ($i -lt $v1Parts.Length) { $v1Parts[$i] } else { 0 }
        $v2 = if ($i -lt $v2Parts.Length) { $v2Parts[$i] } else { 0 }
        
        if ($v1 -gt $v2) { return 1 }
        if ($v1 -lt $v2) { return -1 }
    }
    
    return 0
}

# 检查Node.js环境
Log-Message "正在检查Node.js环境..." -Color Cyan
Try {
    $nodeVersion = (node -v).Replace('v', '')
    Log-Message "当前Node.js版本: $nodeVersion" -Color Cyan
    
    # 版本检查逻辑
    $compareResult = Compare-Versions -Version1 $nodeVersion -Version2 $NODE_VERSION
    if ($compareResult -ge 0) {
        Log-Message "Node.js版本符合要求。" -Color Green
    } else {
        Log-Message "警告: Node.js版本 $nodeVersion 低于推荐版本 $NODE_VERSION，可能会导致兼容性问题。" -Color Yellow
    }
} Catch {
    Log-Message "错误: 未找到Node.js。请安装Node.js $NODE_VERSION或更高版本。" -Color Red
    Exit 1
}

# 检查npm环境
Log-Message "正在检查npm环境..." -Color Cyan
Try {
    $npmVersion = npm -v
    Log-Message "当前npm版本: $npmVersion" -Color Cyan
    
    # npm版本检查
    $compareResult = Compare-Versions -Version1 $npmVersion -Version2 $NPM_VERSION
    if ($compareResult -ge 0) {
        Log-Message "npm版本符合要求。" -Color Green
    } else {
        Log-Message "警告: npm版本低于推荐版本 $NPM_VERSION，建议更新。" -Color Yellow
    }
} Catch {
    Log-Message "错误: 未找到npm。" -Color Red
    Exit 1
}

# 检查磁盘空间
Log-Message "正在检查磁盘空间..." -Color Cyan
Try {
    $diskInfo = Get-WmiObject -Class Win32_LogicalDisk -Filter "DeviceID='$((Get-Location).Drive.Name)'"
    $availableSpaceGB = [math]::Round($diskInfo.FreeSpace / 1GB, 2)
    Log-Message "当前磁盘可用空间: $availableSpaceGB GB" -Color Cyan
    
    if ($availableSpaceGB -lt 1) {
        Log-Message "警告: 当前磁盘空间不足1GB，可能会影响构建过程。" -Color Yellow
    }
} Catch {
    Log-Message "警告: 无法检查磁盘空间。" -Color Yellow
}

# 清理并备份旧的构建目录
if (Test-Path $DEPLOY_DIR) {
    Log-Message "正在备份旧的构建目录到 $BACKUP_DIR..." -Color Cyan
    Move-Item -Path $DEPLOY_DIR -Destination $BACKUP_DIR
    Log-Message "备份完成。" -Color Green
    
    # 清理旧备份
    Log-Message "正在清理超过 $KEEP_BACKUPS 个的旧备份..." -Color Cyan
    $oldBackups = Get-ChildItem -Path ./dist-backup-* -Directory | Sort-Object CreationTime -Descending | Select-Object -Skip $KEEP_BACKUPS
    if ($oldBackups.Count -gt 0) {
        Log-Message "删除旧备份: $($oldBackups.Name -join ', ')" -Color Cyan
        $oldBackups | Remove-Item -Recurse -Force
    }
}

# 安装依赖
Log-Message "正在安装项目依赖..." -Color Cyan
Try {
    # 使用npm install而不是npm ci --production，因为需要安装devDependencies中的构建工具
    npm install
    if ($LASTEXITCODE -ne 0) {
        throw "npm install failed"
    }
    Log-Message "依赖安装成功！" -Color Green
} Catch {
    Log-Message "错误: 依赖安装失败。详细日志请查看 $LOG_FILE" -Color Red
    Exit 1
}

# 构建项目
Log-Message "正在构建生产版本..." -Color Cyan
$env:NODE_ENV = "production"

# 支持自定义环境变量
if (-not $env:VITE_API_BASE_URL) {
    $env:VITE_API_BASE_URL = '/api'
}
Log-Message "使用API基础URL: $env:VITE_API_BASE_URL" -Color Cyan

# 在构建前创建备份
if (Test-Path $DEPLOY_DIR) {
    Log-Message "正在备份当前构建目录..." -Color Cyan
    if (Test-Path $BACKUP_DIR) {
        Remove-Item $BACKUP_DIR -Recurse -Force
    }
    Move-Item $DEPLOY_DIR $BACKUP_DIR
    Log-Message "备份完成，备份目录: $BACKUP_DIR" -Color Green
}

Try {
    npm run build:prod
    if ($LASTEXITCODE -ne 0) {
        throw "npm build:prod failed"
    }
    Log-Message "生产版本构建成功！" -Color Green
} Catch {
    Log-Message "错误: 项目构建失败。详细日志请查看 $LOG_FILE" -Color Red
    
    # 执行回滚
    if (Test-Path $BACKUP_DIR) {
        Log-Message "正在回滚到之前的版本..." -Color Yellow
        if (Test-Path $DEPLOY_DIR) {
            Remove-Item $DEPLOY_DIR -Recurse -Force
        }
        Move-Item $BACKUP_DIR $DEPLOY_DIR
        Log-Message "回滚完成！" -Color Green
    }
    Exit 1
}

# 验证构建结果
if (!(Test-Path $DEPLOY_DIR)) {
    Log-Message "错误: 构建目录 $DEPLOY_DIR 不存在，构建可能失败。" -Color Red
    Exit 1
}

# 检查关键文件是否存在
$keyFiles = @("index.html", "assets", "favicon.ico")
foreach ($file in $keyFiles) {
    if (!(Test-Path "$DEPLOY_DIR\$file")) {
        Log-Message "警告: 关键文件 $DEPLOY_DIR\$file 不存在，构建可能不完整。" -Color Yellow
    }
}

# 显示构建成功信息
$buildFiles = Get-ChildItem -Path $DEPLOY_DIR -Recurse | Where-Object { !$_.PSIsContainer }
Log-Message "构建成功！生成了 $($buildFiles.Count) 个文件。" -Color Green

# 显示构建文件大小统计
$totalSize = ($buildFiles | Measure-Object -Property Length -Sum).Sum / 1MB
Log-Message "构建产物总大小: $([math]::Round($totalSize, 2)) MB" -Color Green

# 生成构建摘要
$buildInfoContent = @"
构建信息
--------
项目: LibreOfficeFun
构建时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Node.js版本: $(node -v)
npm版本: $(npm -v)
文件数量: $($buildFiles.Count)
总大小: $([math]::Round($totalSize, 2)) MB
API基础URL: $env:VITE_API_BASE_URL
"@

$buildInfoContent | Out-File -FilePath "$DEPLOY_DIR\BUILD_INFO.txt" -Encoding utf8

# 显示下一步操作建议
Log-Message "\n部署建议:\n" -Color Yellow
Log-Message "1. 使用以下命令预览构建结果:\n   npm run preview\n"
Log-Message "2. 将 $DEPLOY_DIR 目录中的所有文件部署到您的Web服务器根目录。\n"
Log-Message "3. 确保您的Web服务器正确配置了SPA路由（对于Vue Router的history模式）。\n"
Log-Message "   - Nginx配置示例:\n     location / {\n       try_files $uri $uri/ /index.html;\n     }"
Log-Message "4. 部署完成后，请参考DEPLOYMENT.md文档进行验证和故障排除。"

Log-Message "\n=====================================================" -Color Green
Log-Message "     部署脚本执行完成！" -Color Green
Log-Message "详细日志已保存至: $LOG_FILE" -Color Green
Log-Message "=====================================================" -Color Green