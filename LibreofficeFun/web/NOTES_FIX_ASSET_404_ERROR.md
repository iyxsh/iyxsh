# 解决静态资源404错误指南

## 问题分析

根据您提供的错误信息，浏览器在请求以下资源时返回404 Not Found错误：

```
https://192.168.0.6/assets/index-B9XDXsN5.js
https://192.168.0.6/assets/vue-DrAI8ZEh.js
https://192.168.0.6/assets/vendors-CzZYakZo.js
https://192.168.0.6/assets/lodash-S0Y0Up6J.js
https://192.168.0.6/assets/element-plus-VYtAbma5.js
https://192.168.0.6/assets/pinia-q3YNFe3y.js
https://192.168.0.6/assets/element-plus-mtAHe734.css
https://192.168.0.6/assets/index-rpo13He2.css
```

我检查了您的`dist/assets`目录和`dist/index.html`文件，发现了明确的问题：**浏览器正在请求的资源文件名与实际存在的文件名不匹配**。

## 明确的问题原因

通过对比，我发现：

1. **实际存在的文件**（在`dist/assets`目录中并在`dist/index.html`中引用）：
   - `index-nAzBTmfR.js`（浏览器请求的是`index-B9XDXsN5.js`）
   - `vue-DrAI8ZEh.js`（这个文件名匹配，但仍然报错）
   - `vendors-Dm9yPoNF.js`（浏览器请求的是`vendors-CzZYakZo.js`）
   - `lodash-D_V24Ga1.js`（浏览器请求的是`lodash-S0Y0Up6J.js`）
   - `element-plus-BROPxBcI.js`（浏览器请求的是`element-plus-VYtAbma5.js`）
   - `pinia-q3YNFe3y.js`（这个文件名匹配，但仍然报错）
   - `element-plus-CtIGEsJ0.css`（浏览器请求的是`element-plus-mtAHe734.css`）
   - `index-rpo13He2.css`（这个文件名匹配，但仍然报错）

2. **根本原因**：浏览器正在加载**缓存的旧版本index.html文件**，其中引用了旧的资源哈希值，但这些旧资源文件已被新版本替换，导致404错误。

## 解决方案

### 1. 清除浏览器缓存（最直接的解决方案）

强制浏览器加载最新的index.html文件：

- **Chrome/Edge**：按`Ctrl+Shift+R`进行硬刷新，或按`Ctrl+Shift+Delete`清除缓存
- **Firefox**：按`Ctrl+Shift+R`或`Ctrl+F5`进行硬刷新
- **Safari**：按住`Shift`键点击刷新按钮
- 也可以尝试使用浏览器的无痕/隐私模式访问网站

### 2. 检查Nginx配置

确保Nginx配置正确指向dist目录，并且没有缓存HTML文件：

```nginx
location / {
    root /path/to/your/dist/folder;  # 确保这是正确的dist目录路径
    index index.html;
    try_files $uri $uri/ /index.html;  # 支持SPA路由
    
    # 重要：不要缓存HTML文件
    if ($request_filename ~* .*\.(?:htm|html)$) {
        add_header Cache-Control "no-cache, no-store, must-revalidate";
        add_header Pragma "no-cache";
        add_header Expires "0";
    }
}

# 对静态资源设置合理的缓存时间
location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg)$ {
    expires 30d;
    add_header Cache-Control "public, max-age=2592000";
}
```

### 3. 重新构建和部署项目

确保您部署的是最新构建的文件：

```bash
# 在项目根目录执行
npm install  # 确保所有依赖都已安装
npm run build  # 重新构建项目
```

构建完成后，确认`dist`目录中的文件与`dist/index.html`中的引用匹配。

### 4. 重启Nginx服务

应用配置更改并确保服务正常运行：

```bash
nginx -t  # 验证配置是否正确
nginx -s reload  # 重新加载Nginx配置
```

### 5. 高级解决方案：配置版本化URL或缓存破坏

为了防止未来出现类似问题，可以在构建过程中实现更强大的缓存策略：

- 使用构建工具（如Vite）的内容哈希文件名功能（已启用，但需要确保浏览器获取最新的HTML）
- 在部署脚本中添加清除CDN缓存的步骤（如果使用CDN）
- 考虑在HTML中添加版本查询参数

## 验证方法

解决后，您可以通过以下方式验证：

1. 打开浏览器开发者工具（F12）
2. 切换到Network选项卡
3. 勾选"Disable cache"选项
4. 刷新页面
5. 确认所有资源都能成功加载，没有404错误

如果以上方法都无法解决问题，请检查服务器文件权限、Nginx用户权限或是否存在其他代理服务器缓存问题。