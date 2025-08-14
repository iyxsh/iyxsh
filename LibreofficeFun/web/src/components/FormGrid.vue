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
      <el-icon class="is-loading">
        <svg viewBox="0 0 1024 1024" fill="currentColor" xmlns="http://www.w3.org/2000/svg" data-v-72eee16e="">
          <path d="M512 512m-224 0a224 224 0 1 0 448 0 224 224 0 1 0-448 0Z" fill="#1890ff" data-v-72eee16e=""></path>
          <path
            d="M957.1 512c0-29.4-4.2-57.8-11.9-84.7-7.7-26.9-18.8-51.9-32.7-74.4-14-22.5-31.5-42.3-51.8-59.1-20.3-16.8-43.2-30.3-68.2-40.1-25-9.8-51.8-15.9-80-17.9-28.2-2-57.2-.3-85.9-7.1-28.7-6.8-56.2-17.9-81.6-32.9-25.4-15-48.4-33.7-68.5-55.8-20.1-22.1-36.9-47.4-50.1-75.2-13.2-27.9-22.8-58.2-28.5-90.5-5.7 32.3-8.5 66.1-8.5 100.8 0 29.4 4.2 57.8 11.9 84.7 7.7 26.9 18.8 51.9 32.7 74.4 14 22.5 31.5 42.3 51.8 59.1 20.3 16.8 43.2 30.3 68.2 40.1 25 9.8 51.8 15.9 80 17.9 28.2 2 57.2-.3 85.9-7.1 28.7-6.8 56.2-17.9 81.6-32.9 25.4-15 48.4-33.7 68.5-55.8 20.1-22.1 36.9-47.4 50.1-75.2 13.2-27.9 22.8-58.2 28.5-90.5 5.8-32.3 8.6-66.1 8.6-100.8z m-89.6 0c0 25.6-3.6 50.4-10.5 74-6.9 23.5-16.3 45.5-27.8 65.4-11.5 19.9-25.3 37.4-40.9 52.1-15.6 14.7-33.2 26.1-52.2 33.8-19 7.7-39.4 11.9-60.5 12.4-21.1.5-42.3-2.8-62.7-9.9-20.4-7.1-39.7-17.9-57.5-32-17.8-14.1-33.8-31.7-47.6-52.4-13.8-20.7-25.2-44.5-33.8-70.9-8.7-26.4-14.7-55.2-17.7-85.6-3-30.4-3-62.4 0-95.2 3-32.8 9-64.4 17.7-93.6 8.7-29.2 20.7-55.9 35.6-79.6 14.9-23.7 32.6-43.8 52.6-59.8 20-16 42.2-27.7 66.1-34.8 23.9-7.1 49.4-10.5 75.9-10.1 26.5.4 52.6 5 77.3 13.5 24.7 8.5 47.7 21.2 68.5 37.7 20.8 16.5 39.1 37.1 54.4 61.2 15.3 24.1 27.5 52 36.2 83 8.7 31 13.8 64.7 15 100.4 1.3 35.7-.9 72.8-6.4 110.4z"
            fill="#1890ff" data-v-72eee16e=""></path>
        </svg>
      </el-icon>
      <span>加载中...</span>
    </div>

    <!-- 表单网格内容 -->
    <div v-else-if="!showAddFormEditor && !showFormEditor" class="form-grid" ref="containerRef"
      :style="gridContainerStyle">
      <!-- 表单卡片列表 -->
      <SingleFormShow v-for="(form, index) in forms" :key="form.id" :form="form" :editable="editable"
        :card-style-on="cardStyleOn" :position="form.position || { x: 20, y: 20 }"
        :size="form.size || { width: 200, height: 100 }" @edit="handleEditForm" @delete="handleDeleteForm"
        @dblclick="handleEditForm" @update-position="handleUpdatePosition" @update-size="updateFormSize"
        @mousedown="handleFormMouseDown(form)" />
    </div>

    <!-- 添加表单编辑器 -->
    <div v-else-if="showAddFormEditor" class="form-editor-overlay">
      <div class="form-editor-wrapper">
        <FormEditor :form="addFormData" @save="saveAddForm" @cancel="cancelAddForm" @error="handleFormEditorError"
          :key="addFormDataKey" />
      </div>
    </div>

    <!-- 表单编辑器 -->
    <div class="form-editor-overlay" v-else-if="showFormEditor">
      <div class="form-editor-wrapper">
        <FormEditor :form="selectedForm" :isEdit="true" @save="handleFormSave" @cancel="hideFormEditor"
          @error="handleFormEditorError" :key="selectedForm?.id || 'editor'" />
      </div>
    </div>
  </div>
</template>
<script setup>
// =====================================================
// = 1. 导入依赖
// =====================================================
import { ref, computed, onMounted, watch, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useEventBus } from '../utils/eventBus'
import SingleFormShow from './SingleFormShow.vue'
import FormEditor from './FormEditor.vue'
import errorLogService from '@/services/errorLogService'
import { getCurrentInstance } from 'vue'
import { savePositions } from '../services/formPositionService'

// =====================================================
// = 2. 定义 props
// =====================================================
const props = defineProps({
  modelValue: {
    type: Array,
    required: true
  },
  editable: {
    type: Boolean,
    default: false
  },
  cardStyleOn: {
    type: Boolean,
    default: true
  },
  pageSize: {
    type: [Object, String],
    default: null
  },
  currentPageName: {
    type: String,
    required: true
  }
})

// 定义 sheetname 响应式变量
const sheetname = ref(props.currentPageName)

// 监听 currentPageName 的变化
watch(() => props.currentPageName, (newName) => {
  sheetname.value = newName // 动态更新 sheetname
})

// =====================================================
// = 3. 响应式变量定义
// =====================================================
// 定义 containerRef
const containerRef = ref(null)

// 定义 positions
const positions = ref([])

// 表单相关数据
/** @type {import('vue').Ref<Form[]>} */
const forms = ref([])

// 添加表单数据
/** @type {import('vue').Ref<Form|null>} */
const addFormData = ref(null)

// 选中的表单
/** @type {import('vue').Ref<Form|null>} */
const selectedForm = ref(null)

// 错误状态
const error = ref(null)
// 错误详情
const errorInfo = ref('')
// 加载状态
const loading = ref(true)
// 显示添加表单对话框
const showAddDialog = ref(false)
// 添加表单数据的key，用于强制刷新FormEditor组件
const addFormDataKey = ref(0)
// 是否显示表单编辑器
const showFormEditor = ref(false)
// 是否显示添加表单的编辑器
const showAddFormEditor = ref(false)

// 表单验证函数
const validateForms = (forms) => {
  try {
    // 添加对forms为undefined或null的检查
    if (!forms) {
      console.warn('[FormGrid] 接收到空的表单数据');
      return [];
    }

    if (!Array.isArray(forms)) {
      console.warn('[FormGrid] 接收到非数组表单数据:', forms);
      return [];
    }

    return forms.map((form, index) => {
      // 检查表单是否已经有所有必需的属性
      const hasAllRequiredFields = form.id &&
        form.position &&
        form.size &&
        typeof form.zIndex !== 'undefined';

      // 如果表单已经具备所有必需字段，则直接返回，避免创建新对象
      if (hasAllRequiredFields) {
        return form;
      }

      // 确保表单有唯一ID，但如果已存在则不重新生成
      const formId = form.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`;

      // 确保表单有位置信息
      const position = form.position || { x: 20, y: 20 };

      // 确保表单有尺寸信息
      const size = form.size || { width: 200, height: 100 };

      // 确保表单有zIndex，如果没有则根据索引设置
      const zIndex = form.zIndex !== undefined ? form.zIndex : index + 1;

      // 只有在确实需要修改时才创建新对象
      if (form.id !== formId ||
        form.position !== position ||
        form.size !== size ||
        form.zIndex !== zIndex) {
        return {
          ...form,
          id: formId,
          position,
          size,
          zIndex
        };
      }

      return form;
    });
  } catch (e) {
    handleError(e, '表单数据校验失败');
    return [];
  }
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

    // 生成更可靠的唯一ID，使用时间戳和随机字符串组合
    const generateUniqueId = () => {
      return `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}-${Math.random().toString(36).substring(2, 5)}`;
    };

    // 初始化添加表单数据，确保每次都生成新的唯一ID
    addFormData.value = {
      id: generateUniqueId(),
      title: '',
      value: '',
      remark: '',
      media: '',
      mediaType: 'image',
      showTitle: true,
      showValue: true,
      showRemark: true,
      showMedia: true,
      style: {},
      elementStyles: {
        title: {
          enabled: false
        },
        value: {
          enabled: false
        },
        remark: {
          enabled: false
        },
        media: {
          enabled: false
        }
      },
      position: {
        x: 20 + (forms.value?.length * 20),
        y: 20 + (forms.value?.length * 20)
      },
      size: { width: 200, height: 100 },
      createdAt: new Date().toISOString()
    }

    // 更新key以强制刷新FormEditor组件
    addFormDataKey.value = Date.now() + 1 // 增加一个偏移量确保key变化

    // 显示添加表单编辑器
    showAddFormEditor.value = true

    console.log('[FormGrid] 显示添加表单编辑器，表单ID:', addFormData.value.id)
  } catch (error) {
    handleError(error, '处理添加表单失败')
  }
}

const emit = defineEmits(['update:modelValue'])

// 保存添加的表单
const saveAddForm = async (formData) => {
  try {
    console.log('[FormGrid] 开始保存表单', formData);

    // 确保 forms.value 是一个数组
    const currentForms = Array.isArray(forms.value) ? [...forms.value] : [];
    console.log('[FormGrid] 当前表单数量:', currentForms?.length ?? 0);

    // 生成更可靠的唯一ID，确保每次都有新ID
    const newForm = {
      ...formData,
      id: formData.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`,
      createdAt: formData.createdAt || new Date().toISOString()
    };
    // 优化：只保存media的URL
    if (newForm.media && typeof newForm.media === 'object' && newForm.media.url) {
      newForm.media = newForm.media.url;
    } else if (typeof newForm.media === 'string') {
      // 如果已经是URL字符串，保留
    } else {
      newForm.media = '';
    }
    // 修复 mediaType 和 showMedia
    if (!newForm.media) {
      newForm.mediaType = '';
      newForm.showMedia = false;
    } else {
      // 如果是图片或视频，根据URL后缀判断类型
      if (typeof newForm.media === 'string') {
        if (/\.(jpg|jpeg|png|gif|bmp|webp)$/i.test(newForm.media)) {
          newForm.mediaType = 'image';
        } else if (/\.(mp4|webm|ogg|mov|avi)$/i.test(newForm.media)) {
          newForm.mediaType = 'video';
        } else {
          newForm.mediaType = '';
        }
        newForm.showMedia = true;
      }
    }
    // 确保表单的基本字段存在
    if (!newForm.title) newForm.title = '';
    if (!newForm.value) newForm.value = '';
    if (!newForm.remark) newForm.remark = '';
    if (!newForm.media) newForm.media = '';
    // mediaType 已在上面处理
    // 确保样式字段存在
    if (!newForm.style || typeof newForm.style !== 'object') {
      newForm.style = {
        backgroundColor: '#ffffff',
        color: '#333333',
        borderWidth: 1,
        borderStyle: 'solid',
        borderColor: '#e4e7ed',
        borderRadius: 4,
        fontSize: 14,
        padding: 12,
        hasShadow: true
      };
    }
    // 确保元素样式字段存在
    if (!newForm.elementStyles || typeof newForm.elementStyles !== 'object') {
      newForm.elementStyles = {
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
      };
    }
    // 确保显示设置字段存在
    if (typeof newForm.showTitle !== 'boolean') newForm.showTitle = true;
    if (typeof newForm.showValue !== 'boolean') newForm.showValue = true;
    if (typeof newForm.showRemark !== 'boolean') newForm.showRemark = true;
    if (typeof newForm.showMedia !== 'boolean') newForm.showMedia = !!newForm.media;
    // 确保位置和尺寸字段存在
    if (!newForm.position || typeof newForm.position !== 'object') {
      newForm.position = {
        x: 20 + (currentForms.length * 20),
        y: 20 + (currentForms.length * 20)
      };
    }
    if (!newForm.size || typeof newForm.size !== 'object') {
      newForm.size = { width: 200, height: 100 };
    }

    // 创建新的表单数组，确保不修改原数组
    const updatedForms = [...currentForms, newForm];
    console.log('[FormGrid] 更新后表单数量:', updatedForms.length);

    // 更新内部表单状态，使用响应式方式更新
    forms.value = updatedForms;

    // 触发更新事件，确保父组件接收到更新
    emit('update:modelValue', updatedForms);

    // 调用ApiServiceManager保存表单数据
    const apiService = getApiService()
    if (apiService) {
      try {
        // 获取文件名和页面名称
        const urlParams = new URLSearchParams(window.location.search);
        const filename = urlParams.get('fileName');

        if (!filename) {
          throw new Error('无法从URL参数中获取文件名');
        }

        // 将表单数据转换为符合接口要求的形式
        const updatecells = {};
        const fieldToColumn = {
          'id': 'A',
          'title': 'B',
          'value': 'C',
          'remark': 'D',
          'media': 'E',
          'mediaType': 'F',
          'showTitle': 'G',
          'showValue': 'H',
          'showRemark': 'I',
          'showMedia': 'J',
        };

        updatedForms.forEach((form, index) => {
          const row = index + 1;

          Object.keys(form).forEach(fieldName => {
            const skipFields = ['id', 'createdAt', 'style', 'elementStyles', 'position', 'size', 'zIndex'];
            if (skipFields.includes(fieldName)) {
              return;
            }

            const fieldValue = form[fieldName];

            if (fieldValue !== undefined && fieldValue !== null && fieldValue !== '') {
              let cellValue = fieldValue;
              if (typeof fieldValue === 'boolean') {
                cellValue = fieldValue ? 'true' : 'false';
              } else if (typeof fieldValue === 'object') {
                cellValue = JSON.stringify(fieldValue);
              }

              let column = fieldToColumn[fieldName];
              if (!column) {
                const additionalFields = Object.keys(form).filter(
                  key => !Object.keys(fieldToColumn).includes(key)
                );
                console.warn('[FormGrid] handleFormSave 发现未知字段:', additionalFields);
              }

              const cellAddress = `${column}${row}`;
              updatecells[cellValue] = cellAddress;
            }
          });
        });

        // 构造符合接口定义的更新数据
        const updateRequest = {
          filename: filename,
          updatedata: [{
            sheetname: sheetname.value,
            updatecells: updatecells
          }]
        };

        const response = await apiService.updateFile(updateRequest);
        if(response.errorCode !== 1000 || response.errorMessage !== "Success")
        {
          console.error('[FormGrid] 同步表单到服务器失败:', response);
          ElMessage.error('同步表单到服务器失败: ' + (response.errorMessage || '未知错误'));
          return;
        }
        console.log('[FormGrid] 表单添加成功并同步到服务器')
      } catch (error) {
        console.error('[FormGrid] 同步表单到服务器失败:', error)

        // 根据错误类型提供不同的用户提示
        let errorMessage = '表单同步失败';
        if (error.message) {
          if (error.message.includes('网络') || error.message.includes('Network')) {
            errorMessage = '网络连接失败，请检查网络后重试';
          } else if (error.message.includes('文件名')) {
            errorMessage = error.message;
          } else if (error.message.includes('权限') || error.message.includes('Permission')) {
            errorMessage = '权限不足，无法保存到服务器';
          } else {
            errorMessage = `同步失败: ${error.message}`;
          }
        }

        ElMessage.error(errorMessage);

        // 记录错误日志
        errorLogService.addErrorLog(
          error,
          '表单添加同步失败',
          'error'
        );
      }
    } else {
      console.warn('[FormGrid] ApiServiceManager不可用，仅保存到本地存储')
      ElMessage.warning('当前仅保存到本地，无法同步到服务器')
    }

    // 确保在DOM更新后执行
    nextTick(() => {
      console.log('[FormGrid] 表单保存后DOM已更新');

      // 隐藏添加表单编辑器
      showAddFormEditor.value = false;

      // 更新key以确保下次打开时强制刷新
      addFormDataKey.value = Date.now() + Math.random();

      // 提示成功
      ElMessage.success('表单添加成功');

      // 清空添加表单数据
      addFormData.value = null;

      // 更新卡片位置信息
      const newPositions = updatedForms.map(form => ({
        id: form.id,
        ...form.position,
        width: form.size?.width || 200,
        height: form.size?.height || 100
      }));

      // 更新全局位置信息
      positions.value = newPositions;
    });
  } catch (error) {
    console.error('[FormGrid] 保存表单失败:', error);
    handleError(error, '保存表单失败');

    // 确保即使出错也隐藏表单编辑器并重置状态
    nextTick(() => {
      showAddFormEditor.value = false;
      addFormDataKey.value = Date.now() + Math.random();
      addFormData.value = null;
    });
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

// 表单删除处理函数
const handleDeleteForm = (formToDelete) => {
  try {
    // 确保props.modelValue存在且为数组
    if (!props.modelValue || !Array.isArray(props.modelValue)) {
      console.warn('[FormGrid] props.modelValue不是有效数组:', props.modelValue);
      throw new Error('表单数据无效');
    }

    // 确认删除
    ElMessageBox.confirm('确定要删除此表单吗？', '警告', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    }).then(() => {
      // 过滤掉要删除的表单
      const updatedForms = props.modelValue.filter(f => f.id !== formToDelete.id);
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

// 更新表单尺寸
const updateFormSize = ({ formId, size }) => {
  try {
    console.log(`[FormGrid] 更新表单尺寸: ${formId}`, size);

    // 验证参数
    if (!formId || !size || typeof size.width === 'undefined' || typeof size.height === 'undefined') {
      throw new Error('无效的尺寸数据或表单ID');
    }

    // 确保props.modelValue存在且为数组
    if (!props.modelValue || !Array.isArray(props.modelValue)) {
      console.warn('[FormGrid] props.modelValue不是有效数组:', props.modelValue);
      throw new Error('表单数据无效');
    }

    // 获取容器尺寸以限制调整大小边界
    let containerWidth = 800;
    let containerHeight = 600;
    if (containerRef.value) {
      containerWidth = containerRef.value.clientWidth;
      containerHeight = containerRef.value.clientHeight;
    } else if (props.pageSize) {
      containerWidth = props.pageSize.width;
      containerHeight = props.pageSize.height;
    }

    // 创建更新后的表单数组
    const updatedForms = props.modelValue.map(form => {
      if (form.id === formId) {
        // 确保表单不会调整得太大以至于超出容器边界
        const boundedSize = {
          width: Math.max(50, Math.min(size.width, containerWidth - (form.position?.x || 0))),
          height: Math.max(50, Math.min(size.height, containerHeight - (form.position?.y || 0)))
        };

        // 确保创建新的尺寸对象以避免引用问题
        return {
          ...form,
          size: {
            width: Math.round(boundedSize.width),
            height: Math.round(boundedSize.height)
          }
        };
      }
      return form;
    });

    // 触发更新事件
    emit('update:modelValue', updatedForms);

    console.log(`[FormGrid] 表单尺寸更新成功: ${formId}`, size);
  } catch (error) {
    handleError(error, '更新表单尺寸失败');
  }
};

// 处理表单鼠标按下事件，提升被点击表单的zIndex
const handleFormMouseDown = (clickedForm) => {
  try {
    console.log(`[FormGrid] 表单鼠标按下: ${clickedForm.id}`);

    // 确保props.modelValue存在且为数组
    if (!props.modelValue || !Array.isArray(props.modelValue)) {
      console.warn('[FormGrid] props.modelValue不是有效数组:', props.modelValue);
      throw new Error('表单数据无效');
    }

    // 获取当前最大的zIndex
    const maxZIndex = Math.max(...props.modelValue.map(f => f.zIndex || 1), 1);

    // 创建更新后的表单数组
    const updatedForms = props.modelValue.map(form => {
      if (form.id === clickedForm.id) {
        // 确保创建新的对象以避免引用问题
        return {
          ...form,
          zIndex: maxZIndex + 1
        };
      }
      return form;
    });

    // 触发更新事件
    emit('update:modelValue', updatedForms);

    console.log(`[FormGrid] 表单zIndex更新成功: ${clickedForm.id}`, maxZIndex + 1);
  } catch (error) {
    handleError(error, '更新表单层级失败');
  }
};

// 处理表单位置更新事件
const handleUpdatePosition = ({ formId, position }) => {
  try {
    console.log(`[FormGrid] 更新表单位置: ${formId}`, position);

    // 验证参数
    if (!formId || !position || typeof position.x === 'undefined' || typeof position.y === 'undefined') {
      throw new Error('无效的位置数据或表单ID');
    }

    // 确保props.modelValue存在且为数组
    if (!props.modelValue || !Array.isArray(props.modelValue)) {
      console.warn('[FormGrid] props.modelValue不是有效数组:', props.modelValue);
      throw new Error('表单数据无效');
    }

    // 获取容器尺寸以限制拖动边界
    let containerWidth = 800;
    let containerHeight = 600;
    if (containerRef.value) {
      containerWidth = containerRef.value.clientWidth;
      containerHeight = containerRef.value.clientHeight;
    } else if (props.pageSize) {
      containerWidth = props.pageSize.width;
      containerHeight = props.pageSize.height;
    }

    // 创建更新后的表单数组
    const updatedForms = props.modelValue.map(form => {
      if (form.id === formId) {
        // 确保表单不会被拖出容器边界
        const boundedPosition = {
          x: Math.max(0, Math.min(position.x, containerWidth - (form.size?.width || 200))),
          y: Math.max(0, Math.min(position.y, containerHeight - (form.size?.height || 100)))
        };

        // 确保创建新的位置对象以避免引用问题
        return {
          ...form,
          position: {
            x: Math.round(boundedPosition.x),
            y: Math.round(boundedPosition.y)
          }
        };
      }
      return form;
    });

    // 触发更新事件
    emit('update:modelValue', updatedForms);

    // 记录调试信息
    const updatedForm = updatedForms.find(f => f.id === formId);
    console.log(`[FormGrid] 表单位置更新成功: ${formId}`, {
      oldPosition: props.modelValue.find(f => f.id === formId)?.position,
      newPosition: updatedForm?.position
    });

    // 更新全局位置信息用于样式计算
    const newPositions = updatedForms.map(form => ({
      id: form.id,
      ...form.position,
      width: form.size?.width || 200,
      height: form.size?.height || 100
    }));

    positions.value = newPositions;

    // 如果需要，更新卡片样式
  } catch (error) {
    console.error('[FormGrid] 更新表单位置失败:', error);
    handleError(error, '更新表单位置失败');
  }
};

// 表单保存处理函数
const handleFormSave = async (updatedForm) => {
  try {
    const instance = getCurrentInstance();
    console.log('[FormGrid] handleFormSave instance:', instance);
    if (instance && instance.appContext && instance.appContext.config && instance.appContext.config.globalProperties) {
      console.log('[FormGrid] handleFormSave globalProperties:', instance.appContext.config.globalProperties);
    } else {
      console.warn('[FormGrid] handleFormSave globalProperties 不存在');
    }
  } catch (e) {
    console.error('[FormGrid] handleFormSave 打印实例出错', e);
  }

  try {
    const index = forms.value.findIndex(form => form.id === updatedForm.id);
    if (index !== -1) {
      const currentForms = [...forms.value];
      let safeForm = { ...updatedForm };

      // 优化：只保存media的URL
      if (safeForm.media && typeof safeForm.media === 'object' && safeForm.media.url) {
        safeForm.media = safeForm.media.url;
      } else if (typeof safeForm.media !== 'string') {
        safeForm.media = '';
      }

      currentForms[index] = safeForm;
      forms.value = currentForms;

      // 同时更新父组件的modelValue
      const updatedModelValue = [...props.modelValue];
      const modelIndex = updatedModelValue.findIndex(form => form.id === updatedForm.id);
      if (modelIndex !== -1) {
        let safeModelForm = { ...updatedForm };
        if (safeModelForm.media && typeof safeModelForm.media === 'object' && safeModelForm.media.url) {
          safeModelForm.media = safeModelForm.media.url;
        } else if (typeof safeModelForm.media !== 'string') {
          safeModelForm.media = '';
        }
        updatedModelValue[modelIndex] = safeModelForm;
        emit('update:modelValue', updatedModelValue);
      }

      // 保存到 localStorage
      const positions = {};
      const sizes = {};
      currentForms.forEach(form => {
        if (form.position) {
          positions[form.id] = form.position;
        }
        if (form.size) {
          sizes[form.id] = form.size;
        }
      });
      savePositions(positions, sizes);

      // 调用 ApiServiceManager 更新表单数据
      const apiService = getApiService();
      if (apiService) {
        try {
          // 获取文件名（从 URL 参数中获取）
          const urlParams = new URLSearchParams(window.location.search);
          const filename = urlParams.get('fileName');
          if (!filename) {
            throw new Error('无法从 URL 参数中获取文件名');
          }
          // 验证必要参数
          if (!filename || filename === '未命名文件') {
            throw new Error('文件名无效，无法保存到服务器');
          }

          // 将表单数据转换为符合接口要求的形式
          const updatecells = {};
          const fieldToColumn = {
            'id': 'A',
            'title': 'B',
            'value': 'C',
            'remark': 'D',
            'media': 'E',
            'mediaType': 'F',
            'showTitle': 'G',
            'showValue': 'H',
            'showRemark': 'I',
            'showMedia': 'J',
          };

          currentForms.forEach((form, index) => {
            let safeMedia = form.media;
            if (safeMedia && typeof safeMedia === 'object' && safeMedia.url) {
              safeMedia = safeMedia.url;
            } else if (typeof safeMedia !== 'string') {
              safeMedia = '';
            }

            const row = index + 1;
            Object.keys(form).forEach(fieldName => {
              const skipFields = ['createdAt', 'style', 'elementStyles', 'position', 'size', 'zIndex'];
              if (skipFields.includes(fieldName)) {
                return;
              }

              let fieldValue = form[fieldName];
              if (fieldName === 'media') {
                fieldValue = safeMedia;
              }

              if (fieldValue !== undefined && fieldValue !== null && fieldValue !== '') {
                let cellValue = fieldValue;
                if (typeof fieldValue === 'boolean') {
                  cellValue = fieldValue ? 'true' : 'false';
                } else if (typeof fieldValue === 'object') {
                  cellValue = JSON.stringify(fieldValue);
                }

                let column = fieldToColumn[fieldName];
                if (!column) {
                  const additionalFields = Object.keys(form).filter(
                    key => !skipFields.includes(key) && !fieldToColumn[key]
                  );
                  const fieldIndex = additionalFields.indexOf(fieldName);
                  column = String.fromCharCode(75 + fieldIndex);
                }

                const cellAddress = `${column}${row}`;
                updatecells[cellValue] = cellAddress;
              }
            });
          });

          if (Object.keys(updatecells).length === 0) {
            console.warn('[FormGrid] 没有有效的数据需要更新');
            ElMessage.warning('没有有效的数据需要保存到服务器');
            return;
          }

          const updateRequest = {
            filename: filename,
            updatedata: [{
              sheetname: sheetname.value,
              updatecells: updatecells
            }]
          };

          const response = await apiService.updateFile(updateRequest);
          if(response.errorCode !== 1000 || response.errorMessage !== "Success")
          {
            console.error('[FormGrid] 同步表单更新到服务器失败:', response);
            ElMessage.error('同步表单更新到服务器失败: ' + (response.errorMessage || '未知错误'));
            return;
          }
        } catch (error) {
          console.error('[FormGrid] 同步表单更新到服务器失败:', error);
          let errorMessage = '表单更新同步失败';

          if (error.message) {
            if (error.message.includes('网络') || error.message.includes('Network')) {
              errorMessage = '网络连接失败，请检查网络后重试';
            } else if (error.message.includes('文件名') || error.message.includes('页面名称')) {
              errorMessage = error.message;
            } else if (error.message.includes('权限') || error.message.includes('Permission')) {
              errorMessage = '权限不足，无法保存到服务器';
            } else {
              errorMessage = `同步失败: ${error.message}`;
            }
          }

          ElMessage.error(errorMessage);
          errorLogService.addErrorLog(error, '表单更新同步失败', 'error');
        }
      } else {
        console.warn('[FormGrid] ApiServiceManager不可用，仅保存到本地存储')
        ElMessage.warning('当前仅保存到本地，无法同步到服务器')
      }

      hideFormEditor();
      ElMessage.success('表单更新成功');
    } else {
      ElMessage.error('未找到要更新的表单');
    }
  } catch (error) {
    console.error('[FormGrid] 处理表单保存失败:', error);
    handleError(error, '处理表单保存失败');
  }
};

// 处理编辑表单
const handleEditForm = (formToEdit) => {
  try {
    console.log('[FormGrid] 处理表单编辑请求:', formToEdit?.id, '类型:', formToEdit?.type);

    // 检查是否是视频表单
    if (formToEdit && formToEdit.type === 'video' && formToEdit.content) {
      // 检查是否是base64格式的视频
      if (typeof formToEdit.content === 'string' && formToEdit.content.startsWith('data:video/')) {
        console.log('[FormGrid] 检测到base64视频表单，准备优化处理');
        // 使用requestIdleCallback推迟执行，避免强制回流（如果浏览器支持）
        if (window.requestIdleCallback) {
          window.requestIdleCallback(() => {
            continueEditForm(formToEdit);
          }, { timeout: 1000 });
        } else {
          // 降级到setTimeout
          setTimeout(() => {
            continueEditForm(formToEdit);
          }, 0);
        }
        return;
      }
    }

    // 正常处理流程
    continueEditForm(formToEdit);
  } catch (error) {
    console.error('[FormGrid] 编辑表单失败:', error);
    handleError(error, '编辑表单失败');
  }
};

// 实际执行表单编辑的函数
const continueEditForm = (formToEdit) => {
  // 检查页面是否可编辑
  if (!props.editable) {
    ElMessage.warning('当前页面不可编辑，请先解锁页面');
    return;
  }

  // 设置当前选中的表单
  selectedForm.value = { ...formToEdit };

  // 显示表单编辑器
  showFormEditor.value = true;

  console.log('[FormGrid] 打开表单编辑器，表单ID:', formToEdit.id);
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
  cancelAddForm,
  handleEditForm
})

// 在组件挂载时初始化
onMounted(() => {
  try {
    // 初始化卡片样式服务

    // 初始化表单数据
    console.log('[FormGrid] 初始化表单数据:', forms.value?.length, '项', 'props数据:', props.modelValue?.length || 0);
    if (!props.modelValue || props.modelValue.length === 0) {
      console.warn('[FormGrid] 接收到空表单数据');
      loading.value = false;
    }

    // 监听表单数据变化 - 使用立即执行和深度监听
    watch(() => props.modelValue, (newVal) => {
      console.log('[FormGrid] 检测到表单数据变化:', newVal);

      // 检查新值是否与当前值相同，避免不必要的更新
      const currentForms = forms.value || [];
      if (newVal && Array.isArray(newVal) &&
        newVal.length === currentForms.length &&
        JSON.stringify(newVal) === JSON.stringify(currentForms)) {
        console.log('[FormGrid] 表单数据无实际变化，跳过更新');
        return;
      }

      // 添加深度克隆和空值处理
      // 保持响应式引用，避免重新赋值破坏响应式

      // 使用props.modelValue直接进行验证
      const validated = validateForms(newVal);

      // 检查验证后的数据是否与当前数据相同
      if (validated.length === currentForms.length &&
        JSON.stringify(validated) === JSON.stringify(currentForms)) {
        console.log('[FormGrid] 验证后表单数据无实际变化，跳过更新');
        return;
      }

      // 总是更新表单数据，确保新添加的表单能正确显示
      forms.value.splice(0, forms.value.length, ...validated);

      // 更新卡片位置信息
      const newPositions = validated.map(form => ({
        id: form.id,
        ...form.position,
        width: form.size?.width || 200,
        height: form.size?.height || 100
      }));

      positions.value = newPositions;
    }, {
      immediate: true,    // 立即执行
      deep: true         // 深度监听
    });

    // 优化加载状态处理
    nextTick(() => {
      // 当表单数据就绪时立即更新加载状态
      if (props.modelValue && props.modelValue.length > 0) {
        loading.value = false
      } else {
        // 设置安全超时
        setTimeout(() => {
          loading.value = false
        }, 100)
      }
    });

    // 使用 watch 监听 el-main 容器尺寸变化
    const elMainRef = ref(null);
    const getElMain = () => {
      elMainRef.value = document.querySelector('.main-container .el-main');
    };

    getElMain(); // 初始获取

    watch(
      () => (elMainRef.value ? [elMainRef.value.offsetWidth, elMainRef.value.offsetHeight] : null),
      () => {
        console.log('[FormGrid] 检测到 el-main 容器尺寸变化');
        // 触发更新以重新计算 gridContainerStyle
        nextTick(() => {
          // 强制更新样式
        });
      },
      { deep: true }
    );
  } catch (error) {
    handleError(error, '组件挂载失败');
  }
});

// 获取ApiServiceManager实例
const getApiService = () => {
  try {
    // 首先尝试通过getCurrentInstance获取
    const instance = getCurrentInstance()
    if (instance && instance.appContext && instance.appContext.config && instance.appContext.config.globalProperties) {
      const apiService = instance.appContext.config.globalProperties.$apiService
      if (apiService && typeof apiService.updateFile === 'function') {
        console.log('[FormGrid] 通过Vue实例获取到ApiServiceManager');
        return apiService
      }
    }

    // 如果通过实例无法获取，则尝试通过window对象获取
    if (typeof window !== 'undefined' && window.$apiService && typeof window.$apiService.updateFile === 'function') {
      console.log('[FormGrid] 通过window对象获取到ApiServiceManager');
      return window.$apiService
    }

    // 如果都获取不到，输出详细错误信息
    console.warn('[FormGrid] 无法获取有效的ApiServiceManager实例', {
      instanceExists: !!instance,
      hasGlobalProperties: instance ? !!(instance.appContext && instance.appContext.config && instance.appContext.config.globalProperties) : false,
      globalPropertiesKeys: instance && instance.appContext && instance.appContext.config && instance.appContext.config.globalProperties
        ? Object.keys(instance.appContext.config.globalProperties)
        : 'No globalProperties',
      windowExists: typeof window !== 'undefined',
      hasWindowApiService: typeof window !== 'undefined' ? !!window.$apiService : false,
      windowApiServiceType: typeof window !== 'undefined' && window.$apiService ? typeof window.$apiService.updateFile : 'No updateFile method'
    })

    return null
  } catch (error) {
    console.error('[FormGrid] 获取ApiServiceManager实例时发生错误:', error);
    return null
  }
}

// 计算网格容器样式
const gridContainerStyle = computed(() => {
  // 获取el-main元素的实时尺寸作为依赖，确保当页面尺寸变化时重新计算
  const mainElement = document.querySelector('.main-container .el-main');

  if (props.pageSize) {
    const { width, height, unit } = props.pageSize;
    return {
      width: `${width}${unit}`,
      height: `${height}${unit}`,
      position: 'relative',
      overflow: 'auto'
    };
  }

  // 如果没有pageSize，返回一个默认样式以确保容器可见
  return {
    position: 'relative',
    minHeight: '400px',
    width: '100%',
    overflow: 'auto'
  };
});

</script>

<style scoped>
/* =====================================================
   = 1. 基础样式 =
   ====================================================== */
.form-grid-container {
  position: relative;
  width: 100%;
  height: 100%;
  flex: 1;
  /* 占据可用空间 */
  display: flex;
  flex-direction: column;
}

.form-grid {
  position: relative;
  flex: 1;
  /* 占据剩余空间 */
  min-height: 400px;
  /* 确保容器始终可见 */
  padding: var(--spacing-base);
  /* 添加对页面尺寸的支持 */
  overflow: auto;
  /* 允许滚动 */
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
.form-editor-enter-active,
.form-editor-leave-active {
  transition: opacity 0.3s ease;
}

.form-editor-enter-from,
.form-editor-leave-to {
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