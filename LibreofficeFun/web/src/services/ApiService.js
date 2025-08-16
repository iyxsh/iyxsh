// 创建 ApiService.js 文件

// 引入必要的依赖
import { advancedCardApi } from '@/utils/api';
import storage from '@/utils/storage';

// 定义服务状态
const serviceState = {
  isLoading: false,
  isSaving: false,
  error: null,
  lastSync: null
};

// 任务队列管理
const taskQueue = [];
let activeTasks = 0;
const maxConcurrentTasks = 5; // 最大并发任务数

// 添加任务到队列
const enqueueTask = (task) => {
  taskQueue.push(task);
  processQueue();
};

// 处理任务队列
const processQueue = async () => {
  if (activeTasks >= maxConcurrentTasks || taskQueue.length === 0) {
    return;
  }

  const task = taskQueue.shift();
  activeTasks++;

  try {
    await task();
  } catch (error) {
    console.error('Task failed:', error);
  } finally {
    activeTasks--;
    processQueue(); // 处理下一个任务
  }
};

// 在线/离线模式切换逻辑（异步，先判断 navigator.onLine，再实际 ping 后端）
const isOnline = async () => {
  if (!navigator.onLine) return false;
  try {
    // ping 一个轻量资源，建议用 /favicon.ico 或 /api/health
    const res = await fetch('/favicon.ico', { method: 'HEAD', cache: 'no-store' });
    return res.ok;
  } catch (e) {
    return false;
  }
};

// 健康检查功能
const checkBackendHealth = async () => {
  try {
    const response = await advancedCardApi.healthCheck();
    return response.status === 'ok';
  } catch (error) {
    return false;
  }
};

// 离线数据持久化
const saveOfflineData = (key, data) => {
  storage.set(key, data);
};

const getOfflineData = (key) => {
  return storage.get(key);
};

const clearOfflineData = (key) => {
  storage.remove(key);
};

// 同步离线数据到后端
const syncOfflineData = async () => {
  const offlineDataKeys = storage.getAllKeys();
  for (const key of offlineDataKeys) {
    const data = getOfflineData(key);
    try {
      await advancedCardApi.sync(data);
      clearOfflineData(key);
    } catch (error) {
      console.error(`Failed to sync offline data for key ${key}:`, error);
    }
  }
};

// IndexedDB 队列写入工具
function addToApiQueue(request) {
  return new Promise((resolve, reject) => {
    const openReq = window.indexedDB.open('libreofficefun-sync-db', 1);
    openReq.onupgradeneeded = event => {
      const db = event.target.result;
      if (!db.objectStoreNames.contains('apiQueue')) {
        db.createObjectStore('apiQueue', { keyPath: 'id', autoIncrement: true });
      }
    };
    openReq.onsuccess = () => {
      const db = openReq.result;
      const tx = db.transaction('apiQueue', 'readwrite');
      const store = tx.objectStore('apiQueue');
      store.add(request);
      tx.oncomplete = () => resolve();
      tx.onerror = () => reject(tx.error);
    };
    openReq.onerror = () => reject(openReq.error);
  });
}

function triggerSync() {
  if ('serviceWorker' in navigator && 'SyncManager' in window) {
    navigator.serviceWorker.ready.then(reg => {
      reg.sync.register('sync-api-queue');
    });
  }
}

// 通用的 API 调用方法
const callApi = async (apiFunction, ...args) => {
  if (!(await isOnline())) {
    // 离线时将请求参数写入 IndexedDB 队列
    await addToApiQueue({
      apiFunctionName: apiFunction.name,
      args
    });
    triggerSync();
    throw new Error('Network is unavailable. Request saved to offline queue.');
  }
  try {
    const response = await apiFunction(...args);
    return response;
  } catch (error) {
    console.error('API call failed:', error);
    throw error;
  }
};

// 集中管理所有 API 调用逻辑
const ApiService = {
  // 获取卡片组数据
  getCardGroups(cancelToken = null) {
    return advancedCardApi.getCardGroups(cancelToken);
  },

  // 保存卡片组数据
  saveCardGroups(cardGroups, cancelToken = null) {
    return advancedCardApi.saveCardGroups(cardGroups, cancelToken);
  },

  // 批量保存卡片组数据
  saveCardGroupsBatch(batches, cancelToken = null) {
    return advancedCardApi.saveCardGroupsBatch(batches, cancelToken);
  },

  // 更新卡片组数据
  updateCardGroups(cardGroups, cancelToken = null) {
    return advancedCardApi.updateCardGroups(cardGroups, cancelToken);
  },

  // 创建新文件
  newFile(newfilerequest, cancelToken = null) {
    return advancedCardApi.newFile(newfilerequest, cancelToken);
  },

  // 获取文件状态
  fileStatus(fileStatusRequest, cancelToken = null) {
    return advancedCardApi.fileStatus(fileStatusRequest, cancelToken);
  },

  // 更新文件
  updateFile(fileData, cancelToken = null) {
    return advancedCardApi.updateFile(fileData, cancelToken);
  },

  // 获取文件数据
  getFileData(fileId, cancelToken = null) {
    return advancedCardApi.getFileData(fileId, cancelToken);
  },

  // 添加工作表
  addWorksheet(addWorksheetRequest, cancelToken = null) {
    return advancedCardApi.addWorksheet(addWorksheetRequest, cancelToken);
  },

  // 删除工作表
  removeWorksheet(removeWorksheetRequest, cancelToken = null) {
    return advancedCardApi.removeWorksheet(removeWorksheetRequest, cancelToken);
  },

  // 重命名工作表
  renameWorksheet(renameWorksheetRequest, cancelToken = null) {
    return advancedCardApi.renameWorksheet(renameWorksheetRequest, cancelToken);
  },

  // 重命名文件
  renameFile(renameFileRequest, cancelToken = null) {
    return advancedCardApi.renameFile(renameFileRequest, cancelToken);
  },

  // 获取文件列表
  getFileList(cancelToken = null) {
    return advancedCardApi.getFileList(cancelToken);
  },

  // 获取样式配置
  getStyleConfig(cancelToken = null) {
    return advancedCardApi.getStyleConfig(cancelToken);
  },

  // 保存样式配置
  saveStyleConfig(styleConfig, cancelToken = null) {
    return advancedCardApi.saveStyleConfig(styleConfig, cancelToken);
  },

  // 批量操作
  batchOperation(operations, cancelToken = null) {
    return advancedCardApi.batchOperation(operations, cancelToken);
  },

  // 数据同步
  syncData(cancelToken = null) {
    return advancedCardApi.syncData(cancelToken);
  },

  // 获取工作表数据
  getSheetData(sheetDataRequest, cancelToken = null) {
    return advancedCardApi.getSheetData(sheetDataRequest, cancelToken);
  },

  // 获取工作表列表
  sheetlist(sheetListRequest, cancelToken = null) {
    return advancedCardApi.sheetlist(sheetListRequest, cancelToken);
  },

  // 删除文件
  deleteFile(deleteFileRequest, cancelToken = null) {
    return advancedCardApi.deleteFile(deleteFileRequest, cancelToken);
  },

  // 健康检查
  healthCheck(cancelToken = null) {
    return advancedCardApi.healthCheck(cancelToken);
  }
};


export { callApi };
export default ApiService;
