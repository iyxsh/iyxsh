// errorLogService.js
import { ElMessage } from 'element-plus';

// 存储错误日志
const errorLogs = [];

// 最大错误日志数量
const MAX_ERROR_LOGS = 100;

/**
 * 添加错误日志
 * @param {Error} error 错误对象
 * @param {string} contextInfo 上下文信息
 * @param {string} level 错误级别 ('error', 'warning', 'info')
 */
function addErrorLog(error, contextInfo = 'Unknown Context', level = 'error') {
  // 验证参数
  if (!error || !(error instanceof Error)) {
    console.warn('[errorLogService] 无效的错误对象');
    return;
  }

  // 检查是否是500错误
  let isServerError = false;
  if (contextInfo.includes('500') || 
      (error.message && (error.message.includes('500') || error.message.includes('Internal Server Error'))) ||
      (error.status && error.status === 500)) {
    isServerError = true;
    level = 'error'; // 强制设为error级别
    contextInfo = `服务器错误 (500): ${contextInfo}`;
  }

  if (errorLogs.length >= MAX_ERROR_LOGS) {
    errorLogs.shift(); // 移除最早的日志
  }

  const logEntry = {
    timestamp: new Date(),
    level,
    context: contextInfo,
    message: error.message,
    stack: error.stack,
    // 限制堆栈跟踪长度以节省内存
    shortStack: error.stack ? error.stack.split('\n').slice(0, 5).join('\n') : '',
    isServerError, // 添加是否是服务器错误的标记
    url: window.location.href // 添加当前URL信息
  };

  errorLogs.push(logEntry);

  // 根据错误级别输出日志
  switch (level) {
    case 'error':
      console.error(`[全局错误] ${contextInfo}: ${error.message}`, error);
      // 显示用户友好的错误提示
      ElMessage.error({
        message: isServerError 
          ? `服务器错误：服务暂时不可用，请稍后重试。` 
          : `发生错误：${error.message.substring(0, 50)}...`,
        duration: 5000,
        showClose: true
      });
      break;
    case 'warning':
      console.warn(`[全局警告] ${contextInfo}: ${error.message}`);
      // 显示警告提示
      ElMessage.warning({
        message: `注意：${error.message.substring(0, 50)}...`,
        duration: 3000
      });
      break;
    case 'info':
      console.info(`[全局信息] ${contextInfo}: ${error.message}`);
      break;
    default:
      console.log(`[全局日志] ${contextInfo}: ${error.message}`);
  }
}

/**
 * 获取错误日志
 * @returns {Array} 错误日志数组
 */
function getErrorLogs() {
  return [...errorLogs]; // 返回副本
}

/**
 * 清除错误日志
 */
function clearErrorLogs() {
  errorLogs.length = 0;
  console.log('[errorLogService] 错误日志已清除');
}

/**
 * 处理未捕获的Promise错误
 * @param {Event} event 未捕获的Promise错误事件
 */
function handleUnhandledRejection(event) {
  console.warn('[errorLogService] 捕获到未处理的Promise拒绝:', event);
  
  // 特别处理消息端口关闭的错误
  if (event.reason && typeof event.reason === 'object') {
    if (event.reason.message && 
        event.reason.message.includes('The message port closed before a response was received')) {
      console.warn('[errorLogService] 检测到消息端口关闭错误，这可能是一个Chrome扩展相关的问题，不会影响应用功能');
      // 阻止该特定错误的默认处理
      event.preventDefault();
      return;
    }
    
    // 处理其他特定的命令错误
    if (event.reason.cmd) {
      console.warn('[errorLogService] 检测到命令执行错误:', event.reason.cmd);
      // 根据命令类型决定是否阻止默认处理
      event.preventDefault();
      return;
    }
  }
  
  // 防止默认处理（会将错误暴露给用户）
  event.preventDefault();
  
  // 记录错误
  addErrorLog(event.reason, '未处理的Promise拒绝', 'error');
}

// 监听未捕获的Promise错误
if (typeof window !== 'undefined' && window.addEventListener) {
  window.addEventListener('unhandledrejection', handleUnhandledRejection);
}

/**
 * 清理函数 - 移除事件监听器
 */
function cleanup() {
  if (typeof window !== 'undefined' && window.removeEventListener) {
    window.removeEventListener('unhandledrejection', handleUnhandledRejection);
  }
}

// 导出服务
export default {
  addErrorLog,
  getErrorLogs,
  clearErrorLogs,
  handleUnhandledRejection,
  cleanup
};