import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { generateId } from '../utils/helpers'

/**
 * 页面管理状态存储
 * 使用Pinia实现集中式状态管理，替代原始的usePages.js
 */
export const usePageStore = defineStore('pages', () => {
  // 状态
  const pages = ref([])
  const currentPageIndex = ref(0)
  const editPageIndex = ref(-1)

  // 获取计算属性
  const currentPage = computed(() => pages.value[currentPageIndex.value] || null)
  const isCurrentPageEditable = computed(() => editPageIndex.value === currentPageIndex.value)
  const pageCount = computed(() => pages.value.length)
  const hasPages = computed(() => pages.value.length > 0)

  // 初始化，尝试从本地存储加载
  const initialize = () => {
    try {
      const storedPages = localStorage.getItem('form-pages')
      if (storedPages) {
        pages.value = JSON.parse(storedPages)
        console.log('从本地存储加载了页面:', pages.value.length)
      }
    } catch (error) {
      console.error('从本地存储加载页面失败:', error)
    }
  }

  // 保存到本地存储
  const savePagesToStorage = () => {
    try {
      localStorage.setItem('form-pages', JSON.stringify(pages.value))
    } catch (error) {
      console.error('保存页面到本地存储失败:', error)
    }
  }

  // 添加页面
  const addPage = (name = '新页面', pageSizeObj = { name: 'A4', width: 210, height: 297, unit: 'mm' }) => {
    const newPage = {
      id: generateId(),
      name: name,
      forms: [],
      pageSize: pageSizeObj,
      orientation: 'portrait', // 默认为纵向
      createdAt: new Date().toISOString()
    }

    pages.value.push(newPage)
    currentPageIndex.value = pages.value.length - 1
    editPageIndex.value = currentPageIndex.value // 新页面默认可编辑

    savePagesToStorage()
    return newPage
  }

  // 更新页面
  const updatePage = (index, updates) => {
    if (index < 0 || index >= pages.value.length) return false

    pages.value[index] = {
      ...pages.value[index],
      ...updates,
      updatedAt: new Date().toISOString()
    }

    savePagesToStorage()
    return true
  }

  // 移除页面
  const removePage = (index) => {
    if (index < 0 || index >= pages.value.length) return false

    pages.value.splice(index, 1)

    // 调整当前页面索引
    if (currentPageIndex.value >= pages.value.length) {
      currentPageIndex.value = Math.max(0, pages.value.length - 1)
    }

    // 如果删除的是正在编辑的页面，重置编辑状态
    if (editPageIndex.value === index) {
      editPageIndex.value = -1
    } else if (editPageIndex.value > index) {
      // 如果删除的页面在编辑页面之前，调整编辑页面索引
      editPageIndex.value--
    }

    savePagesToStorage()
    return true
  }

  // 切换页面编辑状态
  const togglePageEdit = (index) => {
    if (index < 0 || index >= pages.value.length) return

    editPageIndex.value = editPageIndex.value === index ? -1 : index
  }

  // 设置当前页面
  const setCurrentPage = (index) => {
    if (index >= 0 && index < pages.value.length) {
      currentPageIndex.value = index
      return true
    }
    return false
  }

  // 旋转页面方向
  const rotatePageOrientation = (index) => {
    if (index < 0 || index >= pages.value.length) return false

    const page = pages.value[index]
    if (!page || !page.pageSize) return false

    // 交换宽高
    const { width, height } = page.pageSize
    page.pageSize = {
      ...page.pageSize,
      width: height,
      height: width
    }

    // 更新方向
    page.orientation = page.orientation === 'portrait' ? 'landscape' : 'portrait'

    savePagesToStorage()
    return true
  }

  // 添加表单到当前页面
  const addFormToCurrentPage = (form) => {
    if (!currentPage.value) return false

    const newForm = {
      id: generateId(),
      ...form,
      createdAt: new Date().toISOString()
    }

    currentPage.value.forms.push(newForm)
    savePagesToStorage()
    return newForm
  }

  // 更新当前页面中的表单
  const updateFormInCurrentPage = (formId, updates) => {
    if (!currentPage.value) return false

    const formIndex = currentPage.value.forms.findIndex(f => f.id === formId)
    if (formIndex === -1) return false

    currentPage.value.forms[formIndex] = {
      ...currentPage.value.forms[formIndex],
      ...updates,
      updatedAt: new Date().toISOString()
    }

    savePagesToStorage()
    return true
  }

  // 从当前页面删除表单
  const removeFormFromCurrentPage = (formId) => {
    if (!currentPage.value) return false

    const initialLength = currentPage.value.forms.length
    currentPage.value.forms = currentPage.value.forms.filter(f => f.id !== formId)

    if (currentPage.value.forms.length !== initialLength) {
      savePagesToStorage()
      return true
    }

    return false
  }

  // 清空当前页面表单
  const clearCurrentPageForms = () => {
    if (!currentPage.value) return false

    currentPage.value.forms = []
    savePagesToStorage()
    return true
  }

  // 清空所有页面
  const clearAllPages = () => {
    pages.value = []
    currentPageIndex.value = 0
    editPageIndex.value = -1
    localStorage.removeItem('form-pages')
    return true
  }

  // 初始化加载
  initialize()

  return {
    // 状态
    pages,
    currentPageIndex,
    editPageIndex,

    // 计算属性
    currentPage,
    isCurrentPageEditable,
    pageCount,
    hasPages,

    // 方法
    addPage,
    updatePage,
    removePage,
    togglePageEdit,
    setCurrentPage,
    rotatePageOrientation,
    addFormToCurrentPage,
    updateFormInCurrentPage,
    removeFormFromCurrentPage,
    clearCurrentPageForms,
    clearAllPages,
    savePagesToStorage
  }
})
