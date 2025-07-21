/**
 * 性能监控工具
 * 用于监控和优化应用性能
 */

// 监控选项
const defaultOptions = {
  enableFpsMonitor: true,       // 启用FPS监控
  enableMemoryMonitor: true,    // 启用内存监控
  enableNetworkMonitor: true,   // 启用网络监控
  enableLongTaskMonitor: true,  // 启用长任务监控
  longTaskThreshold: 50,        // 长任务阈值(ms)
  lowFpsThreshold: 30,          // 低帧率阈值
  memoryWarningThreshold: 0.7,  // 内存警告阈值(占总内存比例)
  sampleInterval: 2000,         // 采样间隔(ms)
  maxSamples: 100,              // 最大样本数
  autoStart: false,             // 自动启动
  debug: false                  // 调试模式
}

// 性能数据
const performanceData = {
  fps: {
    current: 0,
    history: [],
    min: Infinity,
    max: 0,
    avg: 0
  },
  memory: {
    current: {
      used: 0,
      total: 0,
      limit: 0,
      percentage: 0
    },
    history: [],
    peak: 0
  },
  longTasks: {
    count: 0,
    history: [],
    totalDuration: 0,
    maxDuration: 0
  },
  network: {
    requests: {
      total: 0,
      success: 0,
      failed: 0,
      pending: 0
    },
    history: [],
    totalTransferred: 0
  },
  startTime: 0,
  monitorActive: false
}

// 监听器和定时器
let fpsFrameCount = 0
let lastFrameTime = 0
let fpsUpdateInterval = null
let memoryUpdateInterval = null
let longTaskObserver = null
let networkObserver = null
let rafId = null

/**
 * 初始化性能监控
 * @param {Object} options - 监控选项
 */
export function initPerformanceMonitor(options = {}) {
  const config = { ...defaultOptions, ...options }

  // 启动监控
  if (config.autoStart) {
    startMonitoring(config)
  }

  return {
    start: () => startMonitoring(config),
    stop: stopMonitoring,
    getPerformanceData: () => ({ ...performanceData }),
    isActive: () => performanceData.monitorActive
  }
}

/**
 * 启动监控
 * @param {Object} options - 监控选项
 */
function startMonitoring(options = defaultOptions) {
  if (performanceData.monitorActive) return

  performanceData.monitorActive = true
  performanceData.startTime = performance.now()

  // 重置数据
  resetPerformanceData()

  // 启动FPS监控
  if (options.enableFpsMonitor) {
    startFpsMonitoring()
  }

  // 启动内存监控
  if (options.enableMemoryMonitor && window.performance && performance.memory) {
    startMemoryMonitoring(options.sampleInterval)
  }

  // 启动长任务监控
  if (options.enableLongTaskMonitor && window.PerformanceObserver && PerformanceObserver.supportedEntryTypes.includes('longtask')) {
    startLongTaskMonitoring(options.longTaskThreshold)
  }

  // 启动网络监控
  if (options.enableNetworkMonitor && window.PerformanceObserver && PerformanceObserver.supportedEntryTypes.includes('resource')) {
    startNetworkMonitoring()
  }

  if (options.debug) {
    console.log('Performance monitoring started', options)
  }
}

/**
 * 停止监控
 */
function stopMonitoring() {
  if (!performanceData.monitorActive) return

  performanceData.monitorActive = false

  // 停止FPS监控
  if (rafId) {
    cancelAnimationFrame(rafId)
    rafId = null
  }

  // 停止内存监控
  if (memoryUpdateInterval) {
    clearInterval(memoryUpdateInterval)
    memoryUpdateInterval = null
  }

  // 停止长任务监控
  if (longTaskObserver) {
    longTaskObserver.disconnect()
    longTaskObserver = null
  }

  // 停止网络监控
  if (networkObserver) {
    networkObserver.disconnect()
    networkObserver = null
  }

  console.log('Performance monitoring stopped')
}

/**
 * 重置性能数据
 */
function resetPerformanceData() {
  performanceData.fps = {
    current: 0,
    history: [],
    min: Infinity,
    max: 0,
    avg: 0
  }

  performanceData.memory = {
    current: {
      used: 0,
      total: 0,
      limit: 0,
      percentage: 0
    },
    history: [],
    peak: 0
  }

  performanceData.longTasks = {
    count: 0,
    history: [],
    totalDuration: 0,
    maxDuration: 0
  }

  performanceData.network = {
    requests: {
      total: 0,
      success: 0,
      failed: 0,
      pending: 0
    },
    history: [],
    totalTransferred: 0
  }
}

/**
 * 启动FPS监控
 */
function startFpsMonitoring() {
  lastFrameTime = performance.now()
  fpsFrameCount = 0

  // 定期更新FPS
  fpsUpdateInterval = setInterval(() => {
    const now = performance.now()
    const elapsed = now - lastFrameTime

    if (elapsed >= 1000) {
      const fps = Math.round((fpsFrameCount * 1000) / elapsed)

      // 更新FPS数据
      performanceData.fps.current = fps
      performanceData.fps.history.push({
        time: now,
        value: fps
      })

      // 限制历史记录长度
      if (performanceData.fps.history.length > defaultOptions.maxSamples) {
        performanceData.fps.history.shift()
      }

      // 更新统计信息
      performanceData.fps.min = Math.min(performanceData.fps.min, fps)
      performanceData.fps.max = Math.max(performanceData.fps.max, fps)

      // 计算平均FPS
      const sum = performanceData.fps.history.reduce((acc, item) => acc + item.value, 0)
      performanceData.fps.avg = Math.round(sum / performanceData.fps.history.length)

      // 检测低帧率
      if (fps < defaultOptions.lowFpsThreshold) {
        console.warn(`Low FPS detected: ${fps}`)

        // 这里可以添加性能诊断代码
        if (performance.memory) {
          console.warn('Memory usage:', {
            used: formatBytes(performance.memory.usedJSHeapSize),
            total: formatBytes(performance.memory.totalJSHeapSize),
            limit: formatBytes(performance.memory.jsHeapSizeLimit)
          })
        }
      }

      // 重置计数器
      fpsFrameCount = 0
      lastFrameTime = now
    }
  }, 1000)

  // 使用requestAnimationFrame计数帧
  function countFrame() {
    fpsFrameCount++
    rafId = requestAnimationFrame(countFrame)
  }

  rafId = requestAnimationFrame(countFrame)
}

/**
 * 启动内存监控
 * @param {number} interval - 采样间隔
 */
function startMemoryMonitoring(interval) {
  memoryUpdateInterval = setInterval(() => {
    if (!performance.memory) return

    const memory = performance.memory
    const used = memory.usedJSHeapSize
    const total = memory.totalJSHeapSize
    const limit = memory.jsHeapSizeLimit
    const percentage = used / limit

    // 更新内存数据
    performanceData.memory.current = {
      used,
      total,
      limit,
      percentage
    }

    performanceData.memory.history.push({
      time: performance.now(),
      used,
      total,
      percentage
    })

    // 限制历史记录长度
    if (performanceData.memory.history.length > defaultOptions.maxSamples) {
      performanceData.memory.history.shift()
    }

    // 更新峰值
    performanceData.memory.peak = Math.max(performanceData.memory.peak, used)

    // 内存使用警告
    if (percentage > defaultOptions.memoryWarningThreshold) {
      console.warn(`High memory usage: ${(percentage * 100).toFixed(1)}% of limit`, {
        used: formatBytes(used),
        total: formatBytes(total),
        limit: formatBytes(limit)
      })
    }
  }, interval)
}

/**
 * 启动长任务监控
 * @param {number} threshold - 长任务阈值(ms)
 */
function startLongTaskMonitoring(threshold) {
  longTaskObserver = new PerformanceObserver((entryList) => {
    entryList.getEntries().forEach(entry => {
      if (entry.duration > threshold) {
        performanceData.longTasks.count++
        performanceData.longTasks.totalDuration += entry.duration
        performanceData.longTasks.maxDuration = Math.max(performanceData.longTasks.maxDuration, entry.duration)

        performanceData.longTasks.history.push({
          time: performance.now(),
          duration: entry.duration,
          name: entry.name,
          startTime: entry.startTime
        })

        // 限制历史记录长度
        if (performanceData.longTasks.history.length > defaultOptions.maxSamples) {
          performanceData.longTasks.history.shift()
        }

        console.warn(`Long task detected: ${entry.duration.toFixed(2)}ms`, {
          name: entry.name,
          startTime: entry.startTime,
          attribution: entry.attribution
        })
      }
    })
  })

  longTaskObserver.observe({ entryTypes: ['longtask'] })
}

/**
 * 启动网络监控
 */
function startNetworkMonitoring() {
  networkObserver = new PerformanceObserver((entryList) => {
    entryList.getEntries().forEach(entry => {
      // 只处理XHR和fetch请求
      if (entry.initiatorType === 'xmlhttprequest' || entry.initiatorType === 'fetch') {
        const size = entry.transferSize || 0
        const success = entry.responseStatus === 200 || entry.responseStatus === 304

        // 更新网络请求统计
        performanceData.network.requests.total++
        if (success) {
          performanceData.network.requests.success++
        } else {
          performanceData.network.requests.failed++
        }

        performanceData.network.totalTransferred += size

        // 添加到历史记录
        performanceData.network.history.push({
          time: performance.now(),
          url: entry.name,
          duration: entry.duration,
          size,
          type: entry.initiatorType,
          status: entry.responseStatus,
          success
        })

        // 限制历史记录长度
        if (performanceData.network.history.length > defaultOptions.maxSamples) {
          performanceData.network.history.shift()
        }

        // 记录慢请求
        if (entry.duration > 1000) {
          console.warn(`Slow network request: ${entry.duration.toFixed(2)}ms for ${entry.name}`)
        }
      }
    })
  })

  networkObserver.observe({ entryTypes: ['resource'] })
}

/**
 * 格式化字节大小
 * @param {number} bytes - 字节数
 * @returns {string} 格式化后的字符串
 */
function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes'

  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))

  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

// 创建默认监控实例
export const performanceMonitor = initPerformanceMonitor()

// 导出工具对象
export default performanceMonitor
