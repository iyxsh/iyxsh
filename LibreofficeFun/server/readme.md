一键快速生成 支持开发测试 修改对应IP
openssl req -x509 -newkey rsa:2048 -nodes -keyout server-key.pem -out server-cert.pem -days 365 -subj "/CN=localhost" -addext "subjectAltName=IP:192.168.146.128,DNS:localhost"  
不开启代理CA认证

proxy_ssl_verify off;  # 证书验证


递归处理目录下所有文件 dos2unix 
find "$TARGET_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "*.txt" \) -exec dos2unix {} \;


===========================
在 FreeBSD 系统中切换至 root 用户需遵循特定权限策略，以下是完整操作流程及权限配置方法：

🔑 一、切换 root 用户步骤
‌登录普通用户‌
使用具备权限的普通账户登录系统：

sh
Copy Code
ssh your_username@freebsd_ip
‌执行切换命令‌
输入以下命令并填写 root 密码：

sh
Copy Code
su -   # 使用 root 环境变量
# 或
su     # 保留当前环境变量
Password: [输入root密码]
⚠️ 若提示 su: Sorry 错误，表明当前用户无权切换（权限未配置）1011。

⚙️ 二、配置切换权限（关键）
普通用户需加入 wheel 组（组ID为0）才允许切换 root：

▶️ 方法 1：命令配置（推荐）
以 ‌root 身份‌ 执行：

sh
Copy Code
# 将用户加入 wheel 组
pw groupmod wheel -m your_username

# 或修改用户主组为 wheel
pw usermod your_username -g wheel
完成后退出重新登录即可生效1112。

▶️ 方法 2：编辑配置文件
以 root 编辑 /etc/group：
sh
Copy Code
ee /etc/group
在 wheel 组末尾添加用户名（逗号分隔）：
conf
Copy Code
wheel:*:0:root,your_username  # 添加至逗号分隔列表
保存退出并重启生效1011。
🔒 三、安全注意事项
‌禁止 root 远程登录‌
FreeBSD 默认禁止 root 通过 SSH 直接登录（安全策略）12。必须通过普通用户中转切换：

sh
Copy Code
# /etc/ssh/sshd_config
PermitRootLogin no  # 默认禁止
‌最小权限原则‌

仅必要用户加入 wheel 组
日常操作使用普通账户，避免滥用 root1112。
💻 操作验证
配置后测试切换：

sh
Copy Code
$ whoami
your_username
$ su -
Password: [输入root密码]
# whoami  # 成功切换
root
⚠️ ‌故障排除‌：若仍失败，检查：

用户是否在 wheel 组：id your_username
/etc/group 中 wheel 组配置
root 密码是否正确（重置密码需进单用户模式）
===========================

===========================
切换国内源
参考 https://mirrors.ustc.edu.cn/help/freebsd-pkg.html

FreeBSD pkg 包管理器的官方源配置是 /etc/pkg/FreeBSD.conf，请先检查该文件内容。注意其中的 url 参数配置了官方仓库的地址，我们需要把它替换为镜像站的地址。

该配置文件是 FreeBSD 基本系统的一部分，会随着 freebsd-update 更新，请不要直接修改，而是创建 /usr/local/etc/pkg/repos/FreeBSD.conf 覆盖配置，文件内容如下：


FreeBSD: {
  url: "http://mirrors.ustc.edu.cn/freebsd-pkg/${ABI}/quarterly",
  mirror_type: "none",
}
如果要使用滚动更新的 latest 仓库，把 url 配置最后的 quarterly 换成 latest 即可。

修改配置后，运行 pkg update -f 更新索引。
===========================

===========================
ports collection
参考：https://mirrors.ustc.edu.cn/help/freebsd-ports.html
pkg install git 
git clone --filter=tree:0 https://mirrors.ustc.edu.cn/freebsd-ports/ports.git /usr/ports

===========================
libreoffice sdk for FreeBSD 安装配置

参考：https://blog.fortinet.com/2018/02/13/compiling-the-libreoffice-sdk-on-freebsd-11-2
在 FreeBSD 上安装 LibreOffice SDK，‌推荐使用 FreeBSD 原生的 Ports 系统编译安装‌，而非尝试安装 Linux 格式的包（如 DEB/RPM）。以下是具体步骤和关键点：
1. ‌更新Ports Tree‌
首次使用或需完整更新时执行：
portsnap fetch extract


后续增量更新只需：
portsnap fetch update
‌提速技巧‌：修改/etc/portsnap.conf，替换默认服务器为国内镜像（如portsnap.hshh.org）

📦 方法一：通过 Ports 编译安装 LibreOffice (含 SDK 选项)
‌定位 LibreOffice 的 Ports 目录‌
LibreOffice 的主 Port 位于：/usr/ports/editors/libreoffice。SDK 是其编译选项之一。1 4

‌配置编译选项 (启用 SDK)‌
进入 Port 目录，运行配置菜单：

bash
Copy Code
cd /usr/ports/editors/libreoffice
sudo make config
在配置界面中：

勾选 SDK 选项（通常归类在 “Build Options” 或 “Components” 中）。
根据需求启用其他选项（如中文支持 LOCALIZED_LANG=zh_CN）。1
‌编译并安装‌
执行编译安装命令（首次编译耗时较长，需耐心等待）：

bash
Copy Code
sudo make install clean
‌验证 SDK 安装‌
安装完成后，SDK 文件通常位于：

遇到问题：make deinstall   # 卸载旧版本
make reinstall # 重装安装新版本

头文件：/usr/local/include/libreoffice/
库文件：/usr/local/lib/libreoffice/
工具和文档：/usr/local/share/libreoffice/sdk/


## 设计原则
🤖 库设计遵循 Unix 哲学：
* 每个工具应只做一件简单而有用的事
* 每个工具应有很小的接口和明确的输入输出

设置配置文件config.json 配置项目中使用的相关项
deafault.ods 模板文件 支持模版文件预加载，重新加载、加载时接口不允许调用返回加载中