# 调试和 Core Dump 配置说明

本文档介绍了对 LibreOfficeFun 项目的调试支持和 core dump 配置优化，帮助开发者和运维人员更好地进行问题定位和排查。

## 优化说明

我们对 `CMakeLists.txt` 文件进行了以下优化：

### 增强的调试支持（Debug 模式）
- 保留了原有的 `-g -ggdb` 调试信息生成选项
- 添加了 `-O0` 禁用优化，使调试更准确
- 添加了 `-Wall -Wextra` 启用更多编译警告
- 添加了 `-fno-omit-frame-pointer` 保留帧指针，使堆栈跟踪更准确

### Core Dump 支持（Release 模式）
- 保留了 `-O3` 优化选项以保证性能
- 添加了 `-g` 生成调试信息，使 core 文件包含有用信息
- 添加了 `-fno-omit-frame-pointer` 保留帧指针
- 添加了 `-rdynamic` 链接器选项，使动态符号可用于调试

## 新增文件

### 1. core_dump_config.md

提供了详细的 Core Dump 配置指南，包括：
- 什么是 Core Dump 文件
- 如何在不同系统（Linux、FreeBSD、Windows）中启用和配置 Core Dump
- 如何使用 Core Dump 文件进行调试
- 相关的注意事项

### 2. setcore.sh

提供了一个专用脚本，仅用于配置 Core Dump 环境：
- 配置 Core 文件大小不受限制
- 设置 Core 文件的保存路径和命名格式
- 启用 PID 作为 Core 文件名的一部分
- 提供配置状态反馈

### 3. 修改后的 start.sh

原有的启动脚本已更新，会在启动服务前自动调用 setcore.sh 来配置 Core Dump 环境。

## 如何使用

### 开发环境（Debug 模式）

1. 使用 Debug 模式构建项目：
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   ```

2. 使用 GDB 进行调试：
   ```bash
   gdb ./libreofficefun
   ```

### 生产环境（Release 模式）

1. 使用 Release 模式构建项目：
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make
   ```

2. 使用修改后的启动脚本启动服务（会自动配置 Core Dump）：
   ```bash
   cd bin
   bash ./start.sh
   ```

3. 当服务崩溃时，Core 文件将保存在配置的目录中（默认 `/var/core/`）

4. 使用 GDB 分析 Core 文件：
   ```bash
   gdb ./libreofficefun /path/to/corefile
   ```

### 单独配置 Core Dump 环境

如果需要单独配置 Core Dump 环境而不启动服务，可以直接运行：
```bash
cd bin
chmod +x ./setcore.sh
./setcore.sh
```

## 注意事项

1. 在生产环境中启用 Core Dump 可能会产生较大的文件，请确保有足够的磁盘空间
2. Core 文件可能包含敏感信息，请妥善保管和处理
3. 定期清理旧的 Core 文件以避免磁盘空间浪费
4. 使用与生成 Core 文件相同版本的可执行文件和库进行调试，以确保符号匹配

## 排错提示

- 如果 Core 文件没有生成，请检查系统限制和配置是否正确
- 如果调试时符号不匹配，请确保使用了相同版本的构建文件
- 如果使用 Docker 或容器化环境，可能需要额外的配置来启用 Core Dump

如需更多帮助，请参考 `core_dump_config.md` 文件或联系开发团队。