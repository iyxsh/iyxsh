import { createPinia } from 'pinia'

// 创建Pinia实例
const pinia = createPinia()

// 导出存储实例
export { pinia }

// 导出各个模块存储
export { usePageStore } from './pageStore'
export { useFormStore } from './formStore'
export { useConfigStore } from './configStore'

// 设置持久化插件
export function setupPiniaPlugins() {
  // 可以在这里添加Pinia插件，比如持久化插件
  return pinia
}
