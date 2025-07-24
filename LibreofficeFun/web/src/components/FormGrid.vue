<script setup>
// =====================================================
// = 1. 导入依赖
// =====================================================
import { ref, reactive, computed, onMounted, onBeforeMount, onBeforeUnmount, watch, nextTick, toRaw, shallowRef } from 'vue'
import FormCard from './FormCard.vue'
import SingleFormShow from './SingleFormShow.vue'
import FormEditor from './FormEditor.vue'
// 导入完整的 cardStyleService
import * as cardStyleService from '@/services/cardStyleService'
import { calculateCardStyle, getDefaultCardStyles, getCardAutoStyle, initializeCardStyleService } from '@/services/cardStyleService'
// 导入 dragDropService
import * as dragDropService from '@/services/dragDropService'
import { validateForm, validateAllForms } from '@/services/formValidationService'
import { ElMessage, ElMessageBox } from 'element-plus'

// 新增错误日志服务
import errorLogService from '@/services/errorLogService'

// =====================================================
// = 2. 定义 props
// =====================================================
const props = defineProps({
  editable: {
    type: Boolean,
    required: true
  },
  forms: {
    type: Array,
    required: true
  },
  cardStyleOn: {
    type: Boolean,
    default: true
  },
  pageSize: {
    type: [Object, String],
    default: null
  }
})

// =====================================================
// = 3. 响应式变量定义
// =====================================================
// 定义 containerRef
const containerRef = ref(null)
// 定义 contentRefs
const contentRefs = ref({})
// 定义 positions
const positions = ref([])
// 定义 forms
const forms = ref([])

// 错误状态
const error = ref(null)
// 错误详情
const errorInfo = ref('')
// 加载状态
const loading = ref(true)
// 显示添加表单对话框
const showAddDialog = ref(false)
// 添加表单的数据
const addFormData = ref(null)
// 添加表单数据的key，用于强制刷新FormEditor组件
const addFormDataKey = ref(0)
// 是否显示表单编辑器
const showFormEditor = ref(false)
// 当前选中的表单
const selectedForm = ref(null)
// 是否显示添加表单的编辑器
const showAddFormEditor = ref(false)

// 表单验证函数
const validateForms = (forms) => {
  try {
    if (!Array.isArray(forms)) {
      console.warn('[FormGrid] 接收到非数组表单数据:', forms);
      return [];
    }
    return forms.map((form, index) => {
      // 确保表单有唯一ID，但如果已存在则不重新生成
      const formId = form.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`;
      
      // 确保表单有位置信息
      const position = form.position || { x: 20, y: 20 };
      
      // 确保表单有尺寸信息
      const size = form.size || { width: 200, height: 100 };
      
      // 确保表单有zIndex，如果没有则根据索引设置
      const zIndex = form.zIndex !== undefined ? form.zIndex : index + 1;
      
      return {
        ...form,
        id: formId,
        position,
        size,
        zIndex
      };
    });
  } catch (e) {
    handleError(e, '表单数据校验失败');
    return [];
  }
};

// 初始化网格布局
const initializeGridLayout = () => {
  console.log('[FormGrid] 初始化网格布局');
  // 这里可以添加实际的布局初始化逻辑
};

// =====================================================
// = 4. 错误处理和日志
// =====================================================
// 错误处理函数
const handleError = (error, context = '未知上下文') => {
  console.error(`[FormGrid Error] ${context}`, error);
  ElMessage.error({
    message: `操作失败: ${context}`,
    duration: 5000,
    type: 'error'
  });

  // 记录错误日志
  errorLogService.addErrorLog(error, context, 'error');
}

// 重置错误状态
const resetError = () => {
  error.value = null
  errorInfo.value = ''
}

// 处理组件错误
const handleContentError = (err) => {
  error.value = err
  errorInfo.value = '组件加载失败'
}

// 处理表单卡片错误
const handleFormCardError = (err) => {
  error.value = err
  errorInfo.value = '表单卡片加载失败'
}

// 处理表单编辑器错误
const handleFormEditorError = (err) => {
  error.value = err
  errorInfo.value = '表单编辑器加载失败'
}

// =====================================================
// = 5. 表单相关函数
// =====================================================
// 处理添加表单的方法
const handleAddForm = () => {
  try {
    console.log('[FormGrid] 开始处理添加表单');

    // 初始化添加表单数据
    addFormData.value = {
      id: `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`,
      title: '',
      value: '',
      remark: '',
      media: '',
      mediaType: 'image',
      showTitle: true,
      showValue: true,
      showRemark: true,
      showMedia: true,
      style: getDefaultCardStyles(),
      elementStyles: {
        title: {
          enabled: false,
          color: '#333333',
          fontSize: 16,
          fontWeight: 'bold'
        },
        value: {
          enabled: false,
          color: '#333333',
          fontSize: 16,
          fontWeight: 'normal'
        },
        remark: {
          enabled: false,
          color: '#666666',
          fontSize: 14,
          fontWeight: 'normal'
        },
        media: {
          enabled: false,
          color: '#333333',
          fontSize: 14,
          fontWeight: 'normal'
        }
      },
      position: { 
        x: 20 + (forms.value.length * 20), 
        y: 20 + (forms.value.length * 20) 
      },
      size: { width: 200, height: 100 },
      createdAt: new Date().toISOString()
    }
    
    // 更新key以强制刷新FormEditor组件
    addFormDataKey.value = Date.now()

    // 显示添加表单编辑器
    showAddFormEditor.value = true

    console.log('[FormGrid] 显示添加表单编辑器，表单ID:', addFormData.value.id)
  } catch (error) {
    handleError(error, '处理添加表单失败')
  }
}

const emit = defineEmits(['update:modelValue'])

// 保存添加的表单
const saveAddForm = (formData) => {
  try {
    console.log('[FormGrid] 开始保存表单', formData);
    
    // 确保 props.forms 是一个数组
    const currentForms = Array.isArray(props.forms) ? [...props.forms] : [];
    console.log('[FormGrid] 当前表单数量:', currentForms?.length ?? 0);
    
    // 生成更可靠的唯一ID
    const newForm = {
      ...formData,
      id: formData.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`,
      createdAt: formData.createdAt || new Date().toISOString()
    };
    
    // 创建新的表单数组
    const updatedForms = [...currentForms, newForm];
    console.log('[FormGrid] 更新后表单数量:', updatedForms.length);
    
    // 深度克隆表单数据以确保响应式更新
    const deepClonedForms = JSON.parse(JSON.stringify(updatedForms));
    
    // 更新内部表单状态，使用响应式方式更新
    forms.value = [...deepClonedForms];
    
    // 触发更新事件，确保父组件接收到更新
    emit('update:modelValue', [...deepClonedForms]);
    
    // 确保在DOM更新后执行
    nextTick(() => {
      console.log('[FormGrid] 表单保存后DOM已更新');
      
      // 隐藏添加表单编辑器
      showAddFormEditor.value = false;
      
      // 更新key以确保下次打开时强制刷新
      addFormDataKey.value = Date.now() + 1;
      
      // 提示成功
      ElMessage.success('表单添加成功');
      
      // 清空添加表单数据
      addFormData.value = null;
      
      // 强制更新卡片样式和位置
      if (cardStyleService.updateCardStyles) {
        cardStyleService.updateCardStyles([...deepClonedForms]);
      }
      
      // 更新卡片位置信息
      if (cardStyleService.setPositions) {
        const positions = deepClonedForms.map(form => ({
          id: form.id,
          ...form.position,
          width: form.size?.width || 200,
          height: form.size?.height || 100
        }));
        cardStyleService.setPositions(positions);
      }
    });
  } catch (error) {
    console.error('[FormGrid] 保存表单失败:', error);
    handleError(error, '保存表单失败');
  }
}

// 取消添加表单
const cancelAddForm = () => {
  showAddFormEditor.value = false
  // 更新key以确保下次打开时强制刷新
  addFormDataKey.value = Date.now();
  // 清空添加表单数据
  addFormData.value = null
}

// 计算样式函数
const computedStyles = (form) => {
  try {
    const defaultPosition = { x: 20, y: 20 };
    const defaultSize = { width: 200, height: 100 };

    // 获取表单位置和尺寸，如果不存在则使用默认值
    const position = form.position || defaultPosition;
    const size = form.size || defaultSize;

    // 返回计算后的样式
    return {
      position: 'absolute',
      left: `${position.x}px`,
      top: `${position.y}px`,
      width: `${size.width}px`,
      height: `${size.height}px`,
      zIndex: form.zIndex || 1,
    };
  } catch (error) {
    handleError(error, `计算表单样式失败: ${form.id}`);
    // 返回默认样式
    return {
      position: 'absolute',
      left: '20px',
      top: '20px',
      width: '200px',
      height: '100px'
    };
  }
};

// 设置ref的辅助函数
const setContentRef = (id) => (el) => {
  if (el) {
    contentRefs.value[id] = el;
  }
};

// 表单更新处理函数
const handleUpdateForm = (updatedForm) => {
  try {
    console.log('[FormGrid] 开始更新表单:', updatedForm?.id);
    
    // 确保updatedForm存在且有id
    if (!updatedForm || !updatedForm.id) {
      console.warn('[FormGrid] 更新的表单缺少ID:', updatedForm);
      ElMessage.warning('表单数据不完整');
      return;
    }
    
    // 查找表单索引
    const index = props.forms?.findIndex(f => f && f.id === updatedForm.id) ?? -1;
    console.log(`[FormGrid] 找到表单索引: ${index}`);
    
    if (index !== -1) {
      // 创建更新后的表单数组
      const updatedForms = [...props.forms];
      
      // 添加更新时间
      const formWithTimestamp = {
        ...updatedForm,
        updatedAt: new Date().toISOString()
      };
      
      updatedForms[index] = formWithTimestamp;
      
      console.log(`[FormGrid] 准备更新表单数组，长度: ${updatedForms.length}`);
      
      // 触发更新事件
      emit('update:modelValue', updatedForms);
      
      // 确保在DOM更新后执行
      nextTick(() => {
        console.log('[FormGrid] 表单更新后DOM已更新');
        // 提示成功
        ElMessage.success('表单已更新');
      });
    } else {
      console.warn(`[FormGrid] 未找到要更新的表单: ${updatedForm.id}`);
      ElMessage.warning('未找到要更新的表单');
    }
  } catch (error) {
    console.error('[FormGrid] 更新表单失败:', error);
    handleError(error, '更新表单失败');
  }
};

// 表单删除处理函数
const handleDeleteForm = (formToDelete) => {
  try {
    // 确认删除
    ElMessageBox.confirm('确定要删除此表单吗？', '警告', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    }).then(() => {
      // 过滤掉要删除的表单
      const updatedForms = props.forms.filter(f => f.id !== formToDelete.id);
      // 触发更新事件
      emit('update:modelValue', updatedForms);
      // 提示成功
      ElMessage.success('表单已删除');
    }).catch(() => {
      // 用户取消删除
      ElMessage.info('已取消删除');
    });
  } catch (error) {
    handleError(error, '删除表单失败');
  }
};

// 更新表单位置
const updateFormPosition = ({ formId, position }) => {
  try {
    console.log(`[FormGrid] 更新表单位置: ${formId}`, position);
    
    // 查找表单索引
    const index = forms.value.findIndex(f => f.id === formId);
    if (index !== -1) {
      // 更新表单位置
      const updatedForms = [...forms.value];
      updatedForms[index] = {
        ...updatedForms[index],
        position: { ...position }
      };
      
      // 更新内部表单状态
      forms.value = [...updatedForms];
      
      // 触发更新事件
      emit('update:modelValue', updatedForms);
      
      console.log(`[FormGrid] 表单位置更新成功: ${formId}`, position);
    } else {
      console.warn(`[FormGrid] 未找到要更新位置的表单: ${formId}`);
    }
  } catch (error) {
    handleError(error, '更新表单位置失败');
  }
};

// 更新表单尺寸
const updateFormSize = ({ formId, size }) => {
  try {
    console.log(`[FormGrid] 更新表单尺寸: ${formId}`, size);
    
    // 查找表单索引
    const index = forms.value.findIndex(f => f.id === formId);
    if (index !== -1) {
      // 更新表单尺寸
      const updatedForms = [...forms.value];
      updatedForms[index] = {
        ...updatedForms[index],
        size: { ...size }
      };
      
      // 更新内部表单状态
      forms.value = [...updatedForms];
      
      // 触发更新事件
      emit('update:modelValue', updatedForms);
      
      console.log(`[FormGrid] 表单尺寸更新成功: ${formId}`, size);
    } else {
      console.warn(`[FormGrid] 未找到要更新尺寸的表单: ${formId}`);
    }
  } catch (error) {
    handleError(error, '更新表单尺寸失败');
  }
};

// 处理表单鼠标按下事件，提升被点击表单的zIndex
const handleFormMouseDown = (clickedForm) => {
  try {
    console.log(`[FormGrid] 表单鼠标按下: ${clickedForm.id}`);
    
    // 查找表单索引
    const index = forms.value.findIndex(f => f.id === clickedForm.id);
    if (index !== -1) {
      // 获取当前最大的zIndex
      const maxZIndex = Math.max(...forms.value.map(f => f.zIndex || 1), 1);
      
      // 更新被点击表单的zIndex为最大值+1
      const updatedForms = [...forms.value];
      updatedForms[index] = {
        ...updatedForms[index],
        zIndex: maxZIndex + 1
      };
      
      // 更新内部表单状态
      forms.value = [...updatedForms];
      
      // 触发更新事件
      emit('update:modelValue', updatedForms);
      
      console.log(`[FormGrid] 表单zIndex更新成功: ${clickedForm.id}`, maxZIndex + 1);
    } else {
      console.warn(`[FormGrid] 未找到要提升zIndex的表单: ${clickedForm.id}`);
    }
  } catch (error) {
    handleError(error, '处理表单鼠标按下事件失败');
  }
};

// 表单保存处理函数
const handleFormSave = (updatedForm) => {
  try {
    console.log('[FormGrid] 开始保存编辑的表单', updatedForm);
    
    // 确保 props.forms 是一个数组
    const currentForms = Array.isArray(props.forms) ? [...props.forms] : [];
    console.log('[FormGrid] 当前表单数量:', currentForms?.length ?? 0);
    
    // 查找并更新表单
    const formIndex = currentForms.findIndex(form => form.id === updatedForm.id);
    if (formIndex !== -1) {
      // 更新表单
      currentForms[formIndex] = { ...updatedForm };
    } else {
      // 如果没找到表单，可能是新增的
      currentForms.push({ ...updatedForm });
    }
    
    // 更新内部表单状态
    forms.value = [...currentForms];
    
    // 触发更新事件
    emit('update:modelValue', currentForms);
    
    // 隐藏表单编辑器
    showFormEditor.value = false;
    selectedForm.value = null;
    
    // 提示成功
    ElMessage.success('表单保存成功');
    
    console.log('[FormGrid] 表单保存成功，当前表单数量:', currentForms.length);
  } catch (error) {
    console.error('[FormGrid] 保存表单失败:', error);
    handleError(error, '保存表单失败');
  }
};

// 处理编辑表单
const handleEditForm = (formToEdit) => {
  try {
    if (!props.editable) {
      ElMessage.warning('当前页面不可编辑，请先解锁页面');
      return;
    }
    
    // 设置当前选中的表单
    selectedForm.value = { ...formToEdit };
    
    // 显示表单编辑器
    showFormEditor.value = true;
    
    console.log('[FormGrid] 打开表单编辑器，表单ID:', formToEdit.id);
  } catch (error) {
    console.error('[FormGrid] 编辑表单失败:', error);
    handleError(error, '编辑表单失败');
  }
};

// 隐藏表单编辑器
const hideFormEditor = () => {
  showFormEditor.value = false;
  selectedForm.value = null;
};

// 通过defineExpose暴露方法
defineExpose({
  handleAddForm,
  saveAddForm,
  cancelAddForm
})

// =====================================================
// = 6. 生命周期钩子
// =====================================================
// 计算网格容器样式
const gridContainerStyle = computed(() => {
  if (props.pageSize) {
    const { width, height, unit } = props.pageSize;
    return {
      width: `${width}${unit}`,
      minHeight: `${height}${unit}`,
      border: '1px solid #eee',
      position: 'relative',
      overflow: 'auto'
    };
  }
  return {};
});

// 在组件挂载时初始化
onMounted(() => {
  try {
    // 初始化卡片样式服务
    if (cardStyleService.setContainer) {
      cardStyleService.setContainer(containerRef.value)
    }

    if (cardStyleService.setContentRefs) {
      cardStyleService.setContentRefs(contentRefs.value)
    }

    if (cardStyleService.setPositions) {
      cardStyleService.setPositions(positions.value)
    }


    // 监听表单数据变化 - 使用立即执行和深度监听
    watch(() => props.forms, (newVal) => {
      console.log('[FormGrid] 检测到表单数据变化:', newVal);
      // 添加深度克隆和空值处理
      // 保持响应式引用，避免重新赋值破坏响应式
      const validated = validateForms(newVal);
      forms.value.splice(0, forms.value.length, ...validated);
      nextTick(() => {
        initializeGridLayout();
        // 传递原始数据避免响应式嵌套
        // 总是触发更新事件以确保数据同步
        emit('update:modelValue', toRaw(forms.value));
      });
    }, { deep: true, immediate: true });

    // 添加currentPageIdx监听
    watch(() => props.pageSize, (newVal) => {
      console.log('[FormGrid] 页面尺寸更新:', newVal);
      initializeCardStyleService(newVal);
    });

    // 优化加载状态处理
    nextTick(() => {
      // 当表单数据就绪时立即更新加载状态
      if (forms.value.length > 0) {
        loading.value = false
      } else {
        // 设置安全超时
        setTimeout(() => {
          loading.value = false
        }, 100)
      }
    })
  } catch (err) {
    handleError(err, '初始化组件失败')
  }
})

</script>

<template>
  <div class="form-grid-container">
    <!-- 错误提示 -->
    <div v-if="error" class="error-boundary">
      <div class="error-boundary__title">发生错误</div>
      <div class="error-boundary__message">{{ error.message }}</div>
      <div class="error-boundary__info">{{ errorInfo }}</div>
      <el-button @click="resetError">重试</el-button>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-state">
      <el-spin>
        <el-icon name="loading" spin /> 加载中...
      </el-spin>
    </div>

    <!-- 表单网格内容 -->
    <div v-if="!loading && !error && !showAddFormEditor && !showFormEditor" class="form-grid" ref="containerRef" :style="gridContainerStyle">
      <!-- 表单卡片列表 -->
      <div class="form-cards">
        <SingleFormShow
          v-for="(form, index) in forms"
          :key="form.id"
          :form="form"
          :editable="editable"
          :card-style-on="cardStyleOn"
          :position="form.position || { x: 20, y: 20 }"
          :size="form.size || { width: 200, height: 100 }"
          @edit="handleEditForm"
          @delete="handleDeleteForm"
          @dblclick="handleEditForm"
          @update-position="updateFormPosition"
          @update-size="updateFormSize"
          @mousedown="handleFormMouseDown(form)"
        />
      </div>
    </div>

    <!-- 添加表单编辑器 -->
    <div v-if="showAddFormEditor" class="form-editor-overlay">
      <div class="form-editor-wrapper">
        <FormEditor :form="addFormData" @save="saveAddForm" @cancel="cancelAddForm" @error="handleFormEditorError" :key="addFormDataKey" />
      </div>
    </div>

    <!-- 表单编辑器 -->
    <div class="form-editor-overlay" v-if="showFormEditor">
      <div class="form-editor-wrapper">
        <FormEditor 
          :form="selectedForm" 
          :isEdit="true" 
          @save="handleFormSave" 
          @cancel="hideFormEditor"
          @error="handleFormEditorError" 
          :key="selectedForm?.id || 'editor'" 
        />
      </div>
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
  padding: var(--spacing-base);
  /* 添加对页面尺寸的支持 */
  box-sizing: border-box;
  margin: 0 auto;
}

.form-editor-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.5);
  z-index: 2000;
  display: flex;
  justify-content: center;
  align-items: center;
  backdrop-filter: blur(2px);
  padding: 20px;
  box-sizing: border-box;
}

.form-editor-wrapper {
  background: white;
  border-radius: 8px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.15);
  width: min(800px, calc(100vw - 40px));
  max-width: min(800px, calc(100vw - 40px));
  max-height: calc(100vh - 40px);
  overflow-y: hidden;
  animation: slideDown 0.3s ease-out;
  display: flex;
  flex-direction: column;
}

.form-editor-wrapper :deep(.form-editor) {
  max-height: calc(100vh - 40px - 20px);
  overflow-y: auto;
  flex: 1;
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* 表单编辑器过渡动画 */
.form-editor-enter-active, .form-editor-leave-active {
  transition: opacity 0.3s ease;
}

.form-editor-enter-from, .form-editor-leave-to {
  opacity: 0;
}

.form-editor-enter-active .form-editor-wrapper,
.form-editor-leave-active .form-editor-wrapper {
  transition: all 0.3s ease;
}

.form-editor-enter-from .form-editor-wrapper,
.form-editor-leave-to .form-editor-wrapper {
  opacity: 0;
  transform: scale(0.95) translateY(-10px);
}

/* 响应式设计优化 */
@media (max-width: 768px) {
  .form-editor-overlay {
    padding: 15px;
  }
  
  .form-editor-wrapper {
    width: calc(100vw - 30px);
    max-width: calc(100vw - 30px);
    max-height: calc(100vh - 30px);
  }
  
  .form-editor-wrapper :deep(.form-editor) {
    max-height: calc(100vh - 30px - 20px);
  }
}

@media (max-width: 480px) {
  .form-editor-overlay {
    padding: 10px;
  }
  
  .form-editor-wrapper {
    width: calc(100vw - 20px);
    max-width: calc(100vw - 20px);
    max-height: calc(100vh - 20px);
  }
  
  .form-editor-wrapper :deep(.form-editor) {
    max-height: calc(100vh - 20px - 20px);
  }
}

@media (max-height: 700px) {
  .form-editor-wrapper {
    max-height: calc(100vh - 20px);
  }
}

@media (max-height: 500px) {
  .form-editor-overlay {
    padding: 5px;
  }
  
  .form-editor-wrapper {
    max-height: calc(100vh - 10px);
  }
}

/* =====================================================
   = 6. 错误状态样式 =
   ====================================================== */
.error-boundary {
  background-color: var(--background-color);
  border-left: 4px solid var(--danger-color);
  padding: var(--spacing-base);
  margin: var(--spacing-small) 0;
  color: var(--danger-color);
  font-size: 14px;
}

.error-boundary__title {
  font-weight: bold;
  margin-bottom: var(--spacing-small);
}

.error-boundary__message {
  margin-bottom: var(--spacing-small);
}

.error-boundary__info {
  font-size: 12px;
  opacity: 0.8;
}

/* =====================================================
   = 2. 工具栏样式 =
   ====================================================== */
.toolbar {
  margin-bottom: var(--spacing-base);
  text-align: right;
}

/* =====================================================
   = 3. 表单卡片样式 =
   ====================================================== */
.form-card-wrapper {
  position: absolute;
  transition: transform var(--transition-duration) var(--transition-function);
}

.form-card {
  position: relative;
  cursor: pointer;
  transition: all var(--transition-duration) var(--transition-function);
}

.form-card:hover {
  transform: scale(1.02);
  box-shadow: var(--box-shadow-base);
}

.resize-handle {
  position: absolute;
  right: 0;
  bottom: 0;
  width: 15px;
  height: 15px;
  background-color: var(--primary-color);
  cursor: se-resize;
  z-index: 10;
}

/* =====================================================
   = 4. 右键菜单样式 =
   ====================================================== */
.context-menu {
  position: absolute;
  background-color: #fff;
  border: 1px solid var(--border-color);
  border-radius: var(--border-radius-base);
  box-shadow: var(--box-shadow-light);
  z-index: 999;
}

.menu-grid {
  display: flex;
  flex-direction: column;
  padding: var(--spacing-small) 0;
}

.menu-grid .el-button {
  width: 100%;
  text-align: left;
  padding: var(--spacing-small) var(--spacing-base);
}

/* =====================================================
   = 5. 加载状态样式 =
   ====================================================== */
.loading-state {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  z-index: 100;
  background: rgba(255, 255, 255, 0.9);
  padding: var(--spacing-base);
  border-radius: var(--border-radius-base);
  box-shadow: var(--box-shadow-base);
}

/* =====================================================
   = 7. 响应式样式 =
   ====================================================== */
@media (max-width: 768px) {
  .form-grid {
    padding: var(--spacing-small);
  }

  .form-card {
    width: 100%;
    margin: var(--spacing-small) 0;
  }
}

/* ============================
   = 5. 加载状态样式 =
   ====================================================== */
.loading-state {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  z-index: 100;
  background: rgba(255, 255, 255, 0.9);
  padding: var(--spacing-base);
  border-radius: var(--border-radius-base);
  box-shadow: var(--box-shadow-base);
}

/* =====================================================
   = 7. 响应式样式 =
   ====================================================== */
@media (max-width: 768px) {
  .form-grid {
    padding: var(--spacing-small);
  }

  .form-card {
    width: 100%;
    margin: var(--spacing-small) 0;
  }
}
</style>