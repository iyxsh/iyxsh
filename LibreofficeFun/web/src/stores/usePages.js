import { ref, watch } from 'vue'

// 获取当前文件名（作为修文名）
const getCurrentFileName = () => {
  // 从URL参数中获取文件名
  const urlParams = new URLSearchParams(window.location.search);
  return urlParams.get('fileName') || '';
};

// 获取存储键名（支持文件名区分）
const getStorageKey = () => {
  const fileName = getCurrentFileName();
  return fileName ? `form-pages-${fileName}` : 'form-pages';
};

export function usePages() {
  const pages = ref([])
  
  // 加载缓存（支持文件名区分）
  const loadCache = () => {
    const storageKey = getStorageKey();
    const cache = localStorage.getItem(storageKey);
    if (cache) {
      try {
        const parsedCache = JSON.parse(cache);
        if (Array.isArray(parsedCache)) {
          // 确保每个页面都有完整的数据结构
          pages.value = parsedCache.map(page => ({
            id: page.id || Date.now() + Math.random(),
            name: page.name || '新页面',
            forms: Array.isArray(page.forms) ? page.forms.map(form => ({
              ...form,
              id: form.id || Date.now() + Math.random()
            })) : [],
            pageSize: page.pageSize || {
              name: 'A4',
              width: 210,
              height: 297,
              unit: 'mm'
            },
            orientation: page.orientation || 'portrait'
          }));
        }
        console.log(`[usePages] 从本地存储加载页面数据（${storageKey}），页面数量:`, pages.value.length);
      } catch (error) {
        console.error('[usePages] 解析缓存数据失败:', error);
        pages.value = [];
      }
    } else {
      console.log(`[usePages] 未找到缓存数据（${storageKey}）`);
    }
  };
  
  // 初始加载缓存
  loadCache();
  
  // 监听页面数据变化并保存到本地存储（支持文件名区分）
  watch(pages, v => {
    try {
      const storageKey = getStorageKey();
      localStorage.setItem(storageKey, JSON.stringify(v));
      console.log(`[usePages] 页面数据已保存到本地存储（${storageKey}），页面数量:`, v.length);
    } catch (error) {
      console.error('[usePages] 保存数据到本地存储失败:', error);
    }
  }, { deep: true });
  
  // 重新加载缓存（当文件名改变时调用）
  const reloadCache = () => {
    loadCache();
  };
  
  function addPage(pageData) {
    // 如果传入的是完整页面对象
    if (typeof pageData === 'object' && pageData !== null && !Array.isArray(pageData)) {
      pages.value.push({
        id: pageData.id || Date.now() + Math.random(),
        name: pageData.name || '新页面',
        forms: Array.isArray(pageData.forms) ? pageData.forms.map(form => ({
          ...form,
          id: form.id || Date.now() + Math.random()
        })) : [],
        pageSize: pageData.pageSize || {
          name: 'A4',
          width: 210,
          height: 297,
          unit: 'mm'
        },
        orientation: pageData.orientation || 'portrait'
      })
    } else {
      // 向后兼容：如果传入的是页面名称
      const name = typeof pageData === 'string' ? pageData : '新页面'
      pages.value.push({
        id: Date.now() + Math.random(),
        name,
        forms: [],
        pageSize: {
          name: 'A4',
          width: 210,
          height: 297,
          unit: 'mm'
        },
        orientation: 'portrait'
      })
    }
    console.log('[usePages] 添加新页面，当前页面数量:', pages.value.length);
  }
  
  function removePage(idx) {
    if (idx >= 0 && idx < pages.value.length) {
      pages.value.splice(idx, 1)
      console.log('[usePages] 删除页面，当前页面数量:', pages.value.length);
    }
  }
  
  function updatePage(idx, data) {
    if (pages.value[idx]) {
      pages.value[idx] = { ...pages.value[idx], ...data }
      console.log('[usePages] 更新页面，页面索引:', idx);
    }
  }
  
  // 添加旋转页面功能
  function rotatePageOrientation(idx) {
    const page = pages.value[idx]
    if (!page) return

    // 切换方向
    const newOrientation = page.orientation === 'portrait' ? 'landscape' : 'portrait'

    // 如果需要，交换宽高
    const pageSize = page.pageSize
    if (pageSize) {
      const { width, height } = pageSize
      updatePage(idx, {
        orientation: newOrientation,
        pageSize: {
          ...pageSize,
          width: height,
          height: width
        }
      })
    } else {
      updatePage(idx, { orientation: newOrientation })
    }
    console.log('[usePages] 旋转页面方向，页面索引:', idx, '新方向:', newOrientation);
  }

  console.log('[usePages] 初始化完成，页面数量:', pages.value.length);
  return { pages, addPage, removePage, updatePage, rotatePageOrientation, reloadCache }
}