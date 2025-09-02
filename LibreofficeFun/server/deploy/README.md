# LibreofficeFun 生产环境部署指南

## 概述
本指南提供了在生产环境中部署 LibreofficeFun 服务的详细步骤和最佳实践。

## 系统要求
- **操作系统**：Debian/Ubuntu、CentOS/RHEL、SUSE 或 FreeBSD
- **依赖软件**：
  - CMake 3.10+ 
  - C++ 编译器 (支持 C++17)
  - OpenSSL
  - LibreOffice 及开发包
  - dos2unix
- **内存**：建议至少 2GB RAM
- **磁盘空间**：至少 1GB 可用空间

## 部署文件结构
```
server/
├── deploy/
│   ├── deploy.sh         # 主部署脚本
│   ├── libreofficefun.service  # systemd服务文件模板
│   └── README.md         # 部署指南
├── bin/                  # 二进制文件和配置
├── build/                # 构建目录
├── log/                  # 日志目录
└── data/                 # 数据存储目录
```

## 部署步骤

### 1. 准备环境

首先，确保您的系统满足基本要求，并具有 root 或 sudo 权限。

### 2. 使用部署脚本

`deploy.sh` 脚本提供了完整的部署流程，支持以下命令：

```bash
# 进入部署目录
cd /path/to/LibreofficeFun/server/deploy

# 授予执行权限
chmod +x deploy.sh

# 查看帮助信息
./deploy.sh
```

### 3. 完整部署流程

执行以下命令进行完整的部署：

```bash
./deploy.sh all
```

这将自动执行以下步骤：
- 创建必要的目录
- 清理文件格式（转换为Unix格式）
- 安装系统依赖
- 生成SSL证书
- 构建项目
- 配置生产环境参数
- 配置systemd系统服务
- 启动服务

### 4. 单独执行特定操作

您也可以单独执行特定的部署步骤：

```bash
# 仅安装依赖
./deploy.sh install

# 仅构建项目
./deploy.sh build

# 仅配置生产环境
./deploy.sh configure

# 启动服务
./deploy.sh start

# 停止服务
./deploy.sh stop

# 重启服务
./deploy.sh restart

# 查看服务状态
./deploy.sh status

# 清理构建文件
./deploy.sh clean
```

## 生产环境配置说明

### 配置文件
生产环境配置文件位于 `bin/config.json`，主要配置项包括：
- **port**：服务端口（默认8443）
- **datapath**：数据存储路径
- **maxOpenDocuments**：最大打开文档数
- **log_level**：日志级别（0=详细，1=仅错误和信息）

部署脚本会自动将日志级别设置为 1（生产环境推荐），并增加最大打开文档数。

### SSL 证书
部署脚本会自动生成自签名的SSL证书，包括：
- `server.crt`：服务器证书
- `server.key`：服务器私钥
- `ca.crt`：CA证书

在生产环境中，建议使用由可信CA签发的证书替换这些自签名证书。

### 系统服务管理
服务使用 systemd 进行管理，主要功能包括：
- 开机自启动
- 自动重启失败的服务
- 资源限制（内存限制2GB，CPU使用率限制50%）
- 详细的日志记录

## 日志管理

系统使用多个日志文件记录不同类型的信息：
- `log/server.log`：应用程序日志
- `log/stop_service.log`：服务停止日志
- `log/deploy.log`：部署脚本日志
- `log/systemd.log`：systemd服务标准输出日志
- `log/systemd_error.log`：systemd服务错误日志

建议配置日志轮转以防止日志文件过大。

## 安全注意事项

1. **证书安全**：保护好私钥文件，设置适当的文件权限（600）
2. **用户权限**：服务默认以root用户运行，在生产环境中可考虑使用专用用户
3. **防火墙**：配置防火墙，只允许必要的端口访问
4. **定期更新**：定期更新系统和依赖软件，以获取安全补丁
5. **备份数据**：定期备份 `data` 目录中的数据

## 故障排除

### 常见问题及解决方案

1. **服务启动失败**
   - 检查日志文件：`tail -f log/server.log log/systemd_error.log`
   - 确认 LibreOffice 服务是否正常运行：`ps aux | grep soffice`
   - 检查端口是否被占用：`netstat -tuln | grep 8443`

2. **SSL 连接问题**
   - 确认证书和私钥匹配：`openssl x509 -noout -modulus -in server.crt | openssl md5` 与 `openssl rsa -noout -modulus -in server.key | openssl md5` 应输出相同结果
   - 检查证书是否过期：`openssl x509 -in server.crt -text -noout | grep Not`

3. **LibreOffice 相关错误**
   - 确认 LibreOffice 安装正确：`soffice --version`
   - 检查 URE_BOOTSTRAP 环境变量是否正确设置

## 维护指南

### 定期维护任务

1. **备份数据**：定期备份 `data` 目录和配置文件
2. **清理日志**：定期检查并清理或归档日志文件
3. **更新软件**：定期更新项目代码和依赖软件
4. **性能监控**：监控系统资源使用情况，根据需要调整配置

### 更新项目

要更新项目到新版本：

```bash
# 拉取最新代码
cd /path/to/LibreofficeFun/server
# 假设使用git管理代码
# git pull

# 重新构建和部署
cd deploy
./deploy.sh build
./deploy.sh restart
```

## 联系方式

如有任何问题或建议，请联系技术支持团队。