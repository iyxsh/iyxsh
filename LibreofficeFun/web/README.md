# Vue 3 + Element Plus 可拖拽表单管理系统

## 项目简介
本项目基于 Vite + Vue 3 + Element Plus，支持：
- 添加/编辑单个表单（主名称、值、备注、可选图片/视频）
- 多表单卡片式网格布局，可拖拽排序
- 组件化管理表单集合
- 多页面管理，每页可有不同表单集合
- 表单信息未提交前支持 localStorage 缓存

## 启动方式
```bash
npm install
npm run dev
```

## 依赖
- vue
- element-plus
- vue-draggable-next

## 目录结构建议
- src/components  主要组件
- src/pages       页面管理
- src/store       状态管理
- src/router      路由

## 说明
如需自定义功能，请在 copilot-instructions.md 中补充说明。


下一步计划 
将卡片页文本显示替换为svg图片显示；配合后台文转图功能，直接取转换后的svg图片
