// 使用Vue 3的Composition API创建事件总线
import { getCurrentInstance } from 'vue';
import { ref, onBeforeUnmount } from 'vue';

// 在文件顶部添加缺失的 listeners 定义
const listeners = new Map();

// 验证事件名称
function validateEventName(event) {
  if (typeof event !== 'string' || event.trim() === '') {
    throw new Error('Event name must be a non-empty string');
  }
}

/**
 * 创建全局事件总线
 * 优化后的事件总线支持命名空间，提供类型安全和自动清理
 */
class EventBus {
  constructor() {
    this.events = {}
    this.lastId = 0
  }

  /**
   * 注册事件监听器
   * @param {string} event - 事件名称
   * @param {Function} callback - 回调函数
   * @returns {number} 事件ID，用于取消监听
   */
  on(event, callback) {
    if (!this.events[event]) {
      this.events[event] = []
    }

    const id = ++this.lastId
    this.events[event].push({
      id,
      callback
    })

    return id
  }

  /**
   * 注册一次性事件监听器
   * @param {string} event - 事件名称
   * @param {Function} callback - 回调函数
   * @returns {number} 事件ID
   */
  once(event, callback) {
    const id = this.on(event, (...args) => {
      this.off(id)
      callback(...args)
    })

    return id
  }

  /**
   * 取消事件监听
   * @param {string|number} eventOrId - 事件名称或事件ID
   * @param {Function} [callback] - 回调函数（可选，如果提供则只移除特定回调）
   */
  off(eventOrId, callback) {
    // 如果传入的是数字ID
    if (typeof eventOrId === 'number') {
      const id = eventOrId
      // 遍历所有事件找到匹配的ID
      Object.keys(this.events).forEach(event => {
        this.events[event] = this.events[event].filter(listener => listener.id !== id)
        // 如果事件没有监听器了，删除事件
        if (this.events[event].length === 0) {
          delete this.events[event]
        }
      })
      return
    }

    // 如果传入的是事件名
    const event = eventOrId
    if (!this.events[event]) return

    // 如果没有提供回调，则移除该事件的所有监听器
    if (!callback) {
      delete this.events[event]
      return
    }

    // 如果提供了回调，则只移除匹配的监听器
    this.events[event] = this.events[event].filter(
      listener => listener.callback !== callback
    )

    // 如果事件没有监听器了，删除事件
    if (this.events[event].length === 0) {
      delete this.events[event]
    }
  }

  /**
   * 触发事件
   * @param {string} event - 事件名称
   * @param {...any} args - 传递给监听器的参数
   */
  emit(event, ...args) {
    if (!this.events[event]) return

    this.events[event].forEach(listener => {
      try {
        listener.callback(...args)
      } catch (error) {
        console.error(`Error in event listener for "${event}":`, error)
      }
    })
  }

  /**
   * 清除所有事件监听器
   */
  clear() {
    this.events = {}
  }
}

// 创建单例事件总线
const eventBus = new EventBus();

/**
 * 组合式函数 - 使用事件总线
 * 在组件卸载时自动清理事件监听器
 */
export function useEventBus() {
  const eventUnsubscribers = ref([]);
  const instance = getCurrentInstance();
  
  if (!instance) {
    throw new Error('useEventBus must be called within a setup() function or composition API');
  }
  
  // 将 vue 实例的 emit 方法重命名为 vueEmit 以避免与我们自己的 emit 方法冲突
  const { emit: vueEmit } = instance;
  
  // 添加调试日志
  const enableDebug = true; // 设置为true启用详细调试日志
  
  // 创建事件监听器存储
  const onceListeners = new Map();
  const componentListeners = new Map();
  
  // 添加事件监听器
  function on(event, callback, component = null) {
    validateEventName(event);
    if (!listeners.has(event)) {
      listeners.set(event, []);
    }
    listeners.get(event).push(callback);
    
    // 如果提供了组件上下文，将监听器与组件关联
    if (component) {
      if (!componentListeners.has(component)) {
        componentListeners.set(component, []);
      }
      componentListeners.get(component).push({ event, callback });
    }
    
    // 添加调试信息
    if (enableDebug) {
      const componentName = component?.type?.name || 'anonymous';
      console.log(`[EventBus] Event listener added: ${event}`);
      console.log(`[EventBus] Total listeners for ${event}: ${listeners.get(event).length}`);
      console.info(`[Component: ${componentName}] Subscribed to event: ${event}`);
      
      if (component) {
        console.debug(`[Component: ${componentName}] Listener count: ${componentListeners.get(component).length}`);
      }
    }
    
    // 返回取消订阅的函数
    const unsubscribe = () => {
      off(event, callback);
    };
    
    // 将取消订阅函数添加到eventUnsubscribers
    eventUnsubscribers.value.push(unsubscribe);
    
    return unsubscribe;
  }
  
  // 移除事件监听
  function off(event, callback) {
    // 从普通监听器中移除
    if (listeners.has(event)) {
      listeners.set(
        event,
        listeners.get(event).filter(cb => cb !== callback)
      );
    }
    
    // 从组件监听器中移除
    componentListeners.forEach((componentEvents, component) => {
      componentEvents.forEach((componentEvent, index) => {
        if (componentEvent.event === event && componentEvent.callback === callback) {
          componentEvents.splice(index, 1);
        }
      });
      
      // 如果组件没有更多事件监听器，移除该组件
      if (componentEvents.length === 0) {
        componentListeners.delete(component);
      }
    });
  }
  
  // 添加一次性监听器
  function once(event, callback, component = null) {
    validateEventName(event);
    const onceCallback = (...args) => {
      callback(...args);
      off(event, onceCallback);
      
      // 添加调试信息
      if (enableDebug) {
        console.log(`[EventBus] Once event listener executed and removed: ${event}`);
      }
    };
    
    if (!onceListeners.has(event)) {
      onceListeners.set(event, []);
    }
    onceListeners.get(event).push(onceCallback);
    
    // 如果提供了组件上下文，将监听器与组件关联
    if (component) {
      if (!componentListeners.has(component)) {
        componentListeners.set(component, []);
      }
      componentListeners.get(component).push({ event, callback: onceCallback });
    }
    
    // 添加调试信息
    if (enableDebug) {
      const componentName = component?.type?.name || 'anonymous';
      console.log(`[EventBus] Once event listener added: ${event}`);
      console.log(`[EventBus] Total once listeners for ${event}: ${onceListeners.get(event).length}`);
      console.debug(`[Component: ${componentName}] Subscribed once to event: ${event}`);
    }
    
    // 返回取消订阅的函数
    return () => {
      off(event, onceCallback);
    };
  }
  
  // 触发事件
  function emit(event, ...args) {
    validateEventName(event);
    const currentCount = listeners.has(event) ? listeners.get(event).length : 0;
    const onceCount = onceListeners.has(event) ? onceListeners.get(event).length : 0;
    
    // 触发普通监听器
    if (listeners.has(event)) {
      listeners.get(event).forEach(callback => {
        try {
          callback(...args);
        } catch (error) {
          console.error(`[EventBus] Error in event listener for ${event}:`, error);
        }
      });
    }
    
    // 触发一次性监听器
    if (onceListeners.has(event)) {
      const callbacks = [...onceListeners.get(event)];
      onceListeners.delete(event);
      callbacks.forEach(callback => {
        try {
          callback(...args);
        } catch (error) {
          console.error(`[EventBus] Error in once event listener for ${event}:`, error);
        }
      });
    }
    
    // 添加调试信息
    if (enableDebug) {
      console.log(`[EventBus] Event emitted: ${event}`, {
        normalListeners: currentCount,
        onceListeners: onceCount,
        args
      });
      
      // 检查是否有组件监听这个事件
      componentListeners.forEach((componentEvents, component) => {
        const componentName = component?.type?.name || 'anonymous';
        const hasListener = componentEvents.some(evt => evt.event === event);
        
        if (hasListener) {
          console.info(`[Component: ${componentName}] is listening for event: ${event}`);
        }
      });
    }
    
    // 触发全局事件
    window.dispatchEvent(new CustomEvent(`app-event:${event}`, {
      detail: args
    }));
  }
  
  // 添加批量触发事件的方法
  function batchTrigger(events) {
    events.forEach(({ event, payload }) => {
      trigger(event, payload);
    });
  }
  
  // 监听全局事件
  function onGlobalEvent(event, callback) {
    const globalCallback = (e) => {
      callback(...e.detail);
    };
    
    window.addEventListener(`app-event:${event}`, globalCallback);
    
    // 返回一个函数用于移除监听器
    return () => {
      window.removeEventListener(`app-event:${event}`, globalCallback);
    };
  }
  
  // 移除所有监听器
  function removeAllListeners() {
    listeners.forEach((value, key) => listeners.delete(key));
    onceListeners.forEach((value, key) => onceListeners.delete(key));
    componentListeners.forEach((value, key) => componentListeners.delete(key));
  }
  
  // 清理组件相关的监听器
  function cleanupComponentListeners(component) {
    if (componentListeners.has(component)) {
      componentListeners.get(component).forEach(({ event, callback }) => {
        off(event, callback);
      });
      componentListeners.delete(component);
    }
  }
  
  // 获取当前监听器数量
  function getListenerCount(event) {
    const count = listeners.has(event) ? listeners.get(event).length : 0;
    const onceCount = onceListeners.has(event) ? onceListeners.get(event).length : 0;
    return count + onceCount;
  }
  
  // 添加内存泄漏检查功能
  function checkMemoryUsage() {
    const totalListeners = Array.from(listeners.entries()).reduce((sum, [_, callbacks]) => sum + callbacks.length, 0);
    const totalOnceListeners = Array.from(onceListeners.entries()).reduce((sum, [_, callbacks]) => sum + callbacks.length, 0);
    const totalComponentListeners = Array.from(componentListeners.entries()).reduce((sum, [_, componentEvents]) => {
      return sum + componentEvents.length;
    }, 0);
    
    const total = totalListeners + totalOnceListeners + totalComponentListeners;
    
    // 如果监听器数量过多，触发警告
    if (total > 100) {
      console.warn(`[EventBus] Too many listeners (${total}), consider cleaning up`);
      
      // 检查是否有内存泄漏
      checkForMemoryLeaks();
    }
    
    // 添加调试信息
    if (enableDebug) {
      console.log('[EventBus] Memory usage check', {
        normalListeners: totalListeners,
        onceListeners: totalOnceListeners,
        componentListeners: totalComponentListeners,
        total
      });
    }
    
    // 检查内存使用情况
    if (performance && performance.memory) {
      const memoryUsage = performance.memory;
      const memoryInfo = {
        usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
        totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
        jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit),
        utilization: (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit * 100).toFixed(2) + '%'
      };
      
      // 添加调试信息
      if (enableDebug) {
        console.log('[EventBus] Memory Usage:', memoryInfo);
      }
      
      // 如果内存使用超过阈值，触发警告
      if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
        console.warn('[EventBus] Memory usage is high, consider optimizing', memoryInfo);
      }
    }
  }
  
  // 检查内存泄漏
  function checkForMemoryLeaks() {
    const now = Date.now();
    
    // 检查长时间存在的监听器
    listeners.forEach((callbacks, event) => {
      callbacks.forEach(callback => {
        if (callback._attachedAt && now - callback._attachedAt > 5 * 60 * 1000) {
          console.warn(`[EventBus] Listener for ${event} has been attached for more than 5 minutes`);
          
          if (enableDebug) {
            const attachedTime = new Date(callback._attachedAt).toLocaleTimeString();
            console.info(`[EventBus] Listener attached at: ${attachedTime}`, {
              event,
              callback: callback.toString().substring(0, 100) + '...'
            });
          }
        }
      });
    });
    
    // 检查组件监听器
    componentListeners.forEach((componentEvents, component) => {
      const componentName = component?.type?.name || 'anonymous';
      
      if (!component._isDestroyed) {
        console.warn(`[EventBus] Component ${componentName} is not destroyed but still has listeners`);
        
        if (enableDebug) {
          console.debug(`[Component: ${componentName}] Active listeners:`, componentEvents.map(e => e.event));
        }
      }
    });
  }
  
  // 格式化字节大小
  function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }
  
  // 添加内存泄漏检查间隔
  let memoryCheckInterval = null;
  
  function setupMemoryLeakCheck() {
    if (!memoryCheckInterval) {
      memoryCheckInterval = setInterval(() => {
        checkMemoryUsage();
      }, 5 * 60 * 1000); // 每5分钟检查一次
    }
  }
  
  // 停止内存泄漏检查
  function stopMemoryLeakCheck() {
    if (memoryCheckInterval) {
      clearInterval(memoryCheckInterval);
      memoryCheckInterval = null;
    }
  }
  
  // 添加资源清理
  const cleanupTasks = [];
  
  // 添加清理任务
  cleanupTasks.push(stopMemoryLeakCheck);
  
  // 在组件卸载前执行清理
  function cleanupResources() {
    // 执行清理任务
    cleanupTasks.forEach(task => task());
    
    // 清理所有监听器
    removeAllListeners();
    
    // 清理组件监听器
    componentListeners.forEach((componentEvents, component) => {
      componentListeners.delete(component);
    });
  }
  
  // 在组件卸载前执行清理
  function beforeUnmount(component) {
    cleanupComponentListeners(component);
  }
  
  // 在组件卸载时自动清理监听器
  function onUnmounted(callback) {
    const instance = getCurrentInstance();
    
    if (instance) {
      instance.proxy.$onBeforeUnmount(() => {
        callback();
      });
    }
  }

  // 添加事件命名规范检查
  function validateEventName(event) {
    if (typeof event !== 'string' || event.trim() === '') {
      throw new Error('Event name must be a non-empty string');
    }
  }

  // 返回事件总线方法
  return {
    on,
    off,
    once,
    emit,
    batchTrigger,
    onGlobalEvent,
    // 用于切换调试模式的方法
    setDebugMode(debugMode) {
      enableDebug = debugMode;
      console.log(`EventBus debug mode ${enableDebug ? 'enabled' : 'disabled'}`);
    }
  };
}