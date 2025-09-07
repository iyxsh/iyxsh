# Core Dump 配置指南

本文件提供如何在不同环境中配置 core 文件生成，以便在程序崩溃时能够更好地定位问题。

## 什么是 Core Dump 文件？

Core Dump 文件是程序崩溃时的内存快照，包含了程序崩溃时的堆栈信息、寄存器状态和内存内容，对于分析程序崩溃原因非常有价值。

## 启用 Core Dump 文件生成

### Linux 系统

在 Linux 系统中，可以通过以下命令启用和配置 core 文件：

```bash
# 设置 core 文件大小不受限制
echo "* soft core unlimited" >> /etc/security/limits.conf
echo "* hard core unlimited" >> /etc/security/limits.conf

# 设置 core 文件格式和保存路径
echo "kernel.core_pattern=/var/core/core.%e.%p.%t" >> /etc/sysctl.conf
echo "kernel.core_uses_pid=1" >> /etc/sysctl.conf

# 使配置生效
sysctl -p
```

这些命令会：
1. 允许生成任意大小的 core 文件
2. 设置 core 文件的命名格式为 `core.程序名.进程ID.时间戳`
3. 保存到 `/var/core/` 目录（需要确保该目录存在且可写）

### FreeBSD 系统

在 FreeBSD 系统中，可以通过以下方式配置：

```bash
# 在 /etc/sysctl.conf 中添加以下行
kern.corefile="/var/core/%N.%P.core"
kern.coredump=1

# 使配置生效
sysctl -w kern.corefile="/var/core/%N.%P.core"
sysctl -w kern.coredump=1
```

### Windows 系统

Windows 系统中，程序崩溃时默认会生成错误报告。如需生成类似 core 文件的转储文件，可以通过以下方式配置：

1. 使用 Windows 任务管理器
   - 右键点击任务栏 -> 任务管理器
   - 找到你的程序 -> 右键点击 -> 创建转储文件

2. 配置系统级的转储设置
   - 可以使用注册表编辑器配置系统级的转储文件生成
   - 路径：`HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps`

## 使用 Core Dump 文件进行调试

### Linux/FreeBSD 系统（使用 GDB）

```bash
# 使用 gdb 加载 core 文件
gdb ./libreofficefun /path/to/corefile

# 在 gdb 中查看堆栈信息
bt

# 查看特定帧的局部变量
frame <frame-number>
info locals

# 查看内存
x/100x <address>
```

### Windows 系统（使用 Visual Studio）

1. 打开 Visual Studio
2. 选择 `文件` -> `打开` -> `文件...`
3. 选择生成的 .dmp 文件
4. 点击 `使用仅限本机进行调试`

## CMake 配置说明

本项目的 CMakeLists.txt 已配置以下选项以支持 core 文件生成和调试：

### Debug 模式
- `-g -ggdb`: 生成详细的调试信息
- `-O0`: 禁用优化，使调试更准确
- `-Wall -Wextra`: 启用更多警告
- `-fno-omit-frame-pointer`: 保留帧指针，使堆栈跟踪更准确

### Release 模式
- `-O3`: 启用优化
- `-g`: 仍然生成调试信息，以便 core 文件包含有用信息
- `-fno-omit-frame-pointer`: 保留帧指针

### 链接器配置
- `-rdynamic`: 使动态符号可用于调试

## 注意事项

1. Core 文件可能包含敏感信息，请妥善保管
2. 在生产环境中，建议限制 core 文件的大小和保存位置
3. 定期清理旧的 core 文件，避免占用过多磁盘空间
4. 调试时，请确保使用与生成 core 文件相同版本的可执行文件和库