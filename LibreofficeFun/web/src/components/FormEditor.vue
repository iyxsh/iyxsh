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

      <!-- 样式设置（可折叠） -->
      <el-collapse v-model="activeCollapse" class="style-settings-panel">

        <!-- 位置和大小设置 -->
        <el-collapse-item title="位置和大小" name="position">
          <div class="position-grid">
            <!-- X坐标 -->
            <el-form-item label="X坐标">
              <el-input-number v-model="formState.position.x" :min="0" :max="1000" :step="10" controls-position="right"
                size="small" style="width: 120px" />
            </el-form-item>

            <!-- Y坐标 -->
            <el-form-item label="Y坐标">
              <el-input-number v-model="formState.position.y" :min="0" :max="1000" :step="10" controls-position="right"
                size="small" style="width: 120px" />
            </el-form-item>

            <!-- 宽度 -->
            <el-form-item label="宽度">
              <el-input-number v-model="formState.size.width" :min="100" :max="1000" :step="10"
                controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 高度 -->
            <el-form-item label="高度">
              <el-input-number v-model="formState.size.height" :min="50" :max="1000" :step="10"
                controls-position="right" size="small" style="width: 120px" />
            </el-form-item>
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
            <el-button @click="openGlobalStylePanel" type="primary" size="small">
              <el-icon>
                <Setting />
              </el-icon>
              <span>统一样式设置</span>
            </el-button>
            <el-button @click="closePreview" type="default" size="small">
              <el-icon>
                <Close />
              </el-icon>
              <span>关闭</span>
            </el-button>
          </div>
        </div>
        <div class="preview-mode-body" :style="previewStyle">
          <div class="preview-title preview-element" :style="previewTitleStyle" v-if="formState.showTitle"
            @click="openElementStylePanel('title')">
            {{ formState.title || '表单标题' }}
          </div>
          <div class="preview-value preview-element" :style="previewValueStyle" v-if="formState.showValue"
            @click="openElementStylePanel('value')">
            {{ formState.value || '表单内容将显示在这里' }}
          </div>
          <div class="preview-remark preview-element" :style="previewRemarkStyle"
            v-if="formState.remark && formState.showRemark" @click="openElementStylePanel('remark')">
            {{ formState.remark }}
          </div>
          <div class="preview-media preview-element" v-if="formState.media && formState.showMedia"
            :style="previewMediaStyle" @click="openElementStylePanel('media')">
            <img v-if="formState.mediaType === 'image'" :src="formState.media" alt="媒体"
              style="max-width: 100%; border-radius: 4px;" />
            <video v-else :src="formState.media" controls style="max-width: 100%; border-radius: 4px;"></video>
          </div>
        </div>
      </div>

      <!-- 元素样式设置面板 -->
      <div v-if="currentElementStyle" class="element-style-panel-overlay" @click="closeElementStylePanel">
        <div class="element-style-panel-content" @click.stop>
          <div class="element-style-panel-header">
            <h4>{{ getElementDisplayName(currentElementStyle.type) }}样式设置</h4>
            <el-button @click="closeElementStylePanel" type="default" size="small">
              <el-icon>
                <Close />
              </el-icon>
              <span>关闭</span>
            </el-button>
          </div>
          <ElementStylePanel :style-config="currentElementStyle.style"
            @update:style="updateElementStyle(currentElementStyle.type, $event)" />
          <div class="element-style-panel-actions">
            <el-button @click="closeElementStylePanel">确定</el-button>
          </div>
        </div>
      </div>

      <!-- 统一样式设置面板 -->
      <div v-if="isGlobalStylePanelVisible" class="global-style-panel-overlay" @click="closeGlobalStylePanel">
        <div class="global-style-panel-content" @click.stop>
          <div class="global-style-panel-header">
            <h4>统一样式设置</h4>
            <el-button @click="closeGlobalStylePanel" type="default" size="small">
              <el-icon>
                <Close />
              </el-icon>
              <span>关闭</span>
            </el-button>
          </div>

          <div class="global-style-panel-body">
            <!-- 背景颜色 -->
            <el-form-item label="背景颜色">
              <el-color-picker v-model="formState.style.backgroundColor" show-alpha />
            </el-form-item>

            <!-- 文字颜色 -->
            <el-form-item label="文字颜色">
              <el-color-picker v-model="formState.style.color" show-alpha />
            </el-form-item>

            <!-- 边框设置 -->
            <el-form-item label="边框">
              <el-input-number v-model="formState.style.borderWidth" :min="0" :max="10" :step="1" :precision="0"
                controls-position="right" size="small" style="width: 120px" />
              <el-select v-model="formState.style.borderStyle" size="small" style="margin-left: 8px; width: 100px">
                <el-option label="实线" value="solid"></el-option>
                <el-option label="虚线" value="dashed"></el-option>
                <el-option label="点线" value="dotted"></el-option>
                <el-option label="无" value="none"></el-option>
              </el-select>
              <el-color-picker v-model="formState.style.borderColor" size="small" style="margin-left: 8px" />
            </el-form-item>

            <!-- 圆角 -->
            <el-form-item label="圆角">
              <el-input-number v-model="formState.style.borderRadius" :min="0" :max="20" :step="1" :precision="0"
                controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 字体大小 -->
            <el-form-item label="字体大小">
              <el-input-number v-model="formState.style.fontSize" :min="12" :max="32" :step="1" :precision="0"
                controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 阴影 -->
            <el-form-item label="阴影">
              <el-switch v-model="formState.style.hasShadow" />
            </el-form-item>
          </div>

          <div class="global-style-panel-actions">
            <el-button @click="applyGlobalStyles">应用</el-button>
            <el-button @click="closeGlobalStylePanel">确定</el-button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Close, Setting } from '@element-plus/icons-vue'
import ElementStylePanel from './ElementStylePanel.vue'
import errorLogService from '../services/errorLogService'
import { t as i18nTrans } from '../utils/i18n'

// 定义组件属性
const props = defineProps({
  form: {
    type: Object,
    default: null
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
const activeCollapse = ref(['visibility', 'style', 'position'])

// 预览模式状态
const isPreviewMode = ref(false)

// 当前元素样式（用于元素样式设置面板）
const currentElementStyle = ref(null)

// 统一样式面板状态
const isGlobalStylePanelVisible = ref(false)

// 默认样式
const defaultStyle = () => ({
  backgroundColor: '#ffffff',
  color: '#333333',
  borderWidth: 1,
  borderStyle: 'solid',
  borderColor: '#e4e7ed',
  borderRadius: 4,
  fontSize: 14,
  hasShadow: false
})

// 默认元素样式
const defaultElementStyle = () => ({
  enabled: false,
  color: '#333333',
  fontSize: 14,
  fontWeight: 'normal'
})

// 确保元素样式完整性
const ensureElementStyleIntegrity = (style) => {
  const result = { ...defaultElementStyle(), ...style }

  // 验证必要字段
  const requiredFields = Object.keys(defaultElementStyle())
  requiredFields.forEach(key => {
    if (!(key in result)) {
      console.warn(`Missing key "${key}" in element style, using default value`)
      result[key] = defaultElementStyle()[key]
    }
  })

  return result
}

// 打开统一样式设置面板
const openGlobalStylePanel = () => {
  isGlobalStylePanelVisible.value = true
}

// 关闭统一样式设置面板
const closeGlobalStylePanel = () => {
  isGlobalStylePanelVisible.value = false
}

// 应用统一样式到所有启用的元素样式
const applyGlobalStyles = () => {
  // 获取当前的通用样式
  const { color, fontSize } = formState.style

  // 遍历所有元素样式
  Object.keys(formState.elementStyles).forEach(elementType => {
    const elementStyle = formState.elementStyles[elementType]

    // 如果该元素样式已启用，则更新其样式
    if (elementStyle.enabled) {
      formState.elementStyles[elementType] = {
        ...elementStyle,
        color,
        fontSize
      }
    }
  })

  ElMessage.success('统一样式已应用到所有启用的元素')
}

// 表单状态
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
    {
      validator: (rule, value, callback) => {
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
      }, trigger: 'blur'
    }
  ]
}

// 预览样式计算属性
const previewStyle = computed(() => {
  const { backgroundColor, color, borderWidth, borderStyle, borderColor, borderRadius, fontSize, hasShadow } = formState.style
  return {
    backgroundColor,
    color,
    borderWidth: `${borderWidth}px`,
    borderStyle,
    borderColor,
    borderRadius: `${borderRadius}px`,
    fontSize: `${fontSize}px`,
    padding: '20px',
    boxShadow: hasShadow ? '0 2px 12px 0 rgba(0, 0, 0, 0.1)' : 'none',
    maxWidth: '600px',
    margin: '0 auto'
  }
})

// 预览标题样式
const previewTitleStyle = computed(() => {
  const titleStyle = formState.elementStyles.title
  if (!titleStyle.enabled) {
    // 回退到通用样式设置
    return {
      color: formState.style.color,
      fontSize: `${formState.style.fontSize}px`,
      fontWeight: 'normal', // 默认字体粗细
      marginBottom: '10px'
    }
  }

  return {
    color: titleStyle.color,
    fontSize: `${titleStyle.fontSize}px`,
    fontWeight: titleStyle.fontWeight,
    marginBottom: '10px'
  }
})

// 预览内容样式
const previewValueStyle = computed(() => {
  const valueStyle = formState.elementStyles.value
  if (!valueStyle.enabled) {
    // 回退到通用样式设置
    return {
      color: formState.style.color,
      fontSize: `${formState.style.fontSize}px`,
      fontWeight: 'normal', // 默认字体粗细
      marginBottom: '10px',
      whiteSpace: 'pre-wrap'
    }
  }

  return {
    color: valueStyle.color,
    fontSize: `${valueStyle.fontSize}px`,
    fontWeight: valueStyle.fontWeight,
    marginBottom: '10px',
    whiteSpace: 'pre-wrap'
  }
})

// 预览备注样式
const previewRemarkStyle = computed(() => {
  const remarkStyle = formState.elementStyles.remark
  if (!remarkStyle.enabled) {
    // 回退到通用样式设置
    return {
      color: formState.style.color,
      fontSize: `${formState.style.fontSize - 2}px`, // 备注字体稍小
      fontWeight: 'normal', // 默认字体粗细
      whiteSpace: 'pre-wrap'
    }
  }

  return {
    color: remarkStyle.color,
    fontSize: `${remarkStyle.fontSize}px`,
    fontWeight: remarkStyle.fontWeight,
    whiteSpace: 'pre-wrap'
  }
})

// 预览媒体样式
const previewMediaStyle = computed(() => {
  const mediaStyle = formState.elementStyles.media
  if (!mediaStyle.enabled) {
    // 回退到通用样式设置
    return {
      color: formState.style.color,
      fontSize: `${formState.style.fontSize}px`,
      fontWeight: 'normal' // 默认字体粗细
    }
  }

  return {
    color: mediaStyle.color,
    fontSize: `${mediaStyle.fontSize}px`,
    fontWeight: mediaStyle.fontWeight
  }
})

// 切换预览模式
const togglePreview = () => {
  isPreviewMode.value = !isPreviewMode.value
}

// 关闭预览模式
const closePreview = () => {
  isPreviewMode.value = false
  currentElementStyle.value = null
  closeGlobalStylePanel()
}

// 打开元素样式设置面板
const openElementStylePanel = (elementType) => {
  currentElementStyle.value = {
    type: elementType,
    style: { ...formState.elementStyles[elementType] } // 创建副本避免直接引用
  }
}

// 关闭元素样式设置面板
const closeElementStylePanel = () => {
  currentElementStyle.value = null
}

// 获取元素的显示名称
const getElementDisplayName = (elementType) => {
  switch (elementType) {
    case 'title':
      return '标题'
    case 'value':
      return '内容'
    case 'remark':
      return '备注'
    case 'media':
      return '媒体'
    default:
      return elementType
  }
}

// 获取元素的样式配置
const getElementStyleConfig = (elementType) => {
  return formState.elementStyles[elementType]
}

// 更新元素样式
const updateElementStyle = (elementType, styleConfig) => {
  // 确保元素样式已启用
  const updatedStyle = {
    ...formState.elementStyles[elementType],
    ...styleConfig,
    enabled: true // 确保启用该元素的特定样式
  };

  // 更新表单状态中的元素样式
  formState.elementStyles[elementType] = updatedStyle;
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
    if (formData.elementStyles) {
      // 验证每个元素样式是否包含必要字段
      const defaultKeys = Object.keys(defaultElementStyle())
      Object.values(formData.elementStyles).forEach(style => {
        if (typeof style === 'object') {
          defaultKeys.forEach(key => {
            if (!(key in style)) {
              console.warn(`Missing key "${key}" in element style, using default value`)
              style[key] = defaultElementStyle()[key]
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
    loading.value = false
  }
}

// 错误处理
const handleError = (error, context) => {
  try {
    console.error(`[FormEditor] ${context} 发生错误:`, error);
    
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

  .preview-mode-content,
  .element-style-panel-content,
  .global-style-panel-content {
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