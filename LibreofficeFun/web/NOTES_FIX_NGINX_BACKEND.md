# 修复 Nginx "host not found in upstream 'backend-server'" 错误

## 问题分析

您在配置 Nginx 时遇到了以下错误：

```
nginx: [emerg] host not found in upstream "backend-server" in /usr/local/etc/nginx/nginx.conf
```

这个错误发生的原因是：在 `DEPLOYMENT.md` 文件中提供的 Nginx 配置示例使用了 `backend-server` 作为后端服务器的占位符，但这个主机名在您的环境中并不存在。

## 解决方案

要解决这个问题，您需要修改 Nginx 配置文件，将 `backend-server` 替换为实际的后端 API 服务器地址。请按照以下步骤操作：

### 步骤 1: 确定实际的后端服务器地址

首先，您需要确定 LibreOfficeFun 后端服务器的实际地址。这可能是：
- 本地服务器的 IP 地址（如 `127.0.0.1`）
- 局域网内另一台服务器的 IP 地址（如 `192.168.1.100`）
- 公共域名（如 `api.yourdomain.com`）

同时，请确认后端服务器监听的端口号（默认可能是 8080）。

### 步骤 2: 修改 Nginx 配置文件

打开您的 Nginx 配置文件（通常位于 `/usr/local/etc/nginx/nginx.conf` 或 `/etc/nginx/nginx.conf`），找到以下部分：

```nginx
# API 代理配置（如果需要）
location /api {
    proxy_pass http://backend-server:8080;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}
```

将 `http://backend-server:8080` 替换为实际的后端服务器地址。例如：

- 如果后端在本地运行：`http://127.0.0.1:8080`
- 如果后端在局域网的其他服务器：`http://192.168.1.100:8080`
- 如果后端有域名：`http://api.yourdomain.com:8080`

修改后的配置应该类似于：

```nginx
# API 代理配置（如果需要）
location /api {
    proxy_pass http://127.0.0.1:8080;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}
```

### 步骤 3: 测试并应用配置

修改完成后，运行以下命令测试配置是否正确：

```bash
sudo nginx -t
```

如果测试通过，重新加载 Nginx 以使配置生效：

```bash
sudo systemctl reload nginx
# 或者在某些系统上
# sudo service nginx reload
```

## 高级配置选项

### 后端服务器不可用的情况

如果您暂时没有部署后端服务器，或者不需要 API 功能，您可以选择：

1. **注释掉 API 代理配置**：在 Nginx 配置中，将整个 `location /api` 块注释掉或删除

2. **设置一个默认响应**：如果前端仍会发送 API 请求，您可以配置 Nginx 返回默认响应：

```nginx
location /api {
    return 200 '{"status":"ok","message":"Mock API response"}';
    add_header Content-Type application/json;
}
```

### 环境变量配置

请注意，项目构建时使用的 `VITE_API_BASE_URL` 环境变量也会影响 API 请求的基础 URL。默认值为 `/api`，这与 Nginx 配置中的代理路径匹配。

如果您需要更改 API 的基本路径，请确保同时更新：
1. 构建时的 `VITE_API_BASE_URL` 环境变量
2. Nginx 配置中的 `location` 路径和 `proxy_pass` 目标

## 故障排除

如果修改后仍然遇到问题，请检查：

1. 后端服务器是否正在运行
2. 后端服务器的防火墙设置是否允许 Nginx 访问
3. 网络连接是否正常（可以使用 `ping` 或 `curl` 测试）
4. 端口号是否正确（后端可能使用了非默认端口）

有关更多部署和配置信息，请参考项目中的 `DEPLOYMENT.md` 文件。

如有其他问题，请联系项目维护团队获取帮助。