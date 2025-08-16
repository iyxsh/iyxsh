import axios, { AxiosInstance, InternalAxiosRequestConfig, CancelToken } from 'axios';
import { NewFileRequest, UpdateFileRequest, FileListItem, StyleConfig, BatchOperation, NewFileResponse, UpdateFileResponse, FileStatusRequest, FileStatusResponse, DeleteFileRequest, DeleteFileResponse, AddWorksheetRequest, AddWorksheetResponse, RemoveWorksheetRequest, RemoveWorksheetResponse, RenameFileRequest, RenameFileResponse, RenameWorksheetRequest, RenameWorksheetResponse, SheetDataRequest, SheetDataResponse } from './apiTypes';

// 导入新增的类型
import { SheetListRequest, SheetListResponse } from './apiTypes';

// 重新导入 CardGroups 类型
import { CardGroups } from './apiTypes';

// 创建高级axios实例，支持更多功能
const advancedApiClient: AxiosInstance = axios.create({
  baseURL: import.meta.env.VITE_APP_API_BASE_URL || 'https://192.168.0.3:8443/api',
  timeout: 30000, // 增加超时时间以支持大数据量
  headers: {
    'Content-Type': 'application/json'
  }
});

// 请求拦截器
advancedApiClient.interceptors.request.use(
  (config: InternalAxiosRequestConfig): InternalAxiosRequestConfig => {
    // 添加时间戳以避免缓存，但不在POST请求中添加以避免触发CORS预检请求
    if (config.method !== 'get' && config.method !== 'post' && config.params) {
      config.params = {
        ...config.params,
        _t: Date.now()
      };
    }
    
    // 记录请求信息用于调试
    console.log('API Request:', {
      method: config.method?.toUpperCase(),
      url: config.url,
      baseURL: config.baseURL,
      fullURL: `${config.baseURL}${config.url}`,
      data: config.data
    });
    
    // 可以在这里添加认证token等
    return config;
  },
  (error: any) => {
    console.error('Request Interceptor Error:', error);
    return Promise.reject(error);
  }
);

// 响应拦截器
advancedApiClient.interceptors.response.use(
  (response: any) => {
    return response.data;
  },
  (error: any) => {
    console.error('Advanced API Error:', error);
    
    // 添加更详细的错误信息
    if (error.response) {
      console.error('Error Response:', {
        status: error.response.status,
        statusText: error.response.statusText,
        data: error.response.data,
        url: error.config?.url,
        method: error.config?.method,
        baseURL: error.config?.baseURL
      });
    }
    
    return Promise.reject(error);
  }
);

// 定义API方法的参数接口
interface RenameWorksheetParams {
  worksheetId: string;
  newName: string;
}

interface RenameFileParams {
  fileId: string;
  newName: string;
}

interface RemoveWorksheetParams {
  worksheetId: string;
}

interface DeleteFileParams {
  filename: string;
}

interface GetFileDataParams {
  fileId: string;
}

interface CreateFileWithNameParams {
  filename: string;
  name?: string;
  [key: string]: any;
}

// 高级API接口，支持取消请求和批量操作
export const advancedCardApi = {
  // 获取卡片组数据
  getCardGroups(cancelToken: CancelToken | null = null) {
    return advancedApiClient.get<CardGroups>('/card-groups', { cancelToken: cancelToken || undefined });
  },
  
  // 保存卡片组数据
  saveCardGroups(cardGroups: CardGroups, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<CardGroups>('/card-groups', cardGroups, { cancelToken: cancelToken || undefined });
  },
  
  // 批量保存卡片组数据
  saveCardGroupsBatch(batches: CardGroups[], cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<CardGroups[]>('/card-groups/batch', batches, { cancelToken: cancelToken || undefined });
  },
  
  // 更新卡片组数据
  updateCardGroups(cardGroups: CardGroups, cancelToken: CancelToken | null = null) {
    return advancedApiClient.put<CardGroups>('/card-groups', cardGroups, { cancelToken: cancelToken || undefined });
  },
  
  // 创建新文件
  newFile(newfilerequest: NewFileRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<NewFileResponse>('/newfile', newfilerequest, { cancelToken: cancelToken || undefined });
  },
  
  // 获取文件状态
  fileStatus(fileStatusRequest: FileStatusRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<FileStatusResponse>('/filestatus', fileStatusRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 更新文件
  updateFile(fileData: UpdateFileRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<UpdateFileResponse>('/updatefile', fileData, { cancelToken: cancelToken || undefined });
  },
  
  // 获取文件数据
  getFileData(fileId: string, cancelToken: CancelToken | null = null) {
    const params: GetFileDataParams = { fileId };
    return advancedApiClient.post<UpdateFileResponse>('/filedata', params, { cancelToken: cancelToken || undefined });
  },
  
  // 添加工作表
  addWorksheet(addWorksheetRequest: AddWorksheetRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<AddWorksheetResponse>('/addworksheet', addWorksheetRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 删除工作表
  removeWorksheet(removeWorksheetRequest: RemoveWorksheetRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<RemoveWorksheetResponse>('/removeworksheet', removeWorksheetRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 重命名工作表
  renameWorksheet(renameWorksheetRequest: RenameWorksheetRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<RenameWorksheetResponse>('/renameworksheet', renameWorksheetRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 重命名文件
  renameFile(renameFileRequest: RenameFileRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<RenameFileResponse>('/renamefile', renameFileRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 获取文件列表
  getFileList(cancelToken: CancelToken | null = null) {
    return advancedApiClient.get<FileListItem[]>('/filelist', { cancelToken: cancelToken || undefined });
  },
  
  // 获取样式配置
  getStyleConfig(cancelToken: CancelToken | null = null) {
    return advancedApiClient.get<StyleConfig>('/style-config', { cancelToken: cancelToken || undefined });
  },
  
  // 保存样式配置
  saveStyleConfig(styleConfig: Partial<StyleConfig>, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<StyleConfig>('/style-config', styleConfig, { cancelToken: cancelToken || undefined });
  },
  
  // 批量操作
  batchOperation(operations: BatchOperation[], cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<any>('/batch', operations, { cancelToken: cancelToken || undefined });
  },
  
  // 数据同步
  syncData(cancelToken: CancelToken | null = null) {
    return advancedApiClient.get<any>('/sync', { cancelToken: cancelToken || undefined });
  },
  
  // 获取工作表数据
  getSheetData(sheetDataRequest: SheetDataRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<SheetDataResponse>('/sheetdata', sheetDataRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 获取工作表列表
  sheetlist(sheetListRequest: SheetListRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<SheetListResponse>('/sheetlist', sheetListRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 删除文件
  deleteFile(deleteFileRequest: DeleteFileRequest, cancelToken: CancelToken | null = null) {
    return advancedApiClient.post<DeleteFileResponse>('/deletefile', deleteFileRequest, { cancelToken: cancelToken || undefined });
  },
  
  // 健康检查
  healthCheck(cancelToken: CancelToken | null = null) {
    return advancedApiClient.get('/health', { cancelToken: cancelToken || undefined });
  },
  
  // 测试连接
  testConnection(cancelToken: CancelToken | null = null) {
    return advancedApiClient.get('/', { cancelToken: cancelToken || undefined });
  }
};

export default advancedCardApi;