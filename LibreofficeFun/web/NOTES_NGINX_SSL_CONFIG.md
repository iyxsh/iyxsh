# Nginx配置示例（支持SSL和后端API）

根据您的需求，以下是一个完整的Nginx配置示例，配置了SSL认证并将API请求转发到端口8443的后端服务。

## 完整配置示例

```nginx
server {
    listen 80;
    server_name example.com;  # 替换为您的域名
    return 301 https://$server_name$request_uri;  # 强制重定向到HTTPS
}

server {
    listen 443 ssl;
    server_name example.com;  # 替换为您的域名
    
    # SSL证书配置
    ssl_certificate /path/to/your/certificate.crt;  # 替换为您的SSL证书路径
    ssl_certificate_key /path/to/your/private.key;  # 替换为您的私钥路径
    
    # SSL优化配置
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_prefer_server_ciphers off;
    ssl_session_timeout 1d;
    ssl_session_cache shared:SSL:10m;
    ssl_session_tickets off;
    
    # 静态文件配置
    location / {
        root /path/to/your/dist/folder;  # 替换为项目dist目录路径
        index index.html;
        try_files $uri $uri/ /index.html;  # 支持SPA路由
    }
    
    # API代理配置（指向8443端口的后端服务）
    location /api {
        proxy_pass https://localhost:8443;  # 后端API服务地址（使用HTTPS）
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # SSL相关设置
        proxy_ssl_session_reuse on;
        proxy_ssl_verify off;  # 如果后端服务使用自签名证书，设为off
        # 如需验证证书，请使用以下设置
        # proxy_ssl_verify on;
        # proxy_ssl_trusted_certificate /path/to/ca.crt;
        # proxy_ssl_verify_depth 2;
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

## 配置说明

1. **HTTP到HTTPS重定向**：
   - 配置了80端口的服务器块，将所有HTTP请求强制重定向到HTTPS。

2. **SSL配置**：
   - 启用了443端口的HTTPS服务
   - 配置了SSL证书和私钥路径（请替换为您的实际路径）
   - 设置了安全的SSL协议和会话参数

3. **后端API代理**：
   - 将所有`/api`路径的请求代理到`https://localhost:8443`
   - 保留了原始请求的主机名和IP信息
   - 配置了SSL会话重用
   - 默认关闭了SSL证书验证（适用于自签名证书环境）

4. **静态文件服务**：
   - 与DEPLOYMENT.md中的配置保持一致
   - 支持SPA应用的路由重写
   - 配置了静态资源的长期缓存

## 使用说明

1. 复制上述配置到Nginx的配置文件中（通常位于`/etc/nginx/conf.d/`或`/etc/nginx/sites-available/`目录）

2. 替换以下占位符：
   - `example.com` - 您的域名
   - `/path/to/your/certificate.crt` - 您的SSL证书路径
   - `/path/to/your/private.key` - 您的SSL私钥路径
   - `/path/to/your/dist/folder` - 项目的dist目录路径

3. 检查配置语法：
   ```bash
   nginx -t
   ```

4. 重新加载Nginx使配置生效：
   ```bash
   nginx -s reload
   ```

## 注意事项

- 如果您的后端API服务使用自签名证书，保持`proxy_ssl_verify off`设置
- 如果使用受信任的证书，建议开启证书验证以提高安全性
- 确保Nginx用户有权限读取证书文件
- 定期更新SSL证书以避免过期