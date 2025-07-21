import { ref, watch } from 'vue'
const STORAGE_KEY = 'form-pages'
export function usePages() {
  const pages = ref([])
  // 加载缓存
  const cache = localStorage.getItem(STORAGE_KEY)
  if (cache) pages.value = JSON.parse(cache)
  watch(pages, v => localStorage.setItem(STORAGE_KEY, JSON.stringify(v)), { deep: true })
  function addPage(name = '新页面', pageSize = null) {
    pages.value.push({
      id: Date.now(),
      name,
      forms: [],
      pageSize: pageSize || {
        name: 'A4',
        width: 210,
        height: 297,
        unit: 'mm'
      },
      orientation: 'portrait'  // 确保包含初始方向值
    })
  }
  function removePage(idx) {
    pages.value.splice(idx, 1)
  }
  function updatePage(idx, data) {
    pages.value[idx] = { ...pages.value[idx], ...data }
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
  }

  return { pages, addPage, removePage, updatePage, rotatePageOrientation }
}