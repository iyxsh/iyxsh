import { ref, computed } from 'vue'

/**
 * 撤销/重做功能钩子
 * @param {Object} options 配置选项
 * @param {Function} options.onChange 状态变更回调函数
 * @param {number} options.maxHistory 最大历史记录数量
 * @returns {Object} 撤销/重做相关方法和状态
 */
export function useUndoRedo(options = {}) {
  const {
    onChange = () => {},
    maxHistory = 50
  } = options

  // 历史记录和当前位置
  const history = ref([])
  const currentIndex = ref(-1)

  // 计算属性
  const canUndo = computed(() => currentIndex.value > 0)
  const canRedo = computed(() => currentIndex.value < history.value.length - 1)

  /**
   * 添加新的状态到历史记录
   * @param {any} state 要添加的状态
   */
  const addState = (state) => {
    // 如果在历史记录中间做了修改，删除当前位置之后的记录
    if (currentIndex.value < history.value.length - 1) {
      history.value = history.value.slice(0, currentIndex.value + 1)
    }

    // 添加新状态
    history.value.push(JSON.parse(JSON.stringify(state)))

    // 如果历史记录超过最大数量，移除最早的记录
    if (history.value.length > maxHistory) {
      history.value.shift()
    } else {
      // 更新当前索引
      currentIndex.value += 1
    }
  }

  /**
   * 撤销操作
   */
  const undo = () => {
    if (!canUndo.value) return false

    currentIndex.value -= 1
    const state = history.value[currentIndex.value]
    onChange(JSON.parse(JSON.stringify(state)))

    return true
  }

  /**
   * 重做操作
   */
  const redo = () => {
    if (!canRedo.value) return false

    currentIndex.value += 1
    const state = history.value[currentIndex.value]
    onChange(JSON.parse(JSON.stringify(state)))

    return true
  }

  /**
   * 清除历史记录
   */
  const clearHistory = () => {
    history.value = []
    currentIndex.value = -1
  }

  /**
   * 初始化历史记录
   * @param {any} initialState 初始状态
   */
  const initHistory = (initialState) => {
    clearHistory()
    if (initialState !== undefined) {
      addState(initialState)
    }
  }

  /**
   * 获取历史记录
   * @returns {Array} 历史记录数组
   */
  const getHistory = () => {
    return history.value.map(item => JSON.parse(JSON.stringify(item)))
  }

  /**
   * 获取当前状态
   * @returns {any} 当前状态
   */
  const getCurrentState = () => {
    if (currentIndex.value >= 0 && currentIndex.value < history.value.length) {
      return JSON.parse(JSON.stringify(history.value[currentIndex.value]))
    }
    return null
  }

  return {
    // 状态
    history,
    currentIndex,
    canUndo,
    canRedo,

    // 方法
    addState,
    undo,
    redo,
    clearHistory,
    initHistory,
    getHistory,
    getCurrentState
  }
}
