import { ref, onBeforeUnmount } from 'vue'

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
const eventBus = new EventBus()

/**
 * 组合式函数 - 使用事件总线
 * 在组件卸载时自动清理事件监听器
 */
export function useEventBus() {
  const listeners = ref([])

  /**
   * 注册事件监听器
   * @param {string} event - 事件名称
   * @param {Function} callback - 回调函数
   */
  const on = (event, callback) => {
    const id = eventBus.on(event, callback)
    listeners.value.push(id)
    return id
  }

  /**
   * 注册一次性事件监听器
   * @param {string} event - 事件名称
   * @param {Function} callback - 回调函数
   */
  const once = (event, callback) => {
    const id = eventBus.once(event, callback)
    listeners.value.push(id)
    return id
  }

  /**
   * 发送事件
   * @param {string} event - 事件名称
   * @param {...any} args - 传递给监听器的参数
   */
  const emit = (event, ...args) => {
    eventBus.emit(event, ...args)
  }

  /**
   * 取消特定事件监听
   * @param {string|number} eventOrId - 事件名称或ID
   * @param {Function} [callback] - 回调函数（可选）
   */
  const off = (eventOrId, callback) => {
    eventBus.off(eventOrId, callback)

    // 如果是ID，从listeners中移除
    if (typeof eventOrId === 'number') {
      const index = listeners.value.indexOf(eventOrId)
      if (index !== -1) {
        listeners.value.splice(index, 1)
      }
    }
  }

  /**
   * 清理所有当前组件注册的事件监听器
   */
  const cleanup = () => {
    listeners.value.forEach(id => {
      eventBus.off(id)
    })
    listeners.value = []
  }

  // 在组件卸载时自动清理
  onBeforeUnmount(cleanup)

  return {
    on,
    once,
    emit,
    off,
    cleanup
  }
}

// 导出事件总线单例供直接使用
export const bus = eventBus
