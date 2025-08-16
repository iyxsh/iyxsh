import { createRouter, createWebHistory } from 'vue-router';

// 修改 PageManager 路由配置，使用异步组件加载
const lazyLoadView = (view) => {
  return () => import(`../views/${view}.vue`);
};

const routes = [
    {
        path: '/',
        name: 'home',
        component: lazyLoadView('HomeView'),
        meta: { title: 'Home' }
    },
    {
        path: '/about',
        name: 'about',
        component: lazyLoadView('AboutView')
    },
    {
        path: '/page',
        name: 'page',
        component: lazyLoadView('PageManager'),
        props: route => ({ fileName: route.query.fileName }) // 添加查询参数处理
    },
    {
        path: '/files',
        name: 'files',
        component: lazyLoadView('FileManager')
    },
    {
        path: '/:pathMatch(.*)*', // 捕获未定义路径
        redirect: '/' // 重定向到根路径
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
    currentRoute: to.path,
    routeName: to.name
  });
});

export default router