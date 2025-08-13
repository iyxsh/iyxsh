<template>
  <div class="api-service-manager">
    <!-- 这是一个无渲染组件，只提供API服务管理功能 -->
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue';
import { advancedCardApi } from '@/utils/api.ts';

// 定义服务状态
const serviceState = reactive({
  isLoading: false,
  isSaving: false,
  error: null,
  lastSync: null
});

// 任务队列管理
const taskQueue = ref([]);
const activeTasks = ref(0);
const maxConcurrentTasks = 5; // 最大并发任务数

// 添加任务到队列
const enqueueTask = (task) => {
  taskQueue.value.push(task);
  processQueue();
};

// 处理任务队列
const processQueue = async () => {
  if (activeTasks.value >= maxConcurrentTasks || taskQueue.value.length === 0) {
    return;
  }

  const task = taskQueue.value.shift();
  activeTasks.value++;

  try {
    await task();
  } catch (error) {
    console.error('Task failed:', error);
  } finally {
    activeTasks.value--;
    processQueue(); // 处理下一个任务
  }
};

// 创建新文件
const newFile = async (/** @type {NewFileRequest} */ fileData) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    // 类型验证
    if (fileData && typeof fileData === 'object') {
      await advancedCardApi.newFile(fileData);
    } else {
      throw new Error('Invalid fileData parameter');
    }
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 更新文件
const updateFile = async (/** @type {UpdateFileRequest} */ fileData) => {
  serviceState.isSaving = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (fileData && typeof fileData === 'object') {
      await advancedCardApi.updateFile(fileData);
    } else {
      throw new Error('Invalid fileData parameter');
    }
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 获取文件数据
const getFileData = async (/** @type {string} */ fileId) => {
  serviceState.isLoading = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (typeof fileId !== 'string' || !fileId) {
      throw new Error('Invalid fileId parameter');
    }
    
    const response = await advancedCardApi.getFileData(fileId);
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 添加工作表
const addWorksheet = async (/** @type {AddWorksheetRequest} */ worksheetData) => {
  serviceState.isSaving = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!worksheetData || typeof worksheetData !== 'object') {
      throw new Error('Invalid worksheetData parameter');
    }
    
    // 验证必需字段
    if (!worksheetData.filename || !worksheetData.sheetname) {
      throw new Error('Missing required fields: filename and sheetname are required');
    }
    
    await advancedCardApi.addWorksheet(worksheetData);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 删除工作表
const removeWorksheet = async (/** @type {RemoveWorksheetRequest} */ removeWorksheetRequest) => {
  serviceState.isSaving = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!removeWorksheetRequest || typeof removeWorksheetRequest !== 'object') {
      throw new Error('Invalid removeWorksheetRequest parameter');
    }
    
    // 验证必需字段
    if (!removeWorksheetRequest.filename || !removeWorksheetRequest.sheetname) {
      throw new Error('Missing required fields: filename and sheetname are required');
    }
    
    await advancedCardApi.removeWorksheet(removeWorksheetRequest);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 重命名工作表
const renameWorksheet = async (/** @type {RenameWorksheetRequest} */ renameWorksheetRequest) => {
  serviceState.isSaving = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!renameWorksheetRequest || typeof renameWorksheetRequest !== 'object') {
      throw new Error('Invalid renameWorksheetRequest parameter');
    }
    
    // 验证必需字段
    if (!renameWorksheetRequest.filename || 
        !renameWorksheetRequest.sheetname || 
        !renameWorksheetRequest.newsheetname) {
      throw new Error('Missing required fields: filename, sheetname and newsheetname are required');
    }
    
    await advancedCardApi.renameWorksheet(renameWorksheetRequest);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 重命名文件
const renameFile = async (/** @type {import('@/utils/apiTypes').RenameFileRequest} */ renameFileRequest) => {
  serviceState.isSaving = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!renameFileRequest || typeof renameFileRequest !== 'object') {
      throw new Error('Invalid renameFileRequest parameter');
    }
    
    // 验证必需字段
    if (!renameFileRequest.oldFilename || !renameFileRequest.newFilename) {
      throw new Error('Missing required fields: oldFilename and newFilename are required');
    }
    
    await advancedCardApi.renameFile(renameFileRequest);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 删除文件
const deleteFile = async (/** @type {DeleteFileRequest} */ deleteFileRequest) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    // 类型验证
    if (!deleteFileRequest || typeof deleteFileRequest !== 'object') {
      throw new Error('Invalid deleteFileRequest parameter');
    }
    
    // 验证必需字段
    if (!deleteFileRequest.filename) {
      throw new Error('Missing required field: filename is required');
    }
    
    await advancedCardApi.deleteFile(deleteFileRequest);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 获取文件状态
const fileStatus = async (/** @type {FileStatusRequest} */ fileStatusRequest) => {
  serviceState.isLoading = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!fileStatusRequest || typeof fileStatusRequest !== 'object') {
      throw new Error('Invalid fileStatusRequest parameter');
    }
    
    // 验证必需字段
    if (!fileStatusRequest.filename) {
      throw new Error('Missing required field: filename is required');
    }
    
    const response = await advancedCardApi.fileStatus(fileStatusRequest);
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 获取工作表数据（支持大数据量读取策略）
const getSheetData = async (/** @type {import('@/utils/apiTypes').SheetDataRequest} */ sheetDataRequest) => {
  serviceState.isLoading = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!sheetDataRequest || typeof sheetDataRequest !== 'object') {
      throw new Error('Invalid sheetDataRequest parameter');
    }
    
    // 验证必需字段
    if (!sheetDataRequest.filename || !sheetDataRequest.sheetname) {
      throw new Error('Missing required fields: filename and sheetname are required');
    }
    
    // 设置默认的大数据量读取策略参数
    const optimizedRequest = {
      ...sheetDataRequest,
      pageSize: sheetDataRequest.pageSize || 1000, // 默认每页1000条数据
      pageIndex: sheetDataRequest.pageIndex || 0, // 默认从第一页开始
      batchSize: sheetDataRequest.batchSize || 50, // 默认批处理大小50
      enableStreaming: sheetDataRequest.enableStreaming !== false, // 默认启用流式读取
      enableCompression: sheetDataRequest.enableCompression !== false // 默认启用压缩
    };
    
    console.log('[ApiServiceManager] 获取工作表数据，优化参数:', optimizedRequest);
    
    const response = await advancedCardApi.getSheetData(optimizedRequest);
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 大数据量分页读取工作表数据
const getSheetDataWithPagination = async (/** @type {import('@/utils/apiTypes').SheetDataRequest} */ sheetDataRequest, /** @type {Function} */ onProgress = null) => {
  serviceState.isLoading = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!sheetDataRequest || typeof sheetDataRequest !== 'object') {
      throw new Error('Invalid sheetDataRequest parameter');
    }
    
    // 验证必需字段
    if (!sheetDataRequest.filename || !sheetDataRequest.sheetname) {
      throw new Error('Missing required fields: filename and sheetname are required');
    }
    
    const pageSize = sheetDataRequest.pageSize || 1000;
    const batchSize = sheetDataRequest.batchSize || 50;
    let allData = [];
    let currentPage = 0;
    let hasMoreData = true;
    
    console.log('[ApiServiceManager] 开始分页读取工作表数据');
    
    // 分页读取数据
    while (hasMoreData) {
      const pageRequest = {
        ...sheetDataRequest,
        pageIndex: currentPage,
        pageSize: pageSize,
        batchSize: batchSize,
        enableStreaming: true,
        enableCompression: true
      };
      
      console.log(`[ApiServiceManager] 读取第 ${currentPage + 1} 页数据`);
      
      const response = await advancedCardApi.getSheetData(pageRequest);
      
      if (!response || !response.data) {
        console.warn(`[ApiServiceManager] 第 ${currentPage + 1} 页数据为空`);
        hasMoreData = false;
        break;
      }
      
      const pageData = Array.isArray(response.data) ? response.data : [response.data];
      allData = allData.concat(pageData);
      
      // 检查是否还有更多数据
      hasMoreData = pageData.length === pageSize;
      currentPage++;
      
      // 报告进度
      if (onProgress && typeof onProgress === 'function') {
        onProgress({
          currentPage: currentPage,
          totalPages: hasMoreData ? 'unknown' : currentPage,
          loadedRecords: allData.length,
          isComplete: !hasMoreData
        });
      }
      
      // 防止无限循环
      if (currentPage > 100) {
        console.warn('[ApiServiceManager] 达到最大页数限制，停止读取');
        break;
      }
    }
    
    serviceState.lastSync = new Date();
    console.log(`[ApiServiceManager] 分页读取完成，总共读取 ${allData.length} 条记录`);
    
    return {
      data: allData,
      totalPages: currentPage,
      totalRecords: allData.length
    };
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 获取卡片组数据
const getCardGroups = async () => {
  serviceState.isLoading = true;
  serviceState.error = null;
  
  try {
    const response = await advancedCardApi.getCardGroups();
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 保存卡片组数据（支持批量处理）
const saveCardGroups = async (/** @type {any[]} */ cardGroups) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    // 类型验证
    if (!Array.isArray(cardGroups)) {
      throw new Error('Invalid cardGroups parameter, must be an array');
    }
    
    // 对于大数据量，分批处理
    if (cardGroups.length > 100) {
      const batchSize = 50;
      const batches = [];
      
      for (let i = 0; i < cardGroups.length; i += batchSize) {
        batches.push(cardGroups.slice(i, i + batchSize));
      }
      
      // 创建批量保存任务
      const batchTasks = batches.map((batch, index) => {
        return () => advancedCardApi.saveCardGroupsBatch({
          batch: batch,
          batchIndex: index,
          totalBatches: batches.length
        });
      });
      
      // 并行执行任务
      await Promise.all(batchTasks.map(task => enqueueTask(task)));
    } else {
      await advancedCardApi.saveCardGroups(cardGroups);
    }
    
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 获取样式配置
const getStyleConfig = async () => {
  serviceState.isLoading = true;
  serviceState.error = null;
  
  try {
    const response = await advancedCardApi.getStyleConfig();
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 保存样式配置
const saveStyleConfig = async (/** @type {Partial<import('@/utils/apiTypes').StyleConfig>} */ styleConfig) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    await advancedCardApi.saveStyleConfig(styleConfig);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 批量操作支持
const batchOperation = async (/** @type {import('@/utils/apiTypes').BatchOperation[]} */ operations) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    // 类型验证
    if (!Array.isArray(operations)) {
      throw new Error('Invalid operations parameter, must be an array');
    }
    
    const results = [];
    
    // 分组操作以提高效率
    const operationGroups = {};
    operations.forEach(op => {
      if (!operationGroups[op.type]) {
        operationGroups[op.type] = [];
      }
      operationGroups[op.type].push(op);
    });
    
    // 并行处理不同类型的操作
    const groupPromises = Object.keys(operationGroups).map(async (type) => {
      const group = operationGroups[type];
      if (group.length > 20) {
        // 大量操作分批处理
        const batchSize = 20;
        for (let i = 0; i < group.length; i += batchSize) {
          const batch = group.slice(i, i + batchSize);
          await advancedCardApi.batchOperation([{ type, data: batch }]);
        }
      } else {
        // 小批量直接处理
        await advancedCardApi.batchOperation([{ type, data: group }]);
      }
    });
    
    await Promise.all(groupPromises);
    serviceState.lastSync = new Date();
    
    return results;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 获取文件列表
const getFileList = async () => {
  serviceState.isLoading = true;
  serviceState.error = null;
  
  try {
    const response = await advancedCardApi.getFileList();
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 获取工作表列表
const getSheetList = async (/** @type {import('@/utils/apiTypes').SheetListRequest} */ sheetListRequest) => {
  serviceState.isLoading = true;
  serviceState.error = null;

  try {
    // 类型验证
    if (!sheetListRequest || typeof sheetListRequest !== 'object') {
      throw new Error('Invalid sheetListRequest parameter');
    }

    // 验证必需字段
    if (!sheetListRequest.filename) {
      throw new Error('Missing required field: filename is required');
    }

    const response = await advancedCardApi.sheetlist(sheetListRequest);
    serviceState.lastSync = new Date();
    return response;
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 数据同步方法
const syncData = async () => {
  serviceState.isLoading = true;
  try {
    await Promise.all([
      getCardGroups(),
      getStyleConfig()
    ]);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isLoading = false;
  }
};

// 暴露方法和状态
defineExpose({
  // 数据方法
  getCardGroups,
  saveCardGroups,
  getStyleConfig,
  saveStyleConfig,
  batchOperation,
  syncData,
  getFileList,
  newFile, // 添加newFile方法
  updateFile, // 添加updateFile方法
  getFileData, // 添加getFileData方法
  addWorksheet, // 添加addWorksheet方法
  removeWorksheet, // 添加removeWorksheet方法
  renameWorksheet, // 添加renameWorksheet方法
  renameFile, // 添加renameFile方法
  deleteFile, // 添加deleteFile方法
  fileStatus, // 添加fileStatus方法
  getSheetData, // 添加getSheetData方法
  getSheetDataWithPagination, // 添加大数据量分页读取方法
  getSheetList, // 添加 getSheetList 方法
  serviceState,
  
  // 任务管理
  enqueueTask,
  processQueue
});
</script>

<style scoped>
.api-service-manager {
  display: none;
}
</style>