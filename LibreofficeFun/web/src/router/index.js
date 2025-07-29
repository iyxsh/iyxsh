import { createRouter, createWebHistory } from 'vue-router';
// 只保留静态导入
import HomeView from '../views/HomeView.vue';
// 需要懒加载的组件
import AboutView from '../views/AboutView.vue';

// 修改 PageManager 路由配置，使用异步组件加载
const lazyLoadView = (view) => {
  return () => import(`../views/${view}.vue`);
};

const routes = [
  {
    path: '/',
    name: 'home',
    // 统一使用静态导入
    component: HomeView,
    meta: { title: 'Home' }
  },
  {
    path: '/about',
    name: 'about',
    // 统一使用静态导入
    component: AboutView
  },
  {
    path: '/page',
    name: 'page',
    // 使用异步组件加载
    component: lazyLoadView('PageManager')
  }
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
  scrollBehavior: (to, from, savedPosition) => {
    if (savedPosition) {
      return savedPosition
    } else {
      return { top: 0 }
    }
  }
});

// 添加路由守卫
router.beforeEach((to, from, next) => {
  console.log(`Routing from ${from.path} to ${to.path}`);
  // 添加详细的路由信息输出
  console.log('Route object:', {
    to: {
      path: to.path,
      name: to.name,
      params: to.params,
      query: to.query
    },
    from: {
      path: from.path,
      name: from.name,
      params: from.params,
      query: from.query
    }
  });
  next();
});

router.afterEach((to, from) => {
  console.log(`Routed from ${from.path} to ${to.path}`);
  // 添加路由完成后的调试信息
  console.log('Route transition completed', {
    currentRoute: router.currentRoute.value.path,
    routeName: router.currentRoute.value.name
  });
});

export default router