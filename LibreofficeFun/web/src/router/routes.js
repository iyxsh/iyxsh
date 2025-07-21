import { defineAsyncComponent } from 'vue'

// 异步组件加载处理
const loadAsyncComponent = (componentPath) => {
  return defineAsyncComponent({
    loader: () => import(`../views/${componentPath}.vue`),
    // 加载中显示的组件
    loadingComponent: () => import('../components/LoadingComponent.vue'),
    // 出错时显示的组件
    errorComponent: () => import('../components/ErrorComponent.vue'),
    // 延迟显示加载组件的时间，默认为200ms
    delay: 200,
    // 如果提供了timeout，并且加载组件的时间超过了设定值，将显示错误组件
    timeout: 10000,
    // 定义组件是否可挂起，默认为true
    suspensible: true,
    onError(error, retry, fail, attempts) {
      // 重试逻辑
      if (attempts <= 3) {
        // 重试最多3次
        retry()
      } else {
        // 重试3次后最终失败
        console.error('组件加载失败:', error)
        fail()
      }
    }
  })
}

// 路由配置
export const routes = [
  {
    path: '/',
    name: 'home',
    component: () => import('../views/HomeView.vue'),
    meta: { 
      title: '首页',
      keepAlive: true,
      requiresAuth: false
    }
  },
  {
    path: '/about',
    name: 'about',
    component: loadAsyncComponent('AboutView'),
    meta: { 
      title: '关于',
      keepAlive: false,
      requiresAuth: false
    }
  },
  {
    path: '/page',
    name: 'page',
    component: loadAsyncComponent('PageManager'),
    meta: { 
      title: '页面管理',
      keepAlive: true,
      requiresAuth: false
    }
  },
  {
    path: '/forms',
    name: 'forms',
    component: loadAsyncComponent('FormManager'),
    meta: { 
      title: '表单管理',
      keepAlive: true,
      requiresAuth: false
    }
  },
  {
    path: '/cards',
    name: 'cards',
    component: loadAsyncComponent('CardManager'),
    meta: { 
      title: '卡片管理',
      keepAlive: true,
      requiresAuth: false
    }
  },
  {
    path: '/settings',
    name: 'settings',
    component: loadAsyncComponent('SettingsView'),
    meta: { 
      title: '设置',
      keepAlive: false,
      requiresAuth: false
    }
  },
  // 捕获所有未匹配路由
  {
    path: '/:pathMatch(.*)*',
    name: 'not-found',
    component: loadAsyncComponent('NotFoundView'),
    meta: { 
      title: '页面未找到',
      keepAlive: false,
      requiresAuth: false
    }
  }
]
