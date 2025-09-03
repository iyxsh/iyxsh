# LibreofficeFun 构建指南

本项目支持构建不同版本的应用程序，以适应不同的环境需求。

## 可用的构建脚本

### 开发版本构建

开发版本适用于开发环境，特点是：
- 不压缩代码，保留原始格式便于调试
- 生成 sourcemap 文件，有助于定位问题
- 输出到 `dist-dev` 目录

```bash
npm run build:dev
```

### 生产版本构建

生产版本适用于部署到生产环境，特点是：
- 使用 esbuild 压缩代码，减小文件体积
- 不生成 sourcemap 文件，提高安全性
- 输出到 `dist` 目录

```bash
npm run build:prod
```

### 同时构建两个版本

如果需要同时生成开发版本和生产版本，可以使用：

```bash
npm run build:both
```

## 预览构建结果

构建完成后，可以预览构建结果：

- 预览生产版本：
  ```bash
  npm run preview
  ```

- 预览开发版本：
  ```bash
  npm run preview:dev
  ```

## 环境特定配置

项目使用不同的环境配置文件来设置特定于环境的参数：

- `.env.development` - 开发环境配置
- `.env.production` - 生产环境配置

这些文件中包含了如 API 地址、调试模式等环境特定的配置项。

## 注意事项

1. 在部署到生产环境前，请确保使用 `build:prod` 命令构建应用
2. 开发过程中，使用 `build:dev` 可以更快地构建并方便调试
3. 环境变量需要以 `VITE_` 前缀命名才能被 Vite 识别和处理