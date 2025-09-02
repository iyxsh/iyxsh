# Service Worker SSL证书错误修复指南

## 问题概述

在使用HTTPS访问应用时，出现Service Worker注册失败的错误：

```
DOMException: Failed to register a ServiceWorker for scope ('https://192.168.0.6/') with script ('https://192.168.0.6/service-worker.js'): An SSL certificate error occurred when fetching the script.
```

## 原因分析

此错误通常由以下原因引起：

1. **自签名SSL证书不受信任**：浏览器对自签名证书或未经验证的证书有严格的安全限制
2. **证书与域名/IP不匹配**：证书上的域名与您访问的IP地址（192.168.0.6）不匹配
3. **Service Worker文件中的语法错误**：原service-worker.js文件中存在重复的事件监听器和语法错误
4. **Fetch API在SSL环境下的安全策略限制**：缺少适当的跨域和凭证处理

## 已修复的代码问题

我已经对`public/service-worker.js`文件进行了以下修复：

1. **删除了语法错误**：移除了文件中的多余闭合括号`];`
2. **移除了重复的install事件监听器**：保留一个功能完整的install事件处理
3. **优化了SSL环境下的fetch处理**：
   - 添加了origin检查，确保只处理本域请求
   - 为fetch请求添加了`credentials: 'same-origin'`和`redirect: 'follow'`选项
4. **移除了不存在的资源引用**：删除了对不存在的`assets-manifest.json`文件的加载尝试

## SSL证书问题解决方案

### 开发环境临时解决方案

#### 方法1：在Chrome中忽略证书错误（开发环境）

1. 在Chrome中打开新标签页，输入`chrome://flags/#allow-insecure-localhost`
2. 将"Allow invalid certificates for resources loaded from localhost"选项设置为Enabled
3. 重启浏览器
4. 再次访问应用，Service Worker应该能够正常注册

#### 方法2：使用localhost而非IP地址

1. 编辑您的hosts文件，添加：`192.168.0.6    local.libreofficefun.com`
2. 使用`https://local.libreofficefun.com`访问应用
3. 这有助于解决证书与IP地址不匹配的问题

### 生产环境永久解决方案

#### 方法3：使用受信任的SSL证书

1. 从可信的证书颁发机构（CA）获取SSL证书
2. 确保证书包含您的域名或IP地址
3. 正确配置您的Web服务器（如Nginx）使用该证书

## Nginx配置建议

确保您的Nginx配置正确支持Service Worker和HTTPS：

```nginx
server {
    listen 443 ssl;
    server_name 192.168.0.6;

    ssl_certificate /path/to/your/certificate.crt;
    ssl_certificate_key /path/to/your/private.key;
    
    # 推荐的SSL参数
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_prefer_server_ciphers off;
    ssl_session_timeout 1d;
    ssl_session_cache shared:SSL:10m;
    ssl_session_tickets off;
    
    # 允许Service Worker脚本
    location /service-worker.js {
        add_header 'Service-Worker-Allowed' '/' always;
        # 注意：不要对service-worker.js文件设置缓存
        expires -1;
    }
    
    # 静态资源缓存
    location / {
        root /path/to/your/dist;
        try_files $uri $uri/ /index.html;
        # Service Worker需要的跨域头
        add_header 'Access-Control-Allow-Origin' '$scheme://$host' always;
        add_header 'Access-Control-Allow-Credentials' 'true' always;
    }
    
    # API代理配置
    location /api {
        proxy_pass https://backend-server:8443;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}

# HTTP重定向到HTTPS
server {
    listen 80;
    server_name 192.168.0.6;
    return 301 https://$host$request_uri;
}
```

## 验证方法

1. 清除浏览器缓存和Service Worker数据：
   - 打开开发者工具（F12）
   - 进入Application > Service Workers，点击"Unregister"按钮
   - 进入Application > Clear Storage，点击"Clear site data"

2. 重新加载页面，检查控制台输出是否有Service Worker注册成功的消息

3. 如果仍有问题，尝试在隐私模式下访问应用，排除浏览器扩展干扰

## 额外注意事项

1. **安全上下文要求**：Service Worker只能在安全上下文（HTTPS或localhost）中运行
2. **路径问题**：确保service-worker.js位于根目录，路径正确
3. **作用域限制**：Service Worker的作用域默认为其所在目录，如需扩大作用域需添加特殊HTTP头
4. **缓存策略**：避免对service-worker.js文件设置缓存，确保用户总是获取最新版本

按照以上步骤操作后，您的Service Worker应该能够在SSL环境下正常注册和运行。