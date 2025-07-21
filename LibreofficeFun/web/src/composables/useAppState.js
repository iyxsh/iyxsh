import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'

/**
 * 应用全局状态管理
 * 处理全局加载状态、错误处理和应用重载
 */
export function useAppState() {
  // 状态管理
  const errorMessage = ref(null)
  const hasError = ref(false)
  const isLoaded = ref(false)
  const isRouterAlive = ref(true)
  const loadingProgress = ref(0)
  const loadingTimeout = ref(null)
  const loadingInterval = ref(null)

  const router = useRouter()

  // 计算状态
  const isLoading = computed(() => !isLoaded.value)
  const errorState = computed(() => ({
    hasError: hasError.value,
    errorMessage: errorMessage.value
  }))

  // 加载处理
  const startLoading = (timeout = 10000) => {
    // 清理之前的定时器
    if (loadingInterval.value) clearInterval(loadingInterval.value)
    if (loadingTimeout.value) clearTimeout(loadingTimeout.value)

    // 重置状态
    errorMessage.value = null
    hasError.value = false
    loadingProgress.value = 0

    // 开始加载进度
    loadingInterval.value = setInterval(() => {
      if (loadingProgress.value < 100) {
        loadingProgress.value += 1
      } else {
        clearInterval(loadingInterval.value)
        loadingInterval.value = null

        // 模拟加载完成后的操作
        setTimeout(() => {
          isLoaded.value = true
          console.log('应用加载完成')
        }, 500)
      }
    }, 20)

    // 添加超时机制
    loadingTimeout.value = setTimeout(() => {
      if (loadingInterval.value) {
        clearInterval(loadingInterval.value)
        loadingInterval.value = null
      }

      if (!isLoaded.value) {
        hasError.value = true
        errorMessage.value = '应用加载超时，请重试。'
        console.error('应用加载超时')
      }
    }, timeout)
  }

  // 加载完成处理
  const completeLoading = () => {
    if (loadingInterval.value) {
      clearInterval(loadingInterval.value)
      loadingInterval.value = null
    }

    if (loadingTimeout.value) {
      clearTimeout(loadingTimeout.value)
      loadingTimeout.value = null
    }

    loadingProgress.value = 100
    setTimeout(() => {
      isLoaded.value = true
    }, 200)
  }

  // 设置错误状态
  const setError = (message) => {
    errorMessage.value = message
    hasError.value = true
    isLoaded.value = true

    if (loadingInterval.value) {
      clearInterval(loadingInterval.value)
      loadingInterval.value = null
    }

    if (loadingTimeout.value) {
      clearTimeout(loadingTimeout.value)
      loadingTimeout.value = null
    }
  }

  // 重新加载应用
  const reloadApp = () => {
    isLoaded.value = false
    hasError.value = false
    errorMessage.value = null
    isRouterAlive.value = false

    // 强制刷新路由
    setTimeout(() => {
      isRouterAlive.value = true
      startLoading()
    }, 0)
  }

  // 重置应用到初始状态
  const resetApp = () => {
    reloadApp()
    router.push('/')
  }

  // 清理函数
  const cleanup = () => {
    if (loadingInterval.value) {
      clearInterval(loadingInterval.value)
      loadingInterval.value = null
    }

    if (loadingTimeout.value) {
      clearTimeout(loadingTimeout.value)
      loadingTimeout.value = null
    }
  }

  return {
    // 状态
    isLoaded,
    isLoading,
    hasError,
    errorMessage,
    loadingProgress,
    isRouterAlive,
    errorState,

    // 方法
    startLoading,
    completeLoading,
    setError,
    reloadApp,
    resetApp,
    cleanup
  }
}
