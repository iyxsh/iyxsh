<template>
  <div class="api-service-manager">
    <!-- 这是一个无渲染组件，只提供API服务管理功能 -->
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue';
import apiClient from '../utils/api.js';

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

// 获取卡片组数据
const getCardGroups = async () => {
  serviceState.isLoading = true;
  serviceState.error = null;
  
  try {
    const response = await apiClient.get('/card-groups');
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
const saveCardGroups = async (cardGroups) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    // 对于大数据量，分批处理
    if (cardGroups.length > 100) {
      const batchSize = 50;
      const batches = [];
      
      for (let i = 0; i < cardGroups.length; i += batchSize) {
        batches.push(cardGroups.slice(i, i + batchSize));
      }
      
      // 创建批量保存任务
      const batchTasks = batches.map((batch, index) => {
        return () => apiClient.post('/card-groups/batch', {
          batch: batch,
          batchIndex: index,
          totalBatches: batches.length
        });
      });
      
      // 并行执行任务
      await Promise.all(batchTasks.map(task => enqueueTask(task)));
    } else {
      await apiClient.post('/card-groups', cardGroups);
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
    const response = await apiClient.get('/style-config');
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
const saveStyleConfig = async (styleConfig) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
    await apiClient.post('/style-config', styleConfig);
    serviceState.lastSync = new Date();
  } catch (error) {
    serviceState.error = error.message;
    throw error;
  } finally {
    serviceState.isSaving = false;
  }
};

// 批量操作支持
const batchOperation = async (operations) => {
  serviceState.isSaving = true;
  serviceState.error = null;
  
  try {
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
          await apiClient.post(`/batch/${type}`, batch);
        }
      } else {
        // 小批量直接处理
        await apiClient.post(`/batch/${type}`, group);
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
  
  // 状态
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