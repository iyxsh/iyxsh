<template>
  <div class="app-container">
    <!-- 调试信息显示控制 -->
    <div class="debug-control">
      <button @click="toggleDebugInfo">
        {{ showDebugInfo ? '隐藏' : '显示' }}调试信息
      </button>
    </div>

    <!-- 应用状态信息显示 -->
    <div v-if="showDebugInfo" class="debug-info">
      <h3>应用状态</h3>
      <ul>
        <li>当前路由: {{ currentRoute?.path }}</li>
        <li>路由名称: {{ currentRoute?.name }}</li>
        <li>已加载: {{ isLoaded }}</li>
        <li>错误状态: {{ hasError }}</li>
        <li>加载进度: {{ loadingProgress }}%</li>
        <li>语言: {{ locale }}</li>
      </ul>
    </div>

    <!-- 加载状态 -->
    <AppLoading 
      v-if="!isLoaded" 
      :progress="loadingProgress" 
      :has-error="hasError"
      :error-message="errorMessage"
      @reload="reloadApp"
    />

    <!-- 应用内容 -->
    <div v-else>
      <div v-if="!hasError">
        <router-view v-if="isRouterAlive" v-slot="{ Component }">
          <component 
            :is="Component" 
            @load="onComponentLoad" 
            @error.capture="handleComponentError" 
          />
        </router-view>
      </div>
      <AppError 
        v-else 
        :message="errorMessage" 
        @reload="reloadApp" 
      />
    </div>
  </div>
</template>

<script>
import { ref, onMounted, onErrorCaptured, onUpdated, onBeforeUnmount, computed } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { t, setLocale, getLocale } from './utils/i18n';
import AppLoading from './components/AppLoading.vue';
import AppError from './components/AppError.vue';
import errorLogService from './services/errorLogService';
export default {
  name: 'App',
  components: {
    AppLoading,
    AppError
  },
  setup() {
    // 状态管理
    const locale = ref(getLocale());
    const errorMessage = ref(null);
    const hasError = ref(false);
    const isLoaded = ref(false);
    const isRouterAlive = ref(true);
    const loadingProgress = ref(0);
    const showDebugInfo = ref(false);

    // 路由相关
    const route = useRoute();
    const router = useRouter();

    // 使用computed计算属性获取当前路由，确保响应式更新
    const currentRoute = computed(() => route);

    // 增强的加载状态管理
    const startLoading = () => {
      // 重置状态
      errorMessage.value = null;
      hasError.value = false;
      
      // 开始加载进度
      loadingProgress.value = 0;
      const interval = setInterval(() => {
        if (loadingProgress.value < 100) {
          loadingProgress.value += 1;
        } else {
          clearInterval(interval);
          // 模拟加载完成后的操作
          setTimeout(() => {
            isLoaded.value = true;
            console.log('应用加载完成');
          }, 500);
        }
      }, 20);
      
      // 添加超时机制
      setTimeout(() => {
        clearInterval(interval);
        if (!isLoaded.value) {
          hasError.value = true;
          errorMessage.value = '应用加载超时，请重试。';
          console.error('应用加载超时');
          
          // 触发500错误处理逻辑
          handleComponentError({
            message: '应用加载超时 (504)',
            status: 504,
            stack: 'Application loading timeout'
          });
        }
      }, 10000);
    };

    // 组件加载完成时的处理
    const onComponentLoad = () => {
      console.log('组件加载完成');
      isLoaded.value = true;
    };

    // 增强的错误处理
    const handleComponentError = (error) => {
      console.error('组件错误:', error);
      
      // 特殊处理媒体相关错误
      if (error && error.type === 'error' && error.target) {
        // 检查是否是媒体加载错误
        const target = error.target;
        if (target.tagName && (target.tagName === 'IMG' || target.tagName === 'VIDEO')) {
          console.log('媒体加载错误，可能是base64格式视频导致的性能问题');
          // 不将媒体加载错误视为应用级错误，完全阻止传播
          return true; // 返回true表示已处理并不再传播
        }
      }
      
      // 增强的错误处理逻辑
      if (error && error.message) {
        // 检查是否是500错误或服务器错误
        if (error.message.includes('500') || error.message.includes('server') || 
            (error.response && error.response.status === 500)) {
          errorMessage.value = `服务器内部错误 (500): 服务暂时不可用，请稍后重试。`;
          hasError.value = true;
          isLoaded.value = true; // 停止加载状态
          
          // 记录错误日志
          errorLogService.addErrorLog(
            error,
            `服务器错误 (500): ${error.message}`,
            'error'
          );
          
          // 显示详细的错误信息
          console.error('服务器错误详情:', {
            message: error.message,
            stack: error.stack,
            response: error.response,
            request: error.request
          });
          
          // 添加重试机制
          setTimeout(() => {
            if (confirm('服务器暂时不可用，是否立即重试？')) {
              reloadApp();
            }
          }, 3000);
          
          return false; // 阻止错误继续传播
        }
      }
      
      // 默认错误处理
      errorMessage.value = `加载组件时发生错误: ${error.message}`;
      hasError.value = true;
      isLoaded.value = true; // 停止加载状态
      
      // 显示更详细的错误信息
      if (error.stack) {
        console.error('错误堆栈:', error.stack);
      }
      
      // 添加错误类型检查
      if (error.name === 'TypeError' && error.message.includes('text')) {
        errorMessage.value += ' - 请确保组件中的文本属性已正确初始化';
      }
      
      // 记录其他类型的错误
      errorLogService.addErrorLog(
        error,
        `组件错误: ${error.message}`,
        'error'
      );
      
      return false; // 阻止错误继续传播
    };

    // 修改 reloadApp 方法，添加错误状态清理
    const reloadApp = () => {
      isLoaded.value = false;
      hasError.value = false;
      errorMessage.value = null;
      isRouterAlive.value = false;
      // 强制刷新路由
      setTimeout(() => {
        isRouterAlive.value = true;
        startLoading();
      }, 0);
    };

    // 切换调试信息显示
    const toggleDebugInfo = () => {
      showDebugInfo.value = !showDebugInfo.value;
      console.log(`调试信息${showDebugInfo.value ? '开启' : '关闭'}`);
    };

    // 错误捕获
    onErrorCaptured((err) => {
      console.error('App.vue 捕获到错误:', err);
      
      // 处理500错误
      if (err.message.includes('500') || 
          (err.response && err.response.status === 500)) {
        errorMessage.value = '服务器内部错误 (500): 服务暂时不可用，请稍后重试。';
        
        // 记录500错误
        errorLogService.addErrorLog(
          err,
          `全局500错误: ${err.message}`,
          'error'
        );
      } else {
        errorMessage.value = '发生意外错误，请尝试重新加载应用。';
        
        // 记录其他错误
        errorLogService.addErrorLog(
          err,
          `未处理的错误: ${err.message}`,
          'error'
        );
      }
      
      hasError.value = true;
      isLoaded.value = true; // 停止加载状态
      return false; // 阻止错误继续传播
    });

    // 生命周期钩子
    onMounted(() => {
      console.log('App 组件已挂载');
      console.log('路由当前路径:', router.currentRoute.value.path);
      startLoading();
      
      // 添加全局500错误监听
      if (window.$app && window.$app.$on) {
        window.$app.$on('server-error', (error) => {
          if (error.status === 500) {
            console.error('全局500错误:', error);
            errorMessage.value = '服务器内部错误 (500): 服务暂时不可用，请稍后重试。';
            hasError.value = true;
            isLoaded.value = true; // 停止加载状态
            
            // 记录错误日志
            errorLogService.addErrorLog(
              error.error,
              `全局500错误: ${error.message}`,
              'error'
            );
            
            // 显示详细的错误信息
            console.error('500错误详情:', {
              message: error.message,
              error: error.error,
              stack: error.error?.stack
            });
            
            // 添加重试机制
            setTimeout(() => {
              if (confirm('服务器暂时不可用，是否立即重试？')) {
                reloadApp();
              }
            }, 3000);
          }
        });
      }
    });

    onUpdated(() => {
      console.log('App 组件已更新');
      console.log('当前路由:', route.path);
      console.log('当前语言:', locale.value);
    });

    onBeforeUnmount(() => {
      console.log('App 组件即将卸载');
    });

    // 设置语言
    const setLocaleWrapper = (lang) => {
      locale.value = lang;
      setLocale(lang);
      localStorage.setItem('locale', lang);
    };

    // 添加全局Promise错误处理
    window.addEventListener('unhandledrejection', (event) => {
      console.groupCollapsed('Unhandled Promise Rejection');
      console.error('Event:', event);
      console.error('Reason:', event.reason);
      console.error('Stack trace:', event.reason?.stack);
      console.groupEnd();
      
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
      
      // 阻止默认处理
      event.preventDefault();
      
      // 显示错误信息
      errorMessage.value = `Unhandled Promise Rejection: ${event.reason?.message || 'Unknown error'}`;
      hasError.value = true;
      isLoaded.value = true;
      
      // 使用现有的errorLogService记录错误，替代未定义的sendErrorReport函数
      errorLogService.addErrorLog(
        event.reason || new Error('Unknown Promise rejection'),
        '未处理的Promise拒绝',
        'error'
      );
    });

    return {
      // 状态
      errorMessage,
      hasError,
      isLoaded,
      isRouterAlive,
      loadingProgress,
      showDebugInfo,
      locale,
      currentRoute,

      // 方法
      toggleDebugInfo,
      reloadApp,
      onComponentLoad,
      handleComponentError,
      setLocale: setLocaleWrapper
    };
  }
};
</script>

<style scoped>
.app-container {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.debug-info {
  position: fixed;
  top: 0;
  left: 0;
  background: rgba(0, 0, 0, 0.85);
  color: white;
  padding: 15px;
  z-index: 1000;
  font-size: 14px;
  max-width: 350px;
  border-radius: 4px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
}

.debug-info ul {
  padding-left: 20px;
  list-style-type: disc;
  margin: 0;
}

.debug-info li {
  margin: 4px 0;
}

.debug-info h3 {
  margin-top: 0;
  margin-bottom: 10px;
  font-size: 16px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.2);
  padding-bottom: 5px;
}

/* 调试控制样式 */
.debug-control {
  position: fixed;
  top: 10px;
  right: 10px;
  z-index: 999;
}

.debug-control button {
  padding: 8px 12px;
  background-color: #42b883;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;
}

.debug-control button:hover {
  background-color: #369767;
}
</style>