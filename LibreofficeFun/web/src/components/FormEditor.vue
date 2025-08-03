<template>
  <div class="form-editor">
    <div class="form-editor-header">
      <h3>{{ isEdit ? '编辑表单' : '新建表单' }}</h3>
      <el-button @click="cancel" size="small" type="default" icon="Close">取消</el-button>
    </div>

    <el-form :model="formState" label-position="top" :rules="rules" ref="formRef" class="form-content"
      @submit.prevent="submitForm">
      <!-- 标题字段 -->
      <el-form-item label="标题" prop="title">
        <div class="element-container">
          <el-input v-model="formState.title" placeholder="请输入表单标题" :maxlength="50" show-word-limit />
        </div>
      </el-form-item>

      <!-- 内容字段 -->
      <el-form-item label="内容" prop="value">
        <div class="element-container">
          <el-input v-model="formState.value" type="textarea" placeholder="请输入表单内容"
            :autosize="{ minRows: 3, maxRows: 6 }" :maxlength="200" show-word-limit />
        </div>
      </el-form-item>

      <!-- 备注字段 -->
      <el-form-item label="备注" prop="remark">
        <div class="element-container">
          <el-input v-model="formState.remark" type="textarea" placeholder="请输入备注信息（选填）"
            :autosize="{ minRows: 2, maxRows: 4 }" :maxlength="100" show-word-limit />
        </div>
      </el-form-item>

      <!-- 媒体字段 -->
      <el-form-item label="媒体链接" prop="media">
        <div class="element-container">
          <el-input v-model="formState.media" placeholder="请输入图片或视频链接" :maxlength="500" />
          <div class="media-type-select">
            <el-select v-model="formState.mediaType" size="small">
              <el-option label="图片" value="image" />
              <el-option label="视频" value="video" />
            </el-select>
          </div>
        </div>
      </el-form-item>

      <!-- 可见性设置 -->
      <el-collapse v-model="activeCollapse" class="visibility-settings-panel">
        <el-collapse-item title="可见性设置" name="visibility">
          <div class="visibility-grid">
            <el-checkbox v-model="formState.showTitle">显示标题</el-checkbox>
            <el-checkbox v-model="formState.showValue">显示内容</el-checkbox>
            <el-checkbox v-model="formState.showRemark">显示备注</el-checkbox>
            <el-checkbox v-model="formState.showMedia">显示媒体</el-checkbox>
          </div>
        </el-collapse-item>
      </el-collapse>

      <!-- 样式管理器 -->
      <el-collapse v-model="activeCollapse" class="style-settings-panel">
        <el-collapse-item title="样式设置" name="style">
          <StyleManager v-model="formState.style" />
        </el-collapse-item>
      </el-collapse>

      <!-- 元素样式管理器 -->
      <el-collapse v-model="activeCollapse" class="element-style-settings-panel">
        <el-collapse-item title="元素样式设置" name="elementStyle">
          <div class="element-style-section">
            <h4>标题样式</h4>
            <el-switch 
              v-model="formState.elementStyles.title.enabled" 
              active-text="启用"
              inactive-text="禁用" />
            <StyleManager v-model="formState.elementStyles.title" v-if="formState.elementStyles.title.enabled" />
          </div>
          
          <div class="element-style-section">
            <h4>内容样式</h4>
            <el-switch 
              v-model="formState.elementStyles.value.enabled" 
              active-text="启用"
              inactive-text="禁用" />
            <StyleManager v-model="formState.elementStyles.value" v-if="formState.elementStyles.value.enabled" />
          </div>
          
          <div class="element-style-section">
            <h4>备注样式</h4>
            <el-switch 
              v-model="formState.elementStyles.remark.enabled" 
              active-text="启用"
              inactive-text="禁用" />
            <StyleManager v-model="formState.elementStyles.remark" v-if="formState.elementStyles.remark.enabled" />
          </div>
          
          <div class="element-style-section">
            <h4>媒体样式</h4>
            <el-switch 
              v-model="formState.elementStyles.media.enabled" 
              active-text="启用"
              inactive-text="禁用" />
            <StyleManager v-model="formState.elementStyles.media" v-if="formState.elementStyles.media.enabled" />
          </div>
        </el-collapse-item>
      </el-collapse>

      <!-- 操作按钮 -->
      <div class="form-actions">
        <el-button @click="cancel">取消</el-button>
        <el-button @click="togglePreview">{{ isPreviewMode ? '关闭预览' : '预览' }}</el-button>
        <el-button type="primary" @click="submitForm" :loading="loading">保存</el-button>
      </div>
    </el-form>

    <!-- 独立预览模式 -->
    <div v-if="isPreviewMode" class="preview-mode-overlay" @click="closePreview">
      <div class="preview-mode-content" @click.stop>
        <div class="preview-mode-header">
          <h3>表单预览</h3>
          <div class="preview-mode-actions">
            <el-button @click="closePreview" type="default" size="small">
              <el-icon>
                <Close />
              </el-icon>
              <span>关闭</span>
            </el-button>
          </div>
        </div>
        <div class="preview-mode-body">
          <SimpleCardConverter 
            :forms="[formState]"
            :cardGroups="previewCardGroups" 
            :cardGroupStyles="previewCardGroupStyles"
            :cardRowStyles="previewCardRowStyles" 
            :cardStyles="previewCardStyles" 
            :globalTextStyles="previewGlobalTextStyles"
            @edit-card="handleEditCard"
            @edit-row-style="handleEditRowStyle" />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Close, Setting } from '@element-plus/icons-vue'
import SimpleCardConverter from './SimpleCardConverter.vue'
import StyleManager from './StyleManager.vue'
import errorLogService from '../services/errorLogService'
import { t as i18nTrans } from '../utils/i18n'

// 定义props
const props = defineProps({
  form: {
    type: Object,
    default: () => ({})
  },
  isEdit: {
    type: Boolean,
    default: false
  }
})

// 定义事件发射器
const emit = defineEmits(['save', 'cancel', 'error'])

// 表单引用
const formRef = ref(null)

// 加载状态
const loading = ref(false)

// 激活的折叠面板
const activeCollapse = ref(['visibility', 'style'])

// 预览模式状态
const isPreviewMode = ref(false)

// 预览相关的SimpleCardConverter数据
const previewCardGroups = ref([])
const previewCardGroupStyles = ref({})
const previewCardRowStyles = ref({})
const previewCardStyles = ref({})
const previewGlobalTextStyles = ref({})

// 默认样式
const defaultStyle = () => ({
  backgroundColor: '#ffffff',
  color: '#333333',
  borderWidth: 1,
  borderStyle: 'solid',
  borderColor: '#e4e7ed',
  borderRadius: 4,
  fontSize: 14,
  padding: 12,
  hasShadow: true
})

// 默认的元素样式设置
const defaultElementStyle = () => ({
  enabled: false,
  color: '#333333',
  fontSize: 14,
  fontWeight: 'normal'
})

// 确保元素样式数据完整性的函数
const ensureElementStyleIntegrity = (style) => {
  const defaultStyle = defaultElementStyle()
  const result = { ...style }
  
  // 确保所有必要字段都存在
  Object.keys(defaultStyle).forEach(key => {
    if (!(key in result)) {
      console.warn(`Missing key "${key}" in element style, using default value`)
      result[key] = defaultStyle[key]
    }
  })
  
  return result
}


// 打开统一样式设置面板

// 关闭统一样式设置面板

// 应用统一样式到所有启用的元素样式

// 表单状态
/** @type {Form} */
const formState = reactive({
  id: props.form?.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`,
  title: props.isEdit ? (props.form?.title || '') : '',
  value: props.isEdit ? (props.form?.value || '') : '',
  remark: props.isEdit ? (props.form?.remark || '') : '',
  media: props.isEdit ? (props.form?.media || '') : '',
  mediaType: props.isEdit ? (props.form?.mediaType || 'image') : 'image',
  showTitle: props.isEdit ? (props.form?.showTitle !== false) : true,
  showValue: props.isEdit ? (props.form?.showValue !== false) : true,
  showRemark: props.isEdit ? (props.form?.showRemark !== false) : true,
  showMedia: props.isEdit ? (props.form?.showMedia !== false) : true,
  style: { 
    ...defaultStyle(), 
    ...(props.isEdit && props.form?.style ? props.form.style : {}) 
  },
  // 添加元素特定样式设置
  elementStyles: {
    title: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.title ? props.form.elementStyles.title : {})
    }),
    value: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.value ? props.form.elementStyles.value : {})
    }),
    remark: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.remark ? props.form.elementStyles.remark : {})
    }),
    media: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.media ? props.form.elementStyles.media : {})
    })
  },
  position: { 
    x: props.isEdit ? (props.form?.position?.x || 20) : 20, 
    y: props.isEdit ? (props.form?.position?.y || 20) : 20 
  },
  size: { 
    width: props.isEdit ? (props.form?.size?.width || 200) : 200, 
    height: props.isEdit ? (props.form?.size?.height || 100) : 100 
  }
})

// 验证规则
const rules = {
  title: [
    { required: true, message: '请输入表单标题', trigger: 'blur' },
    { min: 1, max: 50, message: '长度在1到50个字符之间', trigger: 'blur' }
  ],
  value: [
    { required: true, message: '请输入表单内容', trigger: 'blur' },
    { min: 1, max: 200, message: '长度在1到200个字符之间', trigger: 'blur' }
  ],
  remark: [
    { max: 100, message: '长度不能超过100个字符', trigger: 'blur' }
  ],
  media: [
    { validator: (rule, value, callback) => {
        if (formState.showMedia && value) {
          // 简单验证URL格式
          const urlPattern = /^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$/;
          if (!urlPattern.test(value)) {
            callback(new Error('请输入有效的URL'));
          } else {
            callback();
          }
        } else {
          callback();
        }
      }, trigger: 'blur' }
  ]
}


// 切换预览模式
const togglePreview = () => {
  isPreviewMode.value = !isPreviewMode.value
}

// 关闭预览模式
const closePreview = () => {
  isPreviewMode.value = false
}

// 取消编辑
const cancel = () => {
  try {
    // 如果表单已修改，提示用户确认
    if (hasFormChanged()) {
      ElMessageBox.confirm(
        '表单已修改，确定要取消吗？',
        '提示',
        {
          confirmButtonText: '确定',
          cancelButtonText: '返回编辑',
          type: 'warning'
        }
      ).then(() => {
        emit('cancel')
      }).catch(() => {
        // 用户选择继续编辑，不做操作
      })
    } else {
      // 表单未修改，直接取消
      emit('cancel')
    }
  } catch (error) {
    handleError(error, '取消编辑失败')
  }
}

// 判断表单是否有变化
const hasFormChanged = () => {
  // 简单判断，只比较主要字段
  if (!props.form) return formState.title || formState.value || formState.remark
  return (
    formState.title !== props.form.title ||
    formState.value !== props.form.value ||
    formState.remark !== props.form.remark
  )
}

// 提交表单
const submitForm = async () => {
  if (!formRef.value) return

  try {
    loading.value = true

    // 验证表单
    await formRef.value.validate()

    // 复制表单数据，避免引用问题
    const formData = JSON.parse(JSON.stringify(formState))

    // 确保elementStyles数据结构完整
    if (formData.elementStyles && typeof formData.elementStyles === 'object') {
      // 验证每个元素样式是否包含必要字段
      const defaultKeys = Object.keys(defaultElementStyle())
      Object.entries(formData.elementStyles).forEach(([key, style]) => {
        // 确保style存在且为对象
        if (style && typeof style === 'object') {
          defaultKeys.forEach(defaultKey => {
            if (!(defaultKey in style)) {
              console.warn(`Missing key "${defaultKey}" in element style "${key}", using default value`)
              style[defaultKey] = defaultElementStyle()[defaultKey]
            }
          })
        }
      })
    }

    // 添加时间戳
    formData.updatedAt = Date.now()

    // 发送保存事件
    emit('save', formData)

    // 显示成功消息
    ElMessage.success('表单保存成功')
  } catch (error) {
    handleError(error, '表单验证失败')
  } finally {
    // 确保在所有情况下loading状态都会被重置
    loading.value = false
  }
}

// 错误处理
const handleError = (error, context) => {
  try {
    console.error(`[FormEditor] ${context} 发生错误:`, error);
    
    // 确保loading状态被重置
    loading.value = false;
    
    // 如果是表单验证错误，不发送错误事件
    if (context === '表单验证失败') {
      console.log('[FormEditor] 表单验证失败，不发送错误事件');
      return;
    }
    
    // 发送错误事件
    emit('error', { error, context });
    
    // 显示用户友好的错误消息
    ElMessage.error(context);
  } catch (e) {
    console.error('[FormEditor] 处理错误时发生异常:', e);
    // 即使处理错误时发生异常，也要确保loading状态被重置
    loading.value = false;
  }
};

// 组件挂载后的操作
onMounted(async () => {
  try {
    // 确保DOM已渲染
    await nextTick()

    // 如果是编辑模式，自动聚焦到标题输入框
    if (props.isEdit && formRef.value) {
      // 通过 $el 访问真实的DOM元素
      const formElement = formRef.value.$el || formRef.value
      const titleInput = formElement.querySelector('input[type="text"]')
      if (titleInput) titleInput.focus()
    }
  } catch (error) {
    handleError(error, '组件挂载失败')
  }
})
</script>

<style scoped>
.form-editor {
  width: 100%;
  padding: 20px;
  box-sizing: border-box;
}

.form-editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.form-content {
  width: 100%;
}

.element-container {
  display: flex;
  flex-direction: column;
}

.element-style-toggle {
  margin-top: 10px;
}

.element-style-controls {
  margin-top: 10px;
}

.media-type-select {
  margin-top: 10px;
}

.visibility-settings-panel,
.style-settings-panel {
  margin-top: 20px;
}

.visibility-grid,
.style-grid,
.position-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px;
}

.form-actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 30px;
}

.preview-mode-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 3000;
}

.preview-mode-content {
  background: #ffffff;
  padding: 20px;
  border-radius: 4px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 80%;
  max-width: 600px;
  position: relative;
  max-height: 80vh;
  overflow-y: auto;
}

.preview-mode-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.preview-mode-actions {
  display: flex;
  gap: 10px;
}

.preview-mode-body {
  width: 100%;
  box-sizing: border-box;
}

.preview-element {
  cursor: pointer;
}

.element-style-panel-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 3001;
}

.element-style-panel-content {
  background: #ffffff;
  padding: 20px;
  border-radius: 4px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 80%;
  max-width: 600px;
  position: relative;
}

.element-style-panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.global-style-panel-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 3002;
}

.global-style-panel-content {
  background: #ffffff;
  padding: 20px;
  border-radius: 4px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 80%;
  max-width: 600px;
  position: relative;
}

.global-style-panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.global-style-panel-body {
  max-height: 60vh;
  overflow-y: auto;
}

.global-style-panel-actions {
  margin-top: 20px;
  padding-top: 15px;
  border-top: 1px solid #eee;
  text-align: right;
}

.element-style-panel-actions {
  margin-top: 20px;
  padding-top: 15px;
  border-top: 1px solid #eee;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .form-editor {
    padding: 15px;
  }

  .style-grid,
  .position-grid {
    grid-template-columns: 1fr;
    gap: 15px;
  }

  .preview-card {
    padding: 10px;
  }

  .element-styles-section {
    padding: 10px;
  }

  .element-style-item {
    padding: 8px;
  }

  .preview-mode-content {
    width: 95%;
    padding: 15px;
  }

  .preview-mode-actions {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }
}
</style>