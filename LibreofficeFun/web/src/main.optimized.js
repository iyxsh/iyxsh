import { createApp } from 'vue'
import App from './App.vue'
import router from './router'
import { t, setLocale, getLocale } from './utils/i18n'

// 导入全局样式
import './styles/index.css'

console.log('开始创建Vue应用');

// 创建Vue实例
const app = createApp(App)

// 环境变量处理
const env = {
  environment: process.env.NODE_ENV || 'development',
  version: process.env.VUE_APP_VERSION || '1.0.0',
  baseUrl: process.env.BASE_URL || '/',
}

// 应用初始化日志
console.log('应用初始化，配置信息:', {
  ...env,
  routerConfig: {
    mode: router.options.history.mode,
    routes: router.options.routes.map(route => ({
      path: route.path,
      name: route.name,
      component: route.component.name || 'LazyLoadedComponent'
    }))
  }
})

// 全局错误处理
app.config.errorHandler = (err, vm, info) => {
  console.error(`Vue错误 [${vm?.$options?.name || '未知组件'}]: ${err.message}`)
  console.error('错误堆栈:', err.stack)
  console.error('错误详情:', {
    error: err,
    component: vm?.$options?.name,
    info,
    stack: err.stack
  })

  // 添加上报逻辑的占位
  // 这里可以集成错误监控系统
}

// 添加全局未处理的Promise拒绝事件监听器
window.addEventListener('unhandledrejection', (event) => {
  console.error('未处理的Promise拒绝:', {
    reason: event.reason,
    stack: event.reason?.stack
  })

  // 阻止默认处理
  event.preventDefault()
})

// 性能监控 - 长任务检测
if (window.PerformanceObserver && PerformanceObserver.supportedEntryTypes.includes('longtask')) {
  const longTaskObserver = new PerformanceObserver((list) => {
    list.getEntries().forEach(entry => {
      if (entry.duration > 100) { // 超过100ms的任务
        console.warn(`检测到长任务: ${entry.duration}ms`, {
          name: entry.name,
          startTime: entry.startTime,
          duration: entry.duration
        })
      }
    })
  })

  longTaskObserver.observe({ entryTypes: ['longtask'] })
}

// 性能监控 - 空闲资源检测
if ('requestIdleCallback' in window) {
  requestIdleCallback((deadline) => {
    console.log('初始空闲时间:', deadline.timeRemaining())

    // 如果初始空闲时间小于50ms，提示可能需要优化
    if (deadline.timeRemaining() < 50) {
      console.warn('初始空闲时间小于50ms，建议优化初始加载')
    }
  }, { timeout: 1000 })
}

// 注册全局属性
app.config.globalProperties.$t = t
app.config.globalProperties.$setLocale = setLocale
app.config.globalProperties.$getLocale = getLocale

console.log('全局属性已设置');

// 注册路由
app.use(router)

// 挂载应用
try {
  console.log('开始挂载应用')
  const appInstance = app.mount('#app')
  console.log('应用挂载成功')

  // 开发环境下挂载实例到window
  if (process.env.NODE_ENV !== 'production') {
    window.$app = appInstance
    console.log('应用实例已挂载到 window.$app，可用于调试')
  }
} catch (mountError) {
  console.error('应用挂载失败:', mountError)

  // 显示致命错误信息
  const rootElement = document.getElementById('app')
  if (rootElement) {
    rootElement.innerHTML = `
      <div style="color: #f56c6c; text-align: center; padding: 20px;">
        <h2>应用启动失败</h2>
        <p>请尝试刷新页面或清除浏览器缓存</p>
        <div style="margin-top: 20px;">
          <details>
            <summary>错误详情</summary>
            <pre style="text-align: left; background: #f8f8f8; padding: 10px; border-radius: 4px;">${mountError.stack || mountError.message}</pre>
          </details>
        </div>
        <button onclick="location.reload()" style="margin-top: 20px; padding: 8px 16px; background: #f56c6c; color: white; border: none; border-radius: 4px; cursor: pointer;">
          重新加载页面
        </button>
      </div>
    `
  }
}

// 导出app实例以便其他模块可以使用
export default app
