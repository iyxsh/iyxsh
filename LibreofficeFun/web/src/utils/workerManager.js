/**
 * Web Worker管理器
 * 用于管理和使用Web Worker进行后台计算
 */

// 存储处理中的请求和回调
const pendingRequests = new Map()

// 当前请求ID
let requestId = 0

// Worker实例
let worker = null

// 是否初始化完成
let isInitialized = false

// 初始化Worker
function initWorker() {
  if (worker) return Promise.resolve(worker)

  return new Promise((resolve, reject) => {
    try {
      // 创建worker实例
      worker = new Worker(new URL('../workers/data-processor.worker.js', import.meta.url), { type: 'module' })

      // 处理worker消息
      worker.addEventListener('message', (event) => {
        const { id, success, result, error } = event.data

        // 初始化完成消息
        if (id === 'init') {
          isInitialized = true
          console.log('Worker initialized:', result)
          resolve(worker)
          return
        }

        // 查找对应的请求
        const request = pendingRequests.get(id)
        if (!request) return

        // 执行回调
        const { resolve: resolveRequest, reject: rejectRequest } = request

        if (success) {
          resolveRequest(result)
        } else {
          rejectRequest(new Error(error.message))
        }

        // 删除已完成的请求
        pendingRequests.delete(id)
      })

      // 处理worker错误
      worker.addEventListener('error', (error) => {
        console.error('Worker error:', error)
        reject(error)
      })

    } catch (error) {
      console.error('Failed to initialize worker:', error)
      reject(error)
    }
  })
}

/**
 * 发送任务到Worker
 * @param {string} action - 操作类型
 * @param {Object} data - 任务数据
 * @returns {Promise} - 任务结果
 */
export async function sendToWorker(action, data) {
  if (!isInitialized) {
    await initWorker()
  }

  // 生成唯一请求ID
  const id = requestId++

  // 创建Promise
  const promise = new Promise((resolve, reject) => {
    pendingRequests.set(id, { resolve, reject })
  })

  // 发送消息到Worker
  worker.postMessage({
    id,
    action,
    data
  })

  return promise
}

/**
 * 计算表单统计信息
 * @param {Array} forms - 表单数据
 * @returns {Promise} - 统计结果
 */
export function calculateFormStats(forms) {
  return sendToWorker('calculateFormStats', { forms })
}

/**
 * 排序和过滤表单
 * @param {Array} forms - 表单数据
 * @param {Object} options - 排序和过滤选项
 * @returns {Promise} - 处理后的表单
 */
export function sortAndFilterForms(forms, options) {
  return sendToWorker('sortAndFilterForms', { forms, options })
}

/**
 * 批量处理表单
 * @param {Array} forms - 表单数据
 * @param {Object} operation - 操作配置
 * @returns {Promise} - 处理后的表单
 */
export function batchProcessForms(forms, operation) {
  return sendToWorker('batchProcessForms', { forms, operation })
}

/**
 * 终止Worker
 */
export function terminateWorker() {
  if (worker) {
    worker.terminate()
    worker = null
    isInitialized = false

    // 拒绝所有待处理请求
    pendingRequests.forEach(({ reject }) => {
      reject(new Error('Worker terminated'))
    })
    pendingRequests.clear()
  }
}

/**
 * 重新初始化Worker
 */
export function restartWorker() {
  terminateWorker()
  return initWorker()
}

// 导出工具对象
export default {
  calculateFormStats,
  sortAndFilterForms,
  batchProcessForms,
  terminateWorker,
  restartWorker
}
