<script setup>
// =====================================================
// = 1. 导入依赖 
// =====================================================
import { ref, reactive, computed, onMounted, onBeforeUnmount, watch } from 'vue'
import FormCard from './FormCard.vue'
import FormEditor from './FormEditor.vue'
import { calculateCardStyle, getDefaultCardStyles } from '@/services/cardStyleService'
import { handleDragStart, handleDragEnd, handleDragOver, handleDrop } from '@/services/dragDropService'
import { validateForm, validateAllForms } from '@/services/formValidationService'
import { ElMessage, ElMessageBox } from 'element-plus'

// 新增错误日志服务
import errorLogService from '@/services/errorLogService'

// =====================================================
// = 2. 定义 props 和 emits
// =====================================================
// 定义 emits
const emit = defineEmits(['update:modelValue'])

// 定义 props
const props = defineProps({
  editable: {
    type: Boolean,
    default: true
  },
  modelValue: {
    type: Array,
    default: () => []
  }
})

// =====================================================
// = 3. 服务初始化 
// =====================================================

// 使用表单验证服务
// 添加对新创建的 formStyleService 的导入
import { getFormStyle, getCardStyle, applyFormStyles } from '@/services/formStyleService'

// 错误处理相关
const error = ref(null)
const errorInfo = ref('')

// 错误处理方法
const handleError = (err, info) => {
  error.value = err
  errorInfo.value = info
  ElMessage.error(`发生错误：${info}`)
  console.error(`[FormGrid Error] ${info}:`, err)
  
  // 使用错误日志服务
  errorLogService.addErrorLog({
    error: err,
    info: info,
    level: 'error'
  })
  
  return false // 返回 false 表示错误已处理
}

// =====================================================
// = 6. 缺失的响应式变量定义 
// =====================================================

// 添加拖拽服务作为响应式变量
// 初始化样式
const formStyles = ref({})

// =====================================================
// = 5. 计算属性
// =====================================================
// 计算表单样式
const computedStyles = computed(() => {
  return (form) => {
    return {
      ...getFormStyle(form),
      ...getCardStyle(positions.value, form)
    }
  }
})

// 计算可编辑状态
const isEditMode = computed(() => {
  return () => {
    return editable.value
  }
})


// 响应式数据
const forms = ref([])
const visibleForms = computed(() => {
  return forms.value.filter(form => !form.hidden)
})

// 更新表单样式
const updateFormStyle = () => {
  if (applyFormStyles) {
    const newStyle = applyFormStyles(forms.value, positions.value)
    // 应用新的样式
    // 这里可以添加额外的样式应用逻辑
  }
}

// 监听 props.modelValue 的变化
watch(
  () => props.modelValue,
  (newVal) => {
    if (Array.isArray(newVal)) {
      forms.value = newVal
    } else {
      handleError(new Error('Model value is not an array'), '数据绑定错误')
    }
  },
  { immediate: true, deep: true }
)

// =====================================================
// = 6. 缺失的响应式变量定义 
// =====================================================
// 定义缺失的响应式变量
const positions = ref({})
const showDetail = ref({})
const menuIndex = ref(-1)
const showAddDialog = ref(false)
const addFormData = ref(null)
const isDragging = ref(false)
const draggedForm = ref(null)
const offsetX = ref(0)
const offsetY = ref(0)
const isResizing = ref(false)
const resizingIndex = ref(-1)
const startX = ref(0)
const startY = ref(0)
const initialWidth = ref(0)
const initialHeight = ref(0)
const contentRefs = ref({})

// 定义缺失的方法
const startDrag = (event, element, index) => {
  if (!editable.value) return
  
  event.preventDefault()
  isDragging.value = true
  draggedForm.value = element
  offsetX.value = event.clientX - element.x
  offsetY.value = event.clientY - element.y
  
  // 添加事件监听器
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', stopDrag)
}

const handleDrag = (event) => {
  if (!isDragging.value || !draggedForm.value) return
  
  // 计算新的位置
  const newX = event.clientX - offsetX.value
  const newY = event.clientY - offsetY.value
  
  // 更新拖拽元素的位置
  if (contentRefs.value[draggedForm.value.id]) {
    contentRefs.value[draggedForm.value.id].style.transform = `translate(${newX}px, ${newY}px)`
  }
}

const stopDrag = () => {
  if (isDragging.value) {
    isDragging.value = false
    
    // 移除事件监听器
    document.removeEventListener('mousemove', handleDrag)
    document.removeEventListener('mouseup', stopDrag)
    
    // 保存最终位置
    if (draggedForm.value && contentRefs.value[draggedForm.value.id]) {
      const rect = contentRefs.value[draggedForm.value.id].getBoundingClientRect()
      updateFormPosition(draggedForm.value.id, rect.left, rect.top)
    }
    
    draggedForm.value = null
  }
}

// 添加缺失的依赖方法
const updateFormPosition = (id, x, y) => {
  positions.value = {
    ...positions.value,
    [id]: { x, y }
  }
}

const resetFormPositions = () => {
  positions.value = {}
}

// =====================================================
// = 7. DOM 引用方法 
// =====================================================
// 添加缺失的 ref
const setContentRef = (id) => {
  return (el) => {
    if (el) {
      contentRefs.value[id] = el
    }
  }
}

// =====================================================
// = 8. 事件处理方法 
// =====================================================
// 添加缺失的事件处理方法
const handleClickOutside = (event) => {
  const menuElement = document.querySelector('.card-menu')
  if (menuElement && !menuElement.contains(event.target)) {
    closeMenu()
  }
}

const closeMenu = () => {
  menuIndex.value = -1
}

// 添加缺失的 resize 方法
const startResize = (event, index) => {
  try {
    if (!editable.value) return
    
    // 验证参数
    if (typeof index !== 'number' || index < 0 || index >= forms.value.length) {
      throw new Error('无效的表单索引')
    }
    
    event.preventDefault()
    isResizing.value = true
    resizingIndex.value = index
    
    // 记录初始位置
    startX.value = event.clientX
    startY.value = event.clientY
    
    // 记录初始尺寸
    const currentForm = forms.value[index]
    initialWidth.value = currentForm.width || 100
    initialHeight.value = currentForm.height || 50
    
    // 如果 dragDropService 有 startResize 方法，使用它
    if (dragDropService && dragDropService.startResize) {
      dragDropService.startResize(event, index)
    } else {
      // 使用错误日志服务记录错误
      errorLogService.addErrorLog({
        error: new Error('startResize方法不存在'),
        info: '拖拽服务缺少startResize方法',
        level: 'error',
      })
    }
    
    // 添加事件监听器
    document.addEventListener('mousemove', handleResize)
    document.addEventListener('mouseup', stopResize)
  } catch (error) {
    handleError(error, '开始调整尺寸时发生错误')
  }
}


const handleResize = (event) => {
  if (!isResizing.value) return
  
  // 如果 dragDropService 有 handleResize 方法，使用它
  if (dragDropService.handleResize) {
    dragDropService.handleResize(event)
  } else {
    // 否则使用本地实现
    const deltaX = event.clientX - startX.value
    const deltaY = event.clientY - startY.value
    
    // 更新表单尺寸
    forms.value[resizingIndex.value] = {
      ...forms.value[resizingIndex.value],
      width: initialWidth.value + deltaX,
      height: initialHeight.value + deltaY
    }
  }
}

const stopResize = () => {
  if (isResizing.value) {
    if (dragDropService.stopResize) {
      dragDropService.stopResize()
    } else {
      // 使用错误日志服务记录错误
      errorLogService.addErrorLog({
        error: new Error('stopResize方法不存在'),
        info: '拖拽服务缺少stopResize方法',
        component: 'FormGrid'
      })
    }
    
    isResizing.value = false
    
    // 移除事件监听器
    document.removeEventListener('mousemove', handleResize)
    document.removeEventListener('mouseup', stopResize)
    
    // 通知父组件表单已更新
    emit('update:modelValue', forms.value)
  }
}

// 添加缺失的菜单相关方法
const openMenu = (index, event) => {
  // 阻止默认右键菜单
  event.preventDefault()
  
  // 设置当前菜单索引
  menuIndex.value = index
  
  // 获取菜单元素
  const menuElement = document.querySelector('.card-menu')
  
  // 设置菜单位置
  if (menuElement) {
    menuElement.style.left = event.clientX + 'px'
    menuElement.style.top = event.clientY + 'px'
  }
}

// 添加缺失的右键菜单方法
const handleContextMenu = (event, index) => {
  event.preventDefault()
  if (!editable.value) return
  
  // 设置当前菜单索引
  menuIndex.value = index
  
  // 获取菜单元素
  const menuElement = document.querySelector('.card-menu')
  
  // 设置菜单位置
  if (menuElement) {
    menuElement.style.left = event.clientX + 'px'
    menuElement.style.top = event.clientY + 'px'
  }
}

// 添加缺失的表单操作方法
const editForm = (index) => {
  // 打开编辑对话框
  showDetail.value = {
    ...showDetail.value,
    [index]: true
  }
}

const closeDetail = (index) => {
  // 关闭对话框
  showDetail.value = {
    ...showDetail.value,
    [index]: false
  }
}

const onSaveDetail = (index, formData) => {
  // 更新表单数据
  forms.value[index] = formData
  
  // 通知父组件表单已更新
  emit('update:modelValue', forms.value)
  
  // 显示成功提示
  ElMessage.success('保存成功')
}

const handleDelete = (index) => {
  // 显示确认对话框
  ElMessageBox.confirm('确定要删除这个表单吗？', '警告', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    type: 'error'
  }).then(() => {
    // 删除表单
    forms.value = forms.value.filter((_, i) => i !== index)
    
    // 通知父组件表单已更新
    emit('update:modelValue', forms.value)
    
    // 显示成功提示
    ElMessage.success('删除成功')
  }).catch(() => {
    // 取消删除
  })
}

// 添加缺失的菜单点击方法
const openDetail = (index) => {
  showDetail.value = {
    ...showDetail.value,
    [index]: true
  }
}

const addForm = () => {
  // 创建新的表单对象
  const newForm = {
    id: Date.now(),
    title: '新表单',
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
    remarkColor: '#666',
    x: 50,
    y: 50,
    width: 100,
    height: 50,
    zIndex: 1
  }
  
  // 显示添加表单的对话框
  addFormData.value = newForm
  showAddDialog.value = true
}

const saveAddForm = (formData) => {
  // 添加新的表单
  forms.value = [
    ...forms.value,
    formData
  ]
  
  // 通知父组件表单已更新
  emit('update:modelValue', forms.value)
  
  // 关闭对话框
  showAddDialog.value = false
  
  // 显示成功提示
  ElMessage.success('表单添加成功')
}

const cancelAddForm = () => {
  // 关闭对话框
  showAddDialog.value = false
}

// 表单编辑器相关
const showFormEditor = ref(false)
const selectedForm = ref(null)

// 显示表单编辑器
const showFormEditorHandler = (form) => {
  selectedForm.value = form
  showFormEditor.value = true
}

// 隐藏表单编辑器
const hideFormEditor = () => {
  showFormEditor.value = false
  selectedForm.value = null
}

// 处理表单保存
const handleFormSave = (formData) => {
  // 保存表单逻辑
  console.log('保存表单:', formData)
  hideFormEditor()
}

// 添加缺失的表单更新方法
const handleUpdateForm = (updatedForm) => {
  try {
    // 输入验证
    if (!updatedForm || !updatedForm.id) {
      throw new Error('无效的表单数据')
    }
    
    // 更新表单数据
    forms.value = forms.value.map(form => 
      form.id === updatedForm.id ? updatedForm : form
    );
    
    // 通知父组件表单已更新
    emit('update:modelValue', forms.value);
    
    // 触发样式更新
    updateFormStyle()
  } catch (error) {
    handleError(error, '更新表单时发生错误')
  }
};

// 添加缺失的表单删除方法
const handleDeleteForm = (formId) => {
  try {
    // 输入验证
    if (!formId) {
      throw new Error('无效的表单ID')
    }
    
    // 显示确认对话框
    ElMessageBox.confirm('确定要删除这个表单吗？', '警告', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'error'
    }).then(() => {
      // 删除表单
      forms.value = forms.value.filter(form => form.id !== formId);
      
      // 通知父组件表单已更新
      emit('update:modelValue', forms.value);
      
      // 显示成功提示
      ElMessage.success('删除成功');
      
      // 触发样式更新
      updateFormStyle()
    }).catch((error) => {
      // 用户取消操作或发生错误
      if (error && error.type !== 'cancel') {
        handleError(error, '删除表单时发生错误')
      }
    });
  } catch (error) {
    handleError(error, '删除表单时发生错误')
  }
};

// 添加 handleAddForm 方法
const handleAddForm = (event) => {
  try {
    // 验证输入事件
    if (event && event.preventDefault) {
      event.preventDefault();
    }
    
    // 创建新的表单对象
    const newForm = {
      id: Date.now().toString() + Math.random().toString(36).slice(2),
      title: '',
      value: '',
      remark: '',
      media: '',
      showTitle: true,
      showValue: true,
      showRemark: false,
      showMedia: false,
      titleFontSize: 16,
      valueFontSize: 16,
      remarkFontSize: 14,
      titleColor: '#333',
      valueColor: '#333',
      remarkColor: '#666'
    };
    
    // 验证表单数据
    if (!newForm.id) {
      throw new Error('表单ID生成失败')
    }
    
    // 设置添加表单的数据并显示对话框
    addFormData.value = newForm;
    showAddDialog.value = true;
    
    return false;
  } catch (error) {
    handleError(error, '添加表单时发生错误')
    return false
  }
};

// 暴露方法给父组件
defineExpose({
  handleAddForm
});

// =====================================================
// = 8. 生命周期钩子 
// =====================================================
onMounted(() => {
  // 初始化位置信息
  forms.value.forEach(form => {
    if (form.x !== undefined && form.y !== undefined) {
      updateFormPosition(form.id, form.x, form.y)
    }
  })
  
  // 初始化表单样式
  updateFormStyle()
  
  // 初始化拖拽服务
  if (dragDropService.initialize) {
    dragDropService.initialize(
      containerRef.value,
      contentRefs.value,
      positions.value,
      showDetail.value,
      showAddDialog.value,
      addFormData.value,
      menuIndex.value,
      cardStyleOn.value,
      getDefaultCardStyles.value,
      calculateCardStyle.value,
      handleContextMenu,
      startDrag,
      startResize,
      cancelAddForm,
      saveAddForm,
      getCardAutoStyle,
      setContentRef
    )
  }

  // 添加点击事件监听器
  document.addEventListener('click', handleClickOutside);
})
</script>

<template>
  <div class="form-grid-container">
    <!-- 错误提示 -->
    <div v-if="error" class="error-boundary">
      <div class="error-boundary__title">发生错误</div>
      <div class="error-boundary__message">{{ error.message }}</div>
      <div class="error-boundary__info">{{ errorInfo }}</div>
    </div>
    
    <!-- 表单网格内容 -->
    <div class="form-grid" ref="containerRef">
      <!-- 表单卡片列表 -->
      <div class="form-cards">
        <div 
          v-for="(form, index) in forms" 
          :key="form.id"
          :ref="setContentRef(form.id)"
          :style="computedStyles(form)"
          class="form-card-wrapper"
        >
          <!-- 表单内容 -->
          <FormCard
            :form="form"
            :editable="editable"
            @update:form="handleUpdateForm"
            @delete:form="handleDeleteForm"
            @resize:start="startResize($event, index)"
          />
        </div>
      </div>
    </div>
    
    <!-- 添加表单对话框 -->
    <el-dialog v-model="showAddDialog" title="添加表单">
      <FormEditor
        :form="addFormData"
        @save="saveAddForm"
        @cancel="cancelAddForm"
      />
    </el-dialog>
    
    <!-- 表单编辑器 -->
    <div class="form-editor-container" v-if="showFormEditor">
      <FormEditor 
        :form-data="selectedForm" 
        @save="handleFormSave"
        @close="hideFormEditor"
      />
    </div>
  </div>
</template>

<style scoped>
/* =====================================================
   = 1. 基础样式 =
   ====================================================== */
.form-grid-container {
  position: relative;
  width: 100%;
  height: 100%;
}

.form-grid {
  position: relative;
  min-height: 100%;
  padding: 20px;
}

/* =====================================================
   = 6. 错误状态样式 =
   ====================================================== */
.error-boundary {
  background-color: #fef0f0;
  border-left: 4px solid #f56c6c;
  padding: 15px;
  margin: 10px 0;
  color: #f56c6c;
  font-size: 14px;
}

.error-boundary__title {
  font-weight: bold;
  margin-bottom: 8px;
}

.error-boundary__message {
  margin-bottom: 8px;
}

.error-boundary__info {
  font-size: 12px;
  opacity: 0.8;
}

/* =====================================================
   = 2. 工具栏样式 =
   ====================================================== */
.toolbar {
  margin-bottom: 15px;
  text-align: right;
}

/* =====================================================
   = 3. 表单卡片样式 =
   ====================================================== */
.form-card-wrapper {
  position: absolute;
  transition: transform 0.2s ease;
}

.form-card {
  position: relative;
  cursor: pointer;
  transition: all 0.3s ease;
}

.form-card:hover {
  transform: scale(1.02);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.resize-handle {
  position: absolute;
  right: 0;
  bottom: 0;
  width: 15px;
  height: 15px;
  background-color: #409EFF;
  cursor: se-resize;
  z-index: 10;
}

/* =====================================================
   = 4. 右键菜单样式 =
   ====================================================== */
.context-menu {
  position: absolute;
  background-color: #fff;
  border: 1px solid #e4e4e4;
  border-radius: 4px;
  box-shadow: 0 2px 12px 0 rgba(0,0,0,0.1);
  z-index: 999;
}

.menu-grid {
  display: flex;
  flex-direction: column;
  padding: 8px 0;
}

.menu-grid .el-button {
  width: 100%;
  text-align: left;
  padding: 8px 16px;
}

/* =====================================================
   = 5. 响应式样式 =
   ====================================================== */
@media (max-width: 768px) {
  .form-grid {
    padding: 10px;
  }
  
  .form-card {
    width: 100%;
    margin: 10px 0;
  }
}
</style>