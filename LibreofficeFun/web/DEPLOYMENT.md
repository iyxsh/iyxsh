# LibreOfficeFun 项目部署指南

# LibreOfficeFun 项目部署指南

本文件提供了将 LibreOfficeFun 项目部署到生产环境的详细指南，包括自动化脚本使用、手动部署步骤、Web 服务器配置和常见问题解决方案。

## 系统要求

在部署前，请确保您的环境满足以下要求：

- **操作系统**：Windows 10/11、Linux (Ubuntu 20.04+/Debian 11+/CentOS 8+)、macOS 12+
- **Node.js**：v16.0.0 或更高版本
- **npm**：v8.0.0 或更高版本
- **Web 服务器**：推荐使用 Nginx、Apache 或 Caddy
- **磁盘空间**：至少 1GB 可用空间

## 部署脚本使用方法

项目根目录中提供了两个自动化部署脚本，分别适用于不同的操作系统：

### Windows（PowerShell）

1. 打开 PowerShell 终端（建议以管理员身份运行）
2. 导航到项目根目录
3. 运行以下命令：

```powershell
# 确保脚本具有执行权限
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process

# 运行部署脚本
.\deploy-production.ps1
```

### Linux/macOS（Bash）

1. 打开终端
2. 导航到项目根目录
3. 运行以下命令：

```bash
# 确保脚本具有执行权限（只需执行一次）
chmod +x deploy-production.sh

# 运行部署脚本
./deploy-production.sh
```

## 部署脚本功能说明

自动化部署脚本提供了以下完整功能：

1. **环境检查**：验证 Node.js 和 npm 是否已安装并满足版本要求
2. **磁盘空间检查**：确保有足够的可用空间进行构建
3. **日志记录**：记录部署过程中的详细信息到 `deploy.log` 文件
4. **依赖安装**：使用 `npm install` 安装所有依赖（包括构建工具）
5. **项目构建**：设置 `NODE_ENV=production` 并运行 `npm run build` 构建生产版本
6. **备份功能**：自动备份旧的构建目录到 `dist-backup-<timestamp>`，并保留最近5个备份
7. **构建验证**：检查构建是否成功，验证关键文件是否存在
8. **构建摘要**：生成 `BUILD_INFO.txt` 文件记录构建信息
9. **环境变量支持**：可自定义 `VITE_API_BASE_URL` 等环境变量
10. **部署建议**：提供详细的下一步操作指导

## 手动部署步骤

如果您希望手动部署项目，可以按照以下步骤操作：

1. **安装依赖**：

```bash
npm ci --production
```

2. **构建项目**：

```bash
# 设置环境变量（可选）
export VITE_API_BASE_URL=https://api.yourdomain.com

# 构建项目
NODE_ENV=production npm run build
```

3. **验证构建**：确保 `dist` 目录已创建，且包含 `index.html` 等关键文件

4. **预览构建结果**：在部署到生产环境之前，您可以使用以下命令在本地预览构建结果：

```bash
npm run preview
```

这将在 http://localhost:4173 启动一个本地服务器。

5. **部署到 Web 服务器**：将 `dist` 目录中的所有文件复制到您的 Web 服务器根目录

## Web 服务器配置

### Nginx 配置示例

如果您使用 Nginx 作为 Web 服务器，请参考以下配置示例：

```nginx
server {
    listen 80;
    server_name example.com;
    
    # 静态文件配置
    location / {
        root /path/to/your/dist/folder;
        index index.html;
        try_files $uri $uri/ /index.html;  # 重要：支持 SPA 路由
    }
    
    # API 代理配置（如果需要）
    location /api {
        proxy_pass http://backend-server:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
    
    # 静态文件缓存设置
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg)$ {
        expires 30d;
        add_header Cache-Control "public, max-age=2592000";
    }
    
    # 压缩配置
    gzip on;
    gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;
    gzip_comp_level 6;
    gzip_proxied any;
    gzip_vary on;
}
```

配置完成后，运行以下命令使配置生效：

```bash
sudo nginx -t  # 测试配置

sudo systemctl reload nginx  # 重新加载 Nginx
```

### Apache 配置示例

如果您使用 Apache 作为 Web 服务器，请参考以下配置示例：

```apache
<VirtualHost *:80>
    ServerName example.com
    
    DocumentRoot /path/to/your/dist/folder
    
    <Directory /path/to/your/dist/folder>
        Options Indexes FollowSymLinks
        AllowOverride All
        Require all granted
        
        # 支持 SPA 路由
        RewriteEngine On
        RewriteBase /
        RewriteRule ^index\.html$ - [L]
        RewriteCond %{REQUEST_FILENAME} !-f
        RewriteCond %{REQUEST_FILENAME} !-d
        RewriteRule . /index.html [L]
    </Directory>
    
    # API 代理配置（如果需要）
    ProxyPass /api http://backend-server:8080/api
    ProxyPassReverse /api http://backend-server:8080/api
    
    # 静态文件缓存设置
    <FilesMatch "\.(js|css|png|jpg|jpeg|gif|ico|svg)$">
        Header set Cache-Control "max-age=2592000, public"
    </FilesMatch>
    
    # 压缩配置
    SetOutputFilter DEFLATE
    AddOutputFilterByType DEFLATE text/html text/plain text/xml text/css text/javascript application/javascript application/json
</VirtualHost>
```

配置完成后，运行以下命令使配置生效：

```bash
sudo a2enmod rewrite proxy proxy_http headers deflate  # 启用必要的模块

sudo systemctl reload apache2  # 重新加载 Apache
```

## 环境变量配置

在构建和部署过程中，您可以通过以下环境变量来自定义项目行为：

| 环境变量 | 说明 | 默认值 |
|----------|------|--------|
| NODE_ENV | 运行环境 | `production` |
| VITE_API_BASE_URL | API 基础 URL | `/api` |

在运行部署脚本前，您可以设置这些环境变量：

**Windows (PowerShell):**
```powershell
$env:VITE_API_BASE_URL = "https://api.yourdomain.com"
```

**Linux/macOS (Bash):**
```bash
export VITE_API_BASE_URL="https://api.yourdomain.com"
```

## 部署后验证

部署完成后，您可以通过以下步骤验证部署是否成功：

1. **访问网站**：在浏览器中访问您的域名或服务器 IP 地址
2. **检查功能**：验证网站的所有功能是否正常工作
3. **检查 API 连接**：确保前端能够正常连接到后端 API
4. **检查响应时间**：使用浏览器的开发者工具检查页面加载时间和资源大小
5. **查看构建信息**：检查 `dist/BUILD_INFO.txt` 文件，确认构建配置

## 常见问题解决

### 1. 页面刷新后出现 404 错误

这通常是因为 SPA 路由配置不正确。请确保您的 Web 服务器已正确配置为将所有未知路径重定向到 `index.html`。

### 2. API 连接失败

- 检查 `VITE_API_BASE_URL` 环境变量是否正确设置
- 确认后端服务是否正在运行
- 检查网络连接和防火墙设置
- 验证跨域资源共享（CORS）配置是否正确
- 查看浏览器控制台中的网络请求错误信息

### 3. 静态资源加载缓慢

- 考虑使用 CDN 加速静态资源分发
- 优化 Nginx 或 Apache 的缓存配置
- 检查服务器的网络带宽和性能
- 确保启用了 Gzip/Brotli 压缩

### 4. 内存使用过高

- 确保使用了生产构建（`NODE_ENV=production`）
- 优化前端代码，减少不必要的依赖
- 考虑升级服务器内存或使用负载均衡
- 检查是否有内存泄漏

### 5. SSL 证书问题

如果遇到 SSL 证书验证问题（如 `ERR_CERT_AUTHORITY_INVALID`），请确保：
- 使用由受信任的证书颁发机构签发的证书
- 证书未过期
- 证书的主题与域名匹配
- 证书链完整

在开发环境中，可以临时修改 API 配置来禁用证书验证，但在生产环境中必须使用有效的 SSL 证书。

## 更新部署

当项目有新版本需要部署时，请按照以下步骤操作：

1. **拉取最新代码**：

```bash
git pull origin main  # 或您使用的分支名称
```

2. **解决冲突**（如果有）：

```bash
git status  # 查看冲突文件
git diff    # 查看冲突内容
# 手动编辑文件解决冲突
git add .   # 标记冲突已解决
git commit  # 提交解决冲突的更改
```

3. **运行部署脚本**：

```bash
# Windows
./deploy-production.ps1

# Linux/macOS
./deploy-production.sh
```

4. **验证更新**：检查网站是否正常运行，功能是否完整

5. **查看部署日志**：如果遇到问题，查看 `deploy.log` 文件获取详细信息

## 回滚部署

如果新版本部署出现问题，可以使用备份回滚到之前的版本：

1. **停止当前服务**（可选）

2. **删除当前构建**：

```bash
# Windows
Remove-Item -Path ./dist -Recurse -Force

# Linux/macOS
rm -rf ./dist
```

3. **恢复备份**：

```bash
# Windows (假设备份文件名为 dist-backup-20231010-123456)
Move-Item -Path ./dist-backup-20231010-123456 -Destination ./dist

# Linux/macOS
mv ./dist-backup-20231010-123456 ./dist
```

4. **重启服务**（如果之前停止了）

## 高级部署建议

1. **自动化部署**：考虑使用 CI/CD 工具（如 Jenkins、GitHub Actions、GitLab CI）自动执行部署流程

2. **容器化部署**：使用 Docker 容器化应用，并通过 Docker Compose 管理多容器应用

3. **负载均衡**：对于高流量网站，配置多个服务器和负载均衡器提高可用性

4. **监控与告警**：设置网站监控和告警系统，及时发现并解决问题

5. **性能优化**：
   - 使用 CDN 分发静态资源
   - 实现 HTTP/2 或 HTTP/3 支持
   - 配置适当的缓存策略

6. **安全措施**：
   - 启用 HTTPS 并配置 HSTS
   - 实施内容安全策略（CSP）
   - 定期更新依赖包，修复已知漏洞

7. **版本控制**：在部署过程中始终保持代码版本控制，便于追踪和回滚

请根据您的实际需求和环境调整这些配置和建议。

---

希望本指南能帮助您顺利部署 LibreOfficeFun 项目！如有任何部署问题，请联系项目维护团队。