import { CardGroups } from './styleConfig';

export type CardGroups = Record<string, any>;

// 带文件名的文件创建接口
export interface NewFileRequest {
  filename: string;
}

// newFile 接口响应数据
export interface NewFileResponse {
  result: string;
  filename: string;
  filestatus: string;
}

// filestatus 接口请求数据
export interface FileStatusRequest {
  filename: string;
}

// filestatus 接口响应数据
export interface FileStatusResponse {
  filename: string;
  status: string;
  lastModified: number;
}

// deletefile 接口请求数据
export interface DeleteFileRequest {
  filename: string;
}

// deletefile 接口响应数据
export interface DeleteFileResponse {
  result: string;
  filename: string;
  filestatus: string;
}

// addWorksheet 接口请求数据
export interface AddWorksheetRequest {
  filename: string;
  sheetname: string;
}

// addWorksheet 接口响应数据
export interface AddWorksheetResponse {
  result: string;
  filename: string;
  sheetname: string;
  filestatus: string;
}

// removeWorksheet 接口请求数据
export interface RemoveWorksheetRequest {
  filename: string;
  sheetname: string;
}

// removeWorksheet 接口响应数据
export interface RemoveWorksheetResponse {
  result: string;
  filename: string;
  sheetname: string;
  filestatus: string;
}

// renameFile 接口请求数据
export interface RenameFileRequest {
  oldFilename: string;
  newFilename: string;
}

// renameFile 接口响应数据
export interface RenameFileResponse {
  result: string;
  oldFilename: string;
  newFilename: string;
  filestatus: string;
}

// renameWorksheet 接口请求数据
export interface RenameWorksheetRequest {
  filename: string;
  sheetname: string;
  newsheetname: string;
}

// renameWorksheet 接口响应数据
export interface RenameWorksheetResponse {
  result: string;
  filename: string;
  sheetname: string;
  newsheetname: string;
  filestatus: string;
}

// sheetdata 接口请求数据
export interface SheetDataRequest {
  filename: string;
  sheetname: string;
}

// sheetdata 接口响应数据
export interface SheetDataResponse {
  [key: string]: string;
}[]

// 更新数据接口
export interface UpdateData {
  sheetname: string;
  updatecells: Record<string, string>;
}

// 更新文件数据接口
export interface UpdateFileRequest {
  filename: string;
  updatedata: UpdateData[];
}

// updateFile 接口响应数据
export interface UpdateFileResponse {
  result: string; // "success"
  filename: string;
  filestatus: string; // "processing"
}

// 工作表数据接口
export interface WorksheetData {
  id: string;
  name: string;
  cards: CardGroups;
  [key: string]: any;
}

// API 响应接口
export interface ApiResponse<T = any> {
  code: number;
  message: string;
  data: T;
  [key: string]: any;
}

// 文件列表项接口
export interface FileListItem {
  id: string;
  name: string;
  lastModified: string; // ISO 日期格式
  size: number;
}

// 样式配置接口
export interface StyleConfig {
  id: string;
  name: string;
  config: Record<string, any>;
  [key: string]: any;
}

// 批量操作接口
export interface BatchOperation {
  operation: string;
  data: any;
  [key: string]: any;
}