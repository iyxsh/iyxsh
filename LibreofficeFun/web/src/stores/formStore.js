import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { generateId } from '../utils/helpers'
import { usePageStore } from './pageStore'
import { validateFormCard, sanitizeFormCard } from '../services/validationService'

/**
 * 表单管理状态存储
 */
export const useFormStore = defineStore('forms', () => {
  // 依赖pageStore
  const pageStore = usePageStore()

  // 状态
  const formPositions = ref({}) // { [formId]: { x, y, z } }
  const formSizes = ref({}) // { [formId]: { width, height } }
  const cardStyleEnabled = ref(true)
  const editingFormId = ref(null)

  // 选择状态
  const selectedFormIds = ref([])

  // 获取当前页面的表单
  const currentPageForms = computed(() => {
    return pageStore.currentPage?.forms || []
  })

  // 是否有表单被选中
  const hasSelectedForms = computed(() => selectedFormIds.value.length > 0)

  // 当前编辑的表单
  const currentEditingForm = computed(() => {
    if (!editingFormId.value) return null
    return currentPageForms.value.find(f => f.id === editingFormId.value) || null
  })

  // 初始化，从本地存储加载位置信息
  const initialize = () => {
    try {
      // 加载表单位置
      const storedPositions = localStorage.getItem('form-positions')
      if (storedPositions) {
        formPositions.value = JSON.parse(storedPositions)
      }

      // 加载表单尺寸
      const storedSizes = localStorage.getItem('form-sizes')
      if (storedSizes) {
        formSizes.value = JSON.parse(storedSizes)
      }

      // 加载卡片样式设置
      const styleSetting = localStorage.getItem('card-style-enabled')
      if (styleSetting !== null) {
        cardStyleEnabled.value = styleSetting === 'true'
      }
    } catch (error) {
      console.error('从本地存储加载表单配置失败:', error)
    }
  }

  // 保存位置和尺寸到本地存储
  const savePositionsAndSizes = () => {
    try {
      localStorage.setItem('form-positions', JSON.stringify(formPositions.value))
      localStorage.setItem('form-sizes', JSON.stringify(formSizes.value))
    } catch (error) {
      console.error('保存表单位置和尺寸失败:', error)
    }
  }

  // 创建新表单
  const createForm = (formData = {}) => {
    if (!pageStore.currentPage) return null

    const defaultFormData = {
      title: '',
      value: '',
      remark: '',
      media: '',
      showTitle: true,
      showValue: true,
      showRemark: true,
      showMedia: false,
      titleFontSize: 16,
      valueFontSize: 16,
      remarkFontSize: 14,
      titleColor: '#333',
      valueColor: '#333',
      remarkColor: '#666'
    }

    const newForm = {
      ...defaultFormData,
      ...formData,
      id: generateId()
    }

    // 添加到当前页面
    const addedForm = pageStore.addFormToCurrentPage(newForm)

    // 设置默认位置和尺寸
    if (addedForm) {
      // 找一个合适的位置放置新表单
      const offset = Object.keys(formPositions.value).length * 20
      formPositions.value[addedForm.id] = { x: 50 + offset, y: 50 + offset, z: Object.keys(formPositions.value).length }
      formSizes.value[addedForm.id] = { width: 240, height: 160 }
      savePositionsAndSizes()
    }

    return addedForm
  }

  // 更新表单
  const updateForm = (formId, updates) => {
    // 净化数据
    const sanitizedUpdates = sanitizeFormCard(updates)

    // 验证表单
    const { isValid, errors } = validateFormCard(sanitizedUpdates)
    if (!isValid) {
      return { success: false, errors }
    }

    // 更新表单
    const success = pageStore.updateFormInCurrentPage(formId, sanitizedUpdates)
    return { success, errors: [] }
  }

  // 删除表单
  const deleteForm = (formId) => {
    const success = pageStore.removeFormFromCurrentPage(formId)

    // 如果删除成功，也删除对应的位置和尺寸信息
    if (success) {
      delete formPositions.value[formId]
      delete formSizes.value[formId]
      savePositionsAndSizes()

      // 如果删除的是正在编辑的表单，重置编辑状态
      if (editingFormId.value === formId) {
        editingFormId.value = null
      }

      // 从选择列表中移除
      selectedFormIds.value = selectedFormIds.value.filter(id => id !== formId)
    }

    return success
  }

  // 设置表单位置
  const setFormPosition = (formId, position) => {
    if (!formId || !position) return false

    formPositions.value[formId] = {
      ...formPositions.value[formId],
      ...position
    }

    savePositionsAndSizes()
    return true
  }

  // 设置表单尺寸
  const setFormSize = (formId, size) => {
    if (!formId || !size) return false

    formSizes.value[formId] = {
      ...formSizes.value[formId],
      ...size
    }

    savePositionsAndSizes()
    return true
  }

  // 切换卡片样式
  const toggleCardStyle = () => {
    cardStyleEnabled.value = !cardStyleEnabled.value
    localStorage.setItem('card-style-enabled', cardStyleEnabled.value.toString())
    return cardStyleEnabled.value
  }

  // 设置编辑表单
  const setEditingForm = (formId) => {
    editingFormId.value = formId
  }

  // 选择表单
  const selectForm = (formId, multiSelect = false) => {
    if (!multiSelect) {
      // 单选模式，清除之前的选择
      selectedFormIds.value = [formId]
    } else {
      // 多选模式，切换选择状态
      const index = selectedFormIds.value.indexOf(formId)
      if (index === -1) {
        selectedFormIds.value.push(formId)
      } else {
        selectedFormIds.value.splice(index, 1)
      }
    }
  }

  // 清除选择
  const clearSelection = () => {
    selectedFormIds.value = []
  }

  // 获取表单的位置
  const getFormPosition = (formId) => {
    return formPositions.value[formId] || { x: 0, y: 0, z: 0 }
  }

  // 获取表单的尺寸
  const getFormSize = (formId) => {
    return formSizes.value[formId] || { width: 240, height: 160 }
  }

  // 清理无效的表单数据（不存在于任何页面中的表单的位置和尺寸）
  const cleanupInvalidData = () => {
    // 收集所有表单ID
    const allFormIds = new Set()
    pageStore.pages.forEach(page => {
      page.forms.forEach(form => {
        allFormIds.add(form.id)
      })
    })

    // 清理不存在的表单数据
    Object.keys(formPositions.value).forEach(id => {
      if (!allFormIds.has(id)) {
        delete formPositions.value[id]
      }
    })

    Object.keys(formSizes.value).forEach(id => {
      if (!allFormIds.has(id)) {
        delete formSizes.value[id]
      }
    })

    savePositionsAndSizes()
  }

  // 初始化
  initialize()

  return {
    // 状态
    formPositions,
    formSizes,
    cardStyleEnabled,
    editingFormId,
    selectedFormIds,

    // 计算属性
    currentPageForms,
    hasSelectedForms,
    currentEditingForm,

    // 方法
    createForm,
    updateForm,
    deleteForm,
    setFormPosition,
    setFormSize,
    toggleCardStyle,
    setEditingForm,
    selectForm,
    clearSelection,
    getFormPosition,
    getFormSize,
    cleanupInvalidData,
    savePositionsAndSizes
  }
})
