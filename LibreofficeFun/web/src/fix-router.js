// 这是一个临时修复程序，用于解决路由显示问题
// 运行此脚本可在控制台中查看当前路由信息

(function() {
  // 等待Vue应用初始化
  window.addEventListener('DOMContentLoaded', () => {
    setTimeout(checkAndFixRouterDisplay, 1000);
  });

  function checkAndFixRouterDisplay() {
    try {
      // 检查Vue应用实例是否可用
      if (window.$app) {
        console.log('======= 路由显示修复工具 =======');

        // 获取当前路由信息
        const router = window.$app.$router;
        const currentRoute = router?.currentRoute?.value;

        console.log('当前路由信息:', {
          path: currentRoute?.path,
          name: currentRoute?.name,
          params: currentRoute?.params,
          query: currentRoute?.query,
          hash: currentRoute?.hash
        });

        // 检查App.vue组件的currentRoute实现
        const appComponent = window.$app?.$root;
        console.log('App组件currentRoute实现:', {
          currentRouteType: typeof appComponent?.currentRoute,
          isRef: appComponent?.currentRoute?.__v_isRef,
          isComputed: appComponent?.currentRoute?.__v_isReadonly
        });

        // 建议修复方法
        console.log('问题诊断:');
        console.log('1. 如果currentRoute是ref而非computed属性，需要在模板中使用currentRoute.value.path');
        console.log('2. 如果currentRoute是computed属性，在模板中应使用currentRoute.path');
        console.log('===== 修复建议 =====');
        console.log('在App.vue中，将:');
        console.log('const currentRoute = ref(null)');
        console.log('修改为:');
        console.log('const currentRoute = computed(() => route)');
        console.log('===================');
      } else {
        console.log('Vue应用尚未初始化，请在应用加载完成后重试');
      }
    } catch (error) {
      console.error('路由显示诊断错误:', error);
    }
  }
})();
