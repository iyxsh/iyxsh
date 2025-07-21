import { ref, computed } from 'vue';

// 支持的语言列表
export const supportedLocales = {
  'zh-CN': '简体中文',
  'en-US': 'English'
};

// 获取浏览器默认语言
function getBrowserLocale() {
  const navigatorLocale = navigator.language || 'en-US';
  return Object.keys(supportedLocales).find(key => key === navigatorLocale) ? navigatorLocale : 'en-US';
}

// 从localStorage获取用户首选语言
function getStoredLocale() {
  const storedLocale = localStorage.getItem('user-locale');
  return storedLocale && Object.keys(supportedLocales).includes(storedLocale) ? storedLocale : null;
}

// 获取初始语言
export function getInitialLocale() {
  return getStoredLocale() || getBrowserLocale();
}

// 缓存已解析的翻译路径
const translationCache = new Map();

// 创建i18n实例
export function createI18n(locale = getInitialLocale()) {
  const currentLocale = ref(locale);
  
  // 加载语言文件
  const messages = {
    'zh-CN': {
      // 页面管理相关
      'pageManager.cancel': '取消',
      'pageManager.create': '创建',
      'pageManager.series': '{category}系列',
      'pageManager.dialogTitle': '页面大小设置',
      'pageManager.form.orientation': '方向',
      'pageManager.form.landscape': '横向',
      'pageManager.form.portrait': '纵向',
      'pageManager.form.size': '大小',
      'pageManager.form.unit': '单位',
      'pageManager.form.margins': '边距',
      'pageManager.form.top': '上',
      'pageManager.form.bottom': '下',
      'pageManager.form.left': '左',
      'pageManager.form.right': '右',
      'pageManager.form.confirm': '确认',
      'pageManager.form.reset': '重置',
      
      // 工具栏相关
      'toolbar.addPage': '添加页面',
      'toolbar.clearAll': '清空所有',
      'toolbar.rotatePage': '旋转页面',
      'toolbar.deletePage': '删除页面',
      
      // 表单相关
      'form.dragPlaceholder': '拖放表单到这里',
      'form.noForms': '没有表单'
    },
    'en-US': {
      // 页面管理相关
      'pageManager.cancel': 'Cancel',
      'pageManager.create': 'Create',
      'pageManager.series': '{category} Series',
      'pageManager.dialogTitle': 'Page Size Settings',
      'pageManager.form.orientation': 'Orientation',
      'pageManager.form.landscape': 'Landscape',
      'pageManager.form.portrait': 'Portrait',
      'pageManager.form.size': 'Size',
      'pageManager.form.unit': 'Unit',
      'pageManager.form.margins': 'Margins',
      'pageManager.form.top': 'Top',
      'pageManager.form.bottom': 'Bottom',
      'pageManager.form.left': 'Left',
      'pageManager.form.right': 'Right',
      'pageManager.form.confirm': 'Confirm',
      'pageManager.form.reset': 'Reset',
      
      // 工具栏相关
      'toolbar.addPage': 'Add Page',
      'toolbar.clearAll': 'Clear All',
      'toolbar.rotatePage': 'Rotate Page',
      'toolbar.deletePage': 'Delete Page',
      
      // 表单相关
      'form.dragPlaceholder': 'Drag form here',
      'form.noForms': 'No forms'
    }
  };

  // 设置语言
  function setLocale(newLocale) {
    if (messages[newLocale]) {
      currentLocale.value = newLocale;
      localStorage.setItem('user-locale', newLocale);
      translationCache.clear(); // 清除缓存
    }
  }

  // 翻译函数
  function t(key, params = {}, defaultValue = '') {
    // 检查缓存
    console.log('currentLocale.value', currentLocale.value);
    console.log('key', key);
    //console.log('params', params);
    //console.log('defaultValue', defaultValue);
    //console.log('messages[currentLocale.value]', messages[currentLocale.value]);
    //console.log('translationCache', translationCache);
    //console.log('translationCache.has(currentLocale.value)', translationCache.has(currentLocale.value));
    //console.log('translationCache.get(currentLocale.value)', translationCache.get(currentLocale.value));
    //console.log('translationCache.get(currentLocale.value)[key]', translationCache.get(currentLocale.value)[key]);
    const cacheKey = `${currentLocale.value}:${key}`;
    if (translationCache.has(cacheKey)) {
      const cachedValue = translationCache.get(cacheKey);
      console.log(`[i18n] 从缓存获取翻译: ${cacheKey} = ${cachedValue}`);
      
      // 如果缓存值为空或未定义，重新生成翻译
      if (cachedValue === undefined || cachedValue === null || cachedValue === '') {
        console.warn(`[i18n] 缓存中存在但值无效: ${cacheKey}`);
        // 删除无效缓存
        translationCache.delete(cacheKey);
      } else {
        return cachedValue;
      }
    }
    
    // 使用更高效的查找方式
    const currentMessages = messages[currentLocale.value];
    let currentValue;
    
    // 特殊处理：检测消息结构类型（扁平化或嵌套）
    const isFlatStructure = !currentMessages || typeof currentMessages !== 'object' || 
                           Object.values(currentMessages).some(value => typeof value !== 'object');
    
    if (isFlatStructure) {
      // 扁平化结构处理：优先尝试完整键匹配
      currentValue = currentMessages[key];
      
      // 如果未找到，尝试按点分隔符查找
      if (currentValue === undefined && key.includes('.')) {
        const dotIndex = key.indexOf('.');
        const firstPart = key.substring(0, dotIndex);
        const remainingPart = key.substring(dotIndex + 1);
        
        const firstValue = currentMessages[firstPart];
        if (firstValue !== undefined && typeof firstValue === 'object') {
          // 如果第一部分是对象，则按嵌套方式查找
          currentValue = remainingPart.split('.').reduce((acc, part) => {
            return acc && typeof acc === 'object' && part in acc ? acc[part] : undefined;
          }, firstValue);
        }
      }
    } else {
      // 嵌套结构处理：按点分隔符逐级查找
      const keyParts = key.split('.');
      currentValue = keyParts.reduce((acc, part) => {
        return acc && typeof acc === 'object' && part in acc ? acc[part] : undefined;
      }, currentMessages);
    }
    
    // 设置最终值，如果未找到则使用默认值
    const value = currentValue !== undefined ? currentValue : defaultValue;
    
    // 添加详细的错误日志
    if (!value && key.split('.').length > 1) {
      const parts = key.split('.');
      let current = messages[currentLocale.value];
      
      console.warn(`[i18n] 翻译查找失败: ${key}`);
      console.warn(`[i18n] 当前语言: ${currentLocale.value}`);
      console.warn(`[i18n] 翻译表结构:`, messages[currentLocale.value]);
      
      // 输出路径上的所有值，帮助调试
      for (let i = 0; i < parts.length - 1; i++) {
        if (current && typeof current === 'object' && parts[i] in current) {
          current = current[parts[i]];
        } else {
          console.warn(`[i18n] 查找中断路径: ${parts.slice(0, i+1).join('.')}`);
          console.warn(`[i18n] 当前值类型: ${typeof current}, 值内容:`, current);
          break;
        }
      }
    }
    console.log(`[i18n] 翻译查找: ${key} = ${value}`);
    // 处理参数替换
    const finalValue = typeof value === 'string' ? 
      value.replace(/\{([^}]+)\}/g, (match, param) => 
        params[param] !== undefined ? params[param] : match
      ) : 
      value || defaultValue;
    
    // 只有当finalValue有效时才缓存
    if (finalValue !== undefined && finalValue !== null && finalValue !== '') {
      translationCache.set(cacheKey, finalValue);
      console.log(`[i18n] 新缓存翻译: ${cacheKey} = ${finalValue}`);
    } else {
      console.warn(`[i18n] 翻译值无效，未缓存: ${cacheKey}`);
      console.warn(`[i18n] 当前语言包内容:`, messages[currentLocale.value]);
    }
    
    return finalValue;
  }

  // 添加内存泄漏检查
  function checkMemoryUsage() {
    if (performance && performance.memory) {
      const memoryUsage = performance.memory;
      console.log('i18n Memory Usage:', {
        usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
        totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
        jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
      });
      
      // 如果内存使用超过阈值，触发警告
      if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
        console.warn('i18n: Memory usage is high, consider optimizing');
      }
    }
  }

  // 格式化字节大小
  function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }

  // 添加资源清理
  const cleanupTasks = [];

  // 添加内存泄漏检查间隔
  let memoryCheckInterval = null;

  // 初始化内存泄漏检查
  function setupMemoryLeakCheck() {
    if (!memoryCheckInterval) {
      memoryCheckInterval = setInterval(() => {
        checkMemoryUsage();
      }, 5 * 60 * 1000); // 每5分钟检查一次
    }
  }

  // 停止内存泄漏检查
  function stopMemoryLeakCheck() {
    if (memoryCheckInterval) {
      clearInterval(memoryCheckInterval);
      memoryCheckInterval = null;
    }
  }

  // 添加清理任务
  cleanupTasks.push(stopMemoryLeakCheck);

  // 在组件卸载前执行清理
  function cleanupResources() {
    // 执行清理任务
    cleanupTasks.forEach(task => task());
    // 清除缓存
    translationCache.clear();
  }

  // 初始化内存泄漏检查
  setupMemoryLeakCheck();

  return {
    locale: computed(() => currentLocale.value),
    setLocale,
    t,
    supportedLocales,
    cleanupResources
  };
}

// 导出一个默认的i18n实例
const i18n = createI18n();

// 优化的翻译函数
export const t = i18n.t;
export const setLocale = i18n.setLocale;
export const getLocale = () => i18n.locale.value;
export const locale = i18n.locale;