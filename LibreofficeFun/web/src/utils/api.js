import axios from 'axios';

// 创建高级axios实例，支持更多功能
const advancedApiClient = axios.create({
  baseURL: process.env.VUE_APP_API_BASE_URL || 'https://192.168.0.3:8443/api',
  timeout: 30000, // 增加超时时间以支持大数据量
  headers: {
    'Content-Type': 'application/json'
  }
});

// 请求拦截器
advancedApiClient.interceptors.request.use(
  config => {
    // 添加时间戳以避免缓存，但不在POST请求中添加以避免触发CORS预检请求
    if (config.method !== 'get' && config.method !== 'post') {
      config.params = {
        ...config.params,
        _t: Date.now()
      };
    }
    
    // 可以在这里添加认证token等
    return config;
  },
  error => {
    return Promise.reject(error);
  }
);

// 响应拦截器
advancedApiClient.interceptors.response.use(
  response => {
    return response.data;
  },
  error => {
    console.error('Advanced API Error:', error);
    return Promise.reject(error);
  }
);

// 高级API接口，支持取消请求和批量操作
export const advancedCardApi = {
  // 获取卡片组数据
  getCardGroups(cancelToken = null) {
    return advancedApiClient.get('/card-groups', { cancelToken });
  },
  
  // 保存卡片组数据
  saveCardGroups(cardGroups, cancelToken = null) {
    return advancedApiClient.post('/card-groups', cardGroups, { cancelToken });
  },
  
  // 批量保存卡片组数据
  saveCardGroupsBatch(batches, cancelToken = null) {
    return advancedApiClient.post('/card-groups/batch', batches, { cancelToken });
  },
  
  // 更新卡片组数据
  updateCardGroups(cardGroups, cancelToken = null) {
    return advancedApiClient.put('/card-groups', cardGroups, { cancelToken });
  },
  
  // 创建新文件
  newFile(fileData, cancelToken = null) {
    return advancedApiClient.post('/newfile', fileData, { cancelToken });
  },
  
  // 更新文件
  updateFile(fileData, cancelToken = null) {
    return advancedApiClient.post('/updatefile', fileData, { cancelToken });
  },
  
  // 获取文件数据
  getFileData(fileId, cancelToken = null) {
    return advancedApiClient.post('/filedata', { fileId }, { cancelToken });
  },
  
  // 添加工作表
  addWorksheet(worksheetData, cancelToken = null) {
    return advancedApiClient.post('/addworksheet', worksheetData, { cancelToken });
  },
  
  // 删除工作表
  removeWorksheet(worksheetId, cancelToken = null) {
    return advancedApiClient.post('/removeworksheet', { worksheetId }, { cancelToken });
  },
  
  // 重命名工作表
  renameWorksheet(worksheetId, newName, cancelToken = null) {
    return advancedApiClient.post('/renameworksheet', { worksheetId, newName }, { cancelToken });
  },
  
  // 重命名文件
  renameFile(fileId, newName, cancelToken = null) {
    return advancedApiClient.post('/renamefile', { fileId, newName }, { cancelToken });
  },
  
  // 获取文件列表
  getFileList(cancelToken = null) {
    return advancedApiClient.get('/filelist', {}, { cancelToken });
  },
  
  // 获取样式配置
  getStyleConfig(cancelToken = null) {
    return advancedApiClient.get('/style-config', { cancelToken });
  },
  
  // 保存样式配置
  saveStyleConfig(styleConfig, cancelToken = null) {
    return advancedApiClient.post('/style-config', styleConfig, { cancelToken });
  },
  
  // 批量操作
  batchOperation(operations, cancelToken = null) {
    return advancedApiClient.post('/batch', operations, { cancelToken });
  },
  
  // 数据同步
  syncData(cancelToken = null) {
    return advancedApiClient.get('/sync', { cancelToken });
  },
  
  // 删除文件
  deleteFile(filename, cancelToken = null) {
    return advancedApiClient.post('/deletefile', { filename }, { cancelToken });
  }
};

export default advancedApiClient;