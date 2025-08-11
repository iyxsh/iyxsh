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
      'pageManager.newPage': '新页面',
      'pageManager.confirmDelete': '确认删除',
      'pageManager.confirmDeletePage': '确定要删除此页面吗？',
      'pageManager.warning': '警告',
      'pageManager.confirm': '确认',
      'pageManager.pageCreationFailed': '页面创建失败',
      'pageManager.selectPageSize': '选择纸张尺寸',
      'pageManager.pageName': '页面名称',
      'pageManager.enterPageName': '输入页面名称',
      'pageManager.pageSize': '页面尺寸',
      'pageManager.formPage': '表单页',
      'pageManager.cardPage': '卡片页',
      'pageManager.confirmCloseDuringCreation': '页面正在创建中，确定要关闭吗？',
      'pageManager.noPages': '请先创建页面',
      'pageManager.unlockPageFirst': '请先解锁页面再添加表单',
      'pageManager.cannotAddForm': '无法添加表单：表单编辑器未就绪',
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
      'pageManager.pageCreated': '页面创建成功',
      
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
      'pageManager.newPage': 'New Page',
      'pageManager.confirmDelete': 'Confirm Deletion',
      'pageManager.confirmDeletePage': 'Are you sure you want to delete this page?',
      'pageManager.warning': 'Warning',
      'pageManager.confirm': 'Confirm',
      'pageManager.pageCreationFailed': 'Page creation failed',
      'pageManager.selectPageSize': 'Select Paper Size',
      'pageManager.pageName': 'Page Name',
      'pageManager.enterPageName': 'Enter page name',
      'pageManager.pageSize': 'Page Size',
      'pageManager.formPage': 'Form Page',
      'pageManager.cardPage': 'Card Page',
      'pageManager.confirmCloseDuringCreation': 'Page is being created, are you sure you want to close?',
      'pageManager.noPages': 'Please create a page first',
      'pageManager.unlockPageFirst': 'Please unlock the page before adding forms',
      'pageManager.cannotAddForm': 'Cannot add form: Form editor is not ready',
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
      'pageManager.pageCreated': 'Page created successfully',
      
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
    //console.log('currentLocale.value', currentLocale.value);
    //console.log('key', key);
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
      //console.log(`[i18n] 从缓存获取翻译: ${cacheKey} = ${cachedValue}`);
      
      // 如果缓存值为空或未定义，重新生成翻译
      if (cachedValue === undefined || cachedValue === null || cachedValue === '') {
        //console.warn(`[i18n] 缓存中存在但值无效: ${cacheKey}`);
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

    // 处理参数替换
    const finalValue = typeof value === 'string' ? 
      value.replace(/\{([^}]+)\}/g, (match, param) => 
        params[param] !== undefined ? params[param] : match
      ) : 
      value || defaultValue;
    
    // 只有当finalValue有效时才缓存
    if (finalValue !== undefined && finalValue !== null && finalValue !== '') {
      translationCache.set(cacheKey, finalValue);
    }
    
    return finalValue;
  }


  return {
    locale: computed(() => currentLocale.value),
    setLocale,
    t,
    supportedLocales
  };
}

// 导出一个默认的i18n实例
const i18n = createI18n();

// 优化的翻译函数
export const t = i18n.t;
export const setLocale = i18n.setLocale;
export const getLocale = () => i18n.locale.value;
export const locale = i18n.locale;