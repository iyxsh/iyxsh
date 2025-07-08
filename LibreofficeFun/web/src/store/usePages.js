import { ref, watch } from 'vue'
const STORAGE_KEY = 'form-pages'
export function usePages() {
  const pages = ref([])
  // 加载缓存
  const cache = localStorage.getItem(STORAGE_KEY)
  if (cache) pages.value = JSON.parse(cache)
  watch(pages, v => localStorage.setItem(STORAGE_KEY, JSON.stringify(v)), { deep: true })
  function addPage(name = '新页面') {
    pages.value.push({ id: Date.now(), name, forms: [] })
  }
  function removePage(idx) {
    pages.value.splice(idx, 1)
  }
  function updatePage(idx, data) {
    pages.value[idx] = { ...pages.value[idx], ...data }
  }
  return { pages, addPage, removePage, updatePage }
}
