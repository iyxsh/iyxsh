import { createRouter, createWebHistory } from 'vue-router'
import { routes } from './routes'

// 创建路由实例
const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
  scrollBehavior(to, from, savedPosition) {
    // 如果有保存的位置则使用
    if (savedPosition) {
      return savedPosition
    }

    // 如果有哈希则滚动到指定元素
    if (to.hash) {
      return {
        el: to.hash,
        behavior: 'smooth'
      }
    }

    // 默认滚动到顶部
    return { top: 0 }
  }
})

// 路由前置守卫
router.beforeEach((to, from, next) => {
  // 更新文档标题
  document.title = `${to.meta.title || '页面'} - LibreOffice Fun`

  // 路由跳转日志
  console.log(`路由从 ${from.path} 跳转到 ${to.path}`)

  // 详细路由信息
  if (import.meta.env.DEV) {
    console.log('路由详情:', {
      to: {
        path: to.path,
        name: to.name,
        params: to.params,
        query: to.query,
        hash: to.hash,
        meta: to.meta
      },
      from: {
        path: from.path,
        name: from.name
      }
    })
  }

  // 权限验证
  if (to.meta.requiresAuth) {
    // 这里添加权限验证逻辑
    const isAuthenticated = localStorage.getItem('auth-token') !== null

    if (!isAuthenticated) {
      // 未认证，重定向到登录页
      next({ 
        path: '/login', 
        query: { redirect: to.fullPath } 
      })
      return
    }
  }

  // 进入路由
  next()
})

// 路由后置钩子
router.afterEach((to, from) => {
  // 路由完成
  console.log(`路由跳转完成: ${from.path} -> ${to.path}`)

  // 路由跳转性能指标
  if (window.performance && window.performance.getEntriesByType) {
    const navigationEntries = window.performance.getEntriesByType('navigation')
    if (navigationEntries.length > 0) {
      const timing = navigationEntries[0]
      console.log('路由跳转性能:', {
        loadTime: timing.loadEventEnd - timing.startTime,
        domReady: timing.domContentLoadedEventEnd - timing.startTime,
        interactive: timing.domInteractive - timing.startTime
      })
    }
  }
})

// 路由错误处理
router.onError((error) => {
  console.error('路由错误:', error)

  // 如果是组件加载错误，可以提供一个全局的回退策略
  if (error.message.includes('Failed to fetch dynamically imported module')) {
    console.warn('组件动态导入失败，尝试刷新页面')
  }
})

export default router
