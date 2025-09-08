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

// 导入 Element Plus 图标
import * as ElementPlusIconsVue from '@element-plus/icons-vue';

// 创建Vue实例
const app = createApp(App)

// 将错误日志服务挂载到Vue原型上
app.config.globalProperties.$errorLogService = errorLogService

// 添加 Element Plus 插件
app.use(ElementPlus);

// 全局注册 Element Plus 图标
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component);
}

// 使用 Vue Router
app.use(router);

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
}

// 创建Pinia存储
const pinia = createPinia();

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
    // 特别处理消息端口关闭的错误
    if (event.reason && typeof event.reason === 'object') {
      if (event.reason.message && 
          event.reason.message.includes('The message port closed before a response was received')) {
        console.warn('检测到消息端口关闭错误，这可能是一个Chrome扩展相关的问题，不会影响应用功能');
        // 阻止该特定错误的默认处理
        event.preventDefault();
        return;
      }
      
      // 处理其他特定的命令错误
      if (event.reason.cmd) {
        console.warn('检测到命令执行错误:', event.reason.cmd);
        // 根据命令类型决定是否阻止默认处理
        event.preventDefault();
        return;
      }
    }
    
    errorLogService.addErrorLog(
      event.reason,
      '未处理的Promise拒绝',
      'error'
    );
  });
} catch (error) {
  console.error('[main] 初始化错误日志服务失败:', error);
}

// 挂载应用
let appInstance
try {
  appInstance = app.mount('#app')
  
  // 将app实例挂载到window对象（确保在挂载后）
  if (process.env.NODE_ENV !== 'production') {
    window.$app = appInstance
  }
  
  // 在应用挂载后，可扩展其他全局初始化逻辑
  appInstance.$nextTick(() => {
    // ...existing code...
  })

  // 注册 Service Worker，实现 PWA 支持
  if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
      navigator.serviceWorker.register('/service-worker.js')
        .then(reg => {
          console.log('Service Worker 注册成功:', reg);
        })
        .catch(err => {
          console.error('Service Worker 注册失败:', err);
        });
    });
  }
} catch (error) {
  errorLogService.addErrorLog(error, '应用程序挂载失败', 'error');
  console.error('[main] 应用程序挂载失败:', error);
}

// 导出app实例以便其他模块可以使用
export { appInstance }