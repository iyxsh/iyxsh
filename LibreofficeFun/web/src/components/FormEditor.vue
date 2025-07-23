<template>
  <div class="form-editor">
    <div class="form-editor-header">
      <h3>{{ isEdit ? '编辑表单' : '新建表单' }}</h3>
      <el-button @click="cancel" size="small" type="default" icon="Close">取消</el-button>
    </div>

    <el-form :model="formState" label-position="top" :rules="rules" ref="formRef" class="form-content" @submit.prevent="submitForm">
      <!-- 标题字段 -->
      <el-form-item label="标题" prop="title">
        <el-input v-model="formState.title" placeholder="请输入表单标题" :maxlength="50" show-word-limit />
      </el-form-item>

      <!-- 内容字段 -->
      <el-form-item label="内容" prop="value">
        <el-input 
          v-model="formState.value" 
          type="textarea" 
          placeholder="请输入表单内容" 
          :autosize="{ minRows: 3, maxRows: 6 }" 
          :maxlength="200" 
          show-word-limit
        />
      </el-form-item>

      <!-- 备注字段 -->
      <el-form-item label="备注" prop="remark">
        <el-input 
          v-model="formState.remark" 
          type="textarea" 
          placeholder="请输入备注信息（选填）" 
          :autosize="{ minRows: 2, maxRows: 4 }" 
          :maxlength="100" 
          show-word-limit
        />
      </el-form-item>

      <!-- 样式设置（可折叠） -->
      <el-collapse v-model="activeCollapse" class="style-settings-panel">
        <el-collapse-item title="样式设置" name="style">
          <div class="style-grid">
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
              <el-input-number v-model="formState.style.borderWidth" :min="0" :max="10" :step="1" :precision="0" controls-position="right" size="small" style="width: 120px" />
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
              <el-input-number v-model="formState.style.borderRadius" :min="0" :max="20" :step="1" :precision="0" controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 字体大小 -->
            <el-form-item label="字体大小">
              <el-input-number v-model="formState.style.fontSize" :min="12" :max="32" :step="1" :precision="0" controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 阴影 -->
            <el-form-item label="阴影">
              <el-switch v-model="formState.style.hasShadow" />
            </el-form-item>
          </div>

          <!-- 预览卡片 -->
          <div class="preview-section">
            <h4>预览效果</h4>
            <div class="preview-card" :style="previewStyle">
              <div class="preview-title">{{ formState.title || '表单标题' }}</div>
              <div class="preview-value">{{ formState.value || '表单内容将显示在这里' }}</div>
              <div class="preview-remark" v-if="formState.remark">{{ formState.remark }}</div>
            </div>
          </div>
        </el-collapse-item>

        <!-- 位置和大小设置 -->
        <el-collapse-item title="位置和大小" name="position">
          <div class="position-grid">
            <!-- X坐标 -->
            <el-form-item label="X坐标">
              <el-input-number v-model="formState.position.x" :min="0" :max="1000" :step="10" controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- Y坐标 -->
            <el-form-item label="Y坐标">
              <el-input-number v-model="formState.position.y" :min="0" :max="1000" :step="10" controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 宽度 -->
            <el-form-item label="宽度">
              <el-input-number v-model="formState.size.width" :min="100" :max="1000" :step="10" controls-position="right" size="small" style="width: 120px" />
            </el-form-item>

            <!-- 高度 -->
            <el-form-item label="高度">
              <el-input-number v-model="formState.size.height" :min="50" :max="1000" :step="10" controls-position="right" size="small" style="width: 120px" />
            </el-form-item>
          </div>
        </el-collapse-item>
      </el-collapse>

      <!-- 操作按钮 -->
      <div class="form-actions">
        <el-button @click="cancel">取消</el-button>
        <el-button type="primary" @click="submitForm" :loading="loading">保存</el-button>
      </div>
    </el-form>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Close } from '@element-plus/icons-vue'
import errorLogService from '@/services/errorLogService'

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

// 定义emits
const emit = defineEmits(['save', 'cancel', 'error'])

// 表单引用
const formRef = ref(null)

// 折叠面板的活动项
const activeCollapse = ref(['style'])

// 加载状态
const loading = ref(false)

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

// 表单状态
const formState = reactive({
  id: props.form?.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`,
  title: props.isEdit ? (props.form?.title || '') : '',
  value: props.isEdit ? (props.form?.value || '') : '',
  remark: props.isEdit ? (props.form?.remark || '') : '',
  style: { 
    ...defaultStyle(), 
    ...(props.isEdit && props.form?.style ? props.form.style : {}) 
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
    padding: '12px',
    boxShadow: hasShadow ? '0 2px 12px 0 rgba(0, 0, 0, 0.1)' : 'none'
  }
})

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
const handleError = (error, context = '未知上下文') => {
  console.error(`[FormEditor Error] ${context}:`, error)
  
  // 使用 ElMessage 显示错误
  ElMessage.error({
    message: `操作失败: ${context}`,
    duration: 5000
  })
  
  // 发送错误事件
  emit('error', { error, context })
  
  // 记录错误日志
  errorLogService.addErrorLog(error, `表单编辑器 - ${context}`, 'error')
}

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
  padding: 20px;
  background-color: #f5f7fa;
  border-radius: 8px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 100%;
  max-width: 800px;
  margin: 0 auto;
}

.form-editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding-bottom: 10px;
  border-bottom: 1px solid #dcdfe6;
}

.form-editor-header h3 {
  margin: 0;
  font-size: 18px;
  color: #303133;
}

.form-content {
  margin-top: 20px;
}

.style-settings-panel {
  margin: 20px 0;
  border: 1px solid #ebeef5;
  border-radius: 4px;
}

.style-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 20px;
  margin-bottom: 20px;
}

.position-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 20px;
}

.preview-section {
  margin-top: 20px;
  padding: 15px;
  background-color: #ffffff;
  border-radius: 4px;
  border: 1px dashed #dcdfe6;
}

.preview-section h4 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #606266;
}

.preview-card {
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  padding: 15px;
  margin-top: 10px;
  background-color: #fff;
  transition: all 0.3s ease;
}

.preview-title {
  font-weight: bold;
  margin-bottom: 8px;
}

.preview-value {
  margin-bottom: 8px;
  white-space: pre-wrap;
}

.preview-remark {
  font-size: 0.9em;
  color: #909399;
  white-space: pre-wrap;
}

.form-actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 30px;
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
}
</style>