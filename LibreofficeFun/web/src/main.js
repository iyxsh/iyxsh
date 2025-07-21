// main.js

// 模拟的API请求函数
// const mockSendErrorLog = (logData) => {
//   // 在开发环境下模拟成功
//   if (process.env.NODE_ENV !== 'production') {
//     console.log('[errorLogService] 模拟发送错误日志:', logData);
//     return Promise.resolve();
//   }
//   
//   // 实际生产环境应该发送到服务器
//   // 这里可以添加实际的API请求逻辑
//   return fetch('/api/logs/error', {
//     method: 'POST',
//     headers: {
//       'Content-Type': 'application/json',
//     },
//     body: JSON.stringify(logData),
//   });
// };

// 错误日志服务
// const errorLogService = {
//   /**
//    * 添加错误日志
//    * @param {Error} error 错误对象
//    * @param {string} message 错误消息
//    * @param {string} level 日志级别
//    * @param {Object} context 上下文信息
//    */
//   addErrorLog: (error, message, level = 'error', context = {}) => {
//     try {
//       // 确保参数是正确的类型
//       if (!(error instanceof Error)) {
//         error = new Error(`Non-error exception: ${error}`);
//       }
//       
//       // 创建日志数据对象
//       const logData = {
//         id: uuidv4(),
//         timestamp: new Date().toISOString(),
//         level,
//         message,
//         error: {
//           name: error.name,
//           message: error.message,
//           stack: error.stack || 'No stack trace available',
//         },
//         context: {
//           ...context,
//           userAgent: navigator.userAgent,
//           platform: navigator.platform,
//           url: window.location.href,
//           timestamp: new Date().toISOString(),
//         },
//       };
//       
//       // 发送日志
//       mockSendErrorLog(logData);
//       
//       return logData.id;
//     } catch (loggingError) {
//       console.error('[errorLogService] 记录错误时发生错误:', loggingError);
//       return null;
//     }
//   },
//   
//   /**
//    * 设置全局错误处理器
//    * @param {Vue} app Vue应用实例
//    */
//   setupGlobalErrorHandler: (app) => {
//     if (!app || !app.config) {
//       console.error('[errorLogService] 无效的Vue应用实例');
//       return;
//     }
//     
//     // 设置Vue的错误处理
//     app.config.errorHandler = (err, vm, info) => {
//       const componentName = vm.$options.name || 'Anonymous';
//       const errorMessage = `Vue组件错误 [${componentName}]: ${err.message}`;
//       
//       errorLogService.addErrorLog(
//         err,
//         errorMessage,
//         'error',
//         {
//           component: componentName,
//           vueInfo: info,
//           props: vm.$props,
//           data: vm.$data
//         }
//       );
//       
//       // 调用原始的errorHandler（如果有）
//       const originalHandler = app.config.globalProperties.$originalErrorHandler;
//       if (typeof originalHandler === 'function') {
//         originalHandler.call(app, err, vm, info);
//       }
//     };
//   }
// };

// 导出服务
// export default errorLogService;

import { createApp } from 'vue'
import App from './App.vue'
import router from './router'
// 创建Pinia存储
import { createPinia } from 'pinia';
// 导入i18n模块
import { t, setLocale, getLocale } from './utils/i18n'

// 导入错误日志服务
import errorLogService from './services/errorLogService'

// 导入 Element Plus 及其样式
import ElementPlus from 'element-plus';
import 'element-plus/dist/index.css';

console.log('Starting to create Vue application');
console.log('App component:', App);
console.log('Router:', router);

// 创建Vue实例
const app = createApp(App)

// 将错误日志服务挂载到Vue原型上
app.config.globalProperties.$errorLogService = errorLogService

// 添加 Element Plus 插件
app.use(ElementPlus);

// 添加应用初始化调试信息
console.log('Application initializing with configuration:', {
  environment: process.env.NODE_ENV || 'development',
  version: process.env.VUE_APP_VERSION || '1.0.0',
  baseUrl: process.env.BASE_URL || '/',
  routerConfig: {
    mode: router.options.history.mode,
    routes: router.options.routes.map(route => ({
      path: route.path,
      name: route.name,
      component: route.component.name || 'LazyLoadedComponent'
    }))
  }
});

// 在应用挂载后添加调试信息
app.mixin({
  mounted() {
    console.log('Vue app mounted');
    console.log('Initial route:', router.currentRoute.value.path);
    console.log('Router options:', router.options);
  }
})

// 添加全局错误边界
app.config.errorHandler = (err, vm, info) => {
  // 增强错误信息输出，直接打印堆栈
  console.error(`Vue error in component ${vm?.$options?.name || 'unknown'}: ${err.message}`);
  console.error('Error stack:', err.stack);
  console.error('Error details:', {
    error: err,
    component: vm?.$options?.name,
    info,
    stack: err.stack
  });
  
  // 检查是否是500错误
  if (err.message.includes('500') || 
      (err.response && err.response.status === 500)) {
    console.error('检测到500服务器内部错误:', {
      message: err.message,
      stack: err.stack,
      response: err.response,
      request: err.request
    });
    
    // 使用原有的errorLogService记录500错误
    errorLogService.addErrorLog(
      err,
      `全局500错误: ${err.message}`,
      'error'
    );
    
    // 触发自定义事件以便App.vue可以处理
    if (window.$app && window.$app.$emit) {
      window.$app.$emit('server-error', {
        status: 500,
        message: '服务器内部错误: 服务暂时不可用',
        error: err
      });
    }
  }
  
  // 这里可以添加错误上报逻辑
  // sendErrorReport({
  //   error: err.message,
  //   stack: err.stack,
  //   component: vm?.$options?.name,
  //   info
  // })
}

// 添加全局未处理的Promise拒绝事件监听器
window.addEventListener('unhandledrejection', (event) => {
  console.groupCollapsed('Unhandled Promise Rejection')
  console.error('Event:', event)
  console.error('Reason:', event.reason)
  console.error('Stack trace:', event.reason?.stack)
  console.groupEnd()
  
  // 阻止默认处理
  event.preventDefault()
  
  // 这里可以添加错误上报
  // sendErrorReport({
  //   error: event.reason?.message || 'Unknown error',
  //   stack: event.reason?.stack,
  //   type: 'Promise Rejection'
  // })
})

// 添加性能监控 - 长任务检测
if (PerformanceObserver && PerformanceObserver.supportedEntryTypes.includes('longtask')) {
  const longTaskObserver = new PerformanceObserver((list) => {
    list.getEntries().forEach(entry => {
      if (entry.duration > 100) { // 超过100ms的任务
        console.warn(`Long task detected: ${entry.duration}ms`, {
          name: entry.name,
          startTime: entry.startTime,
          duration: entry.duration
        })
      }
    })
  })
  
  longTaskObserver.observe({ entryTypes: ['longtask'] })
}

// 添加性能监控 - 帧率检测
let lastFrameTime = performance.now()
let frameCount = 0
let fps = 0

function updateFPS() {
  frameCount++
  const now = performance.now()
  const delta = now - lastFrameTime
  
  if (delta >= 1000) {
    fps = Math.round((frameCount / delta) * 1000)
    frameCount = 0
    lastFrameTime = now
    
    // 输出帧率信息
    //console.log(`Current FPS: ${fps}`)
    
    // 如果帧率低于30，触发警告
    if (fps < 30) {
      console.warn(`Low FPS detected: ${fps}. Consider optimizing performance.`)
    }
  }
  
  requestAnimationFrame(updateFPS)
}

// 启动帧率监控
requestAnimationFrame(updateFPS)

// 添加性能监控 - 空闲资源检测
if ('requestIdleCallback' in window) {
  requestIdleCallback((deadline) => {
    console.log('Initial idle time:', deadline.timeRemaining())
    
    // 如果初始空闲时间小于50ms，提示可能需要优化
    if (deadline.timeRemaining() < 50) {
      console.warn('Initial idle time is less than 50ms, consider optimizing initial load')
    }
  }, { timeout: 1000 })
} else {
  console.warn('requestIdleCallback is not supported in this browser')
}

// 创建Pinia存储
const pinia = createPinia();

// 添加Element Plus插件
app.use(ElementPlus);

// 初始化错误日志服务
try {
  // 设置全局错误监听
  window.onerror = function(message, source, lineno, colno, error) {
    errorLogService.addErrorLog(
      error || new Error(message),
      `全局错误：${message} @ ${source}:${lineno}:${colno}`,
      'error'
    );
    return true; // 阻止默认处理
  };

  // 添加未处理的Promise拒绝监听
  window.addEventListener('unhandledrejection', (event) => {
    errorLogService.addErrorLog(
      event.reason,
      '未处理的Promise拒绝',
      'error'
    );
    // 阻止默认处理
    event.preventDefault();
  });

  console.log('[main] 错误日志服务初始化成功');
} catch (error) {
  console.error('[main] 初始化错误日志服务失败:', error);
}

// 先设置全局属性
app.config.globalProperties.$t = t;
app.config.globalProperties.$setLocale = setLocale;
app.config.globalProperties.$getLocale = getLocale;

console.log('Global properties set');

// 使用Pinia和路由
app.use(pinia);
console.log('Pinia registered');

app.use(router);
console.log('Router registered');

// 挂载应用
let appInstance
try {
  console.log('Starting to mount application');
  appInstance = app.mount('#app')
  console.log('Application mounted successfully', { appInstance });
  
  // 将app实例挂载到window对象（确保在挂载后）
  if (process.env.NODE_ENV !== 'production') {
    window.$app = appInstance
    console.log('App instance mounted to window.$app')
  }
} catch (error) {
  errorLogService.addErrorLog(error, '应用程序挂载失败', 'error');
  console.error('[main] 应用程序挂载失败:', error);
}

// 将app实例挂载到window对象（确保在挂载后）
if (process.env.NODE_ENV !== 'production') {
  window.$app = appInstance;
  console.log('App instance mounted to window.$app');
  
  // 添加详细调试信息
  console.groupCollapsed('Vue App Instance Details');
  console.log('Basic Info:', {
    isMounted: appInstance._isMounted,
    $options: appInstance.$options,
    $root: appInstance.$root,
    $children: appInstance.$children,
    $slots: appInstance.$slots
  });
  
  // 打印组件树
  function logComponentTree(vm, indent = 0) {
    const name = vm.$options.name || 'Anonymous'
    const uid = vm._uid
    const type = vm.$vnode ? 'Vue Component' : 'Root Instance'
    console.log(`${' '.repeat(indent)}- ${name} (${uid}) [${type}]`, {
      $options: vm.$options,
      $data: vm.$data,
      $props: vm.$props,
      $listeners: vm.$listeners
    })
    
    // 添加空值检查，避免访问 undefined 的 forEach
    if (vm.$children?.length > 0) {
      vm.$children.forEach(child => {
        logComponentTree(child, indent + 2)
      })
    }
  }
  
  console.log('Vue component tree:');
  logComponentTree(appInstance)
  console.groupEnd()
  
  // 添加Vue版本信息
  const app = createApp({});
  console.log(`Vue version: ${app.version}`);
  
  // 添加环境信息
  console.log('Environment Info:', {
    NODE_ENV: process.env.NODE_ENV,
    VUE_APP_ENV: process.env.VUE_APP_ENV,
    platform: process.env.PLATFORM,
    architecture: process.env.ARCH
  })
}

// 添加全局错误处理
window.onerror = function(message, source, lineno, colno, error) {
  console.groupCollapsed('Global Error');
  console.error('Message:', message);
  console.error('Source:', source);
  console.error('Line:', lineno, 'Column:', colno);
  console.error('Error object:', error);
  console.error('Stack trace:', error ? error.stack : 'N/A');
  console.groupEnd();
  
  // 可以在这里添加错误上报逻辑
  return true; // 阻止默认处理
};

// 添加Promise错误处理
window.onunhandledrejection = function(event) {
  console.groupCollapsed('Unhandled Promise Rejection');
  console.error('Reason:', event.reason);
  console.error('Stack trace:', event.reason ? event.reason.stack : 'N/A');
  console.error('Promise:', event.promise);
  console.groupEnd();
  
  event.preventDefault();
};


// 导出app实例以便其他模块可以使用
export { appInstance }