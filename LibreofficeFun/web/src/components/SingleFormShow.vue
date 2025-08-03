<template>
  <div 
    class="single-form-card-wrapper"
    :style="cardStyle"
    @mousedown="handleMouseDown"
    @dblclick="handleDblClick"
  >
    <el-card 
      :class="['single-form-card', { 'editable': editable, 'no-style': !cardStyleOn }]">
      <div class="form-content">
        <!-- 标题 -->
        <div 
          v-if="shouldShowTitle" 
          class="form-title"
          :style="titleStyle"
        >
          {{ form.title }}
        </div>

        <!-- 内容 -->
        <div 
          v-if="shouldShowValue" 
          class="form-value"
          :style="valueStyle"
        >
          {{ form.value }}
        </div>

        <!-- 备注 -->
        <div 
          v-if="shouldShowRemark" 
          class="form-remark"
          :style="remarkStyle"
        >
          {{ form.remark }}
        </div>

        <!-- 媒体内容（如果有的话） -->
        <div 
          v-if="shouldShowMedia" 
          class="form-media"
          :style="mediaStyle"
        >
          <img 
            v-if="form.mediaType === 'image'" 
            :src="form.media" 
            :alt="form.title || 'Form media'"
            class="media-image"
          >
          <video 
            v-else-if="form.mediaType === 'video'" 
            :src="form.media" 
            controls
            class="media-video"
          ></video>
        </div>
      </div>

      <!-- 编辑模式下的操作菜单 -->
      <div v-if="editable" class="form-actions">
        <el-dropdown @command="handleCommand">
          <el-button class="action-button" size="small">
            <el-icon><MoreFilled /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="edit">
                <el-icon><Edit /></el-icon> 编辑
              </el-dropdown-item>
              <el-dropdown-item command="delete">
                <el-icon><Delete /></el-icon> 删除
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
      
      <!-- 调整大小手柄 -->
      <div 
        v-if="editable" 
        class="resize-handle" 
        @mousedown="startResize"
      ></div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'

// 修改后的计算属性
const cardBodyStyle = computed(() => {
  
  return {};
})
import { MoreFilled, Edit, Delete } from '@element-plus/icons-vue'
// 定义props并添加类型注释
const props = defineProps({
  /** @type {Form} */
  form: {
    type: Object,
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
  position: {
    type: Object,
    default: () => ({ x: 20, y: 20 })
  },
  size: {
    type: Object,
    default: () => ({ width: 200, height: 100 })
  }
})

// 定义emits
const emit = defineEmits(['edit', 'delete', 'update-position', 'update-size', 'dblclick', 'mousedown'])

// 响应式数据
const isResizing = ref(false)
const isDragging = ref(false)
const startX = ref(0)
const startY = ref(0)
const startWidth = ref(0)
const startHeight = ref(0)
const startLeft = ref(0)
const startTop = ref(0)
const formRef = ref(null)

// 计算卡片样式
const cardStyle = computed(() => {
  const baseStyle = {
    position: 'absolute',
    left: `${props.position.x}px`,
    top: `${props.position.y}px`,
    width: `${props.size.width}px`,
    height: `${props.size.height}px`,
    zIndex: props.form?.zIndex || 1
  }
  
  // 如果启用卡片样式，则应用表单样式
  if (props.cardStyleOn && props.form?.style) {
    // 直接使用表单的style属性
    const style = props.form.style;
    
    // 构建body样式对象
    const bodyStyle = {...baseStyle};
    
    // 处理各种样式属性
    if (style.padding !== undefined) {
      bodyStyle.padding = typeof style.padding === 'number' ? `${style.padding}px` : style.padding;
    }
    
    if (style.backgroundColor) {
      bodyStyle.backgroundColor = style.backgroundColor;
    }
    
    if (style.color) {
      bodyStyle.color = style.color;
    }
    
    if (style.fontSize !== undefined) {
      bodyStyle.fontSize = typeof style.fontSize === 'number' ? `${style.fontSize}px` : style.fontSize;
    }
    
    if (style.borderWidth !== undefined) {
      bodyStyle.borderWidth = typeof style.borderWidth === 'number' ? `${style.borderWidth}px` : style.borderWidth;
    }
    
    if (style.borderStyle) {
      bodyStyle.borderStyle = style.borderStyle;
    }
    
    if (style.borderColor) {
      bodyStyle.borderColor = style.borderColor;
    }
    
    if (style.borderRadius !== undefined) {
      bodyStyle.borderRadius = typeof style.borderRadius === 'number' ? `${style.borderRadius}px` : style.borderRadius;
    }
    
    if (style.hasShadow) {
      bodyStyle.boxShadow = '0 2px 12px 0 rgba(0, 0, 0, 0.1)';
    } else if (style.hasShadow === false) {
      bodyStyle.boxShadow = 'none';
    }
    
    return bodyStyle;
  }
  return baseStyle
})

// 元素样式计算 - 使用表单的elementStyles并通过StyleManager处理
const titleStyle = computed(() => {
  if (!props.form?.elementStyles?.title?.enabled) return {}
  const style = props.form.elementStyles.title
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

const valueStyle = computed(() => {
  if (!props.form?.elementStyles?.value?.enabled) return {}
  const style = props.form.elementStyles.value
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

const remarkStyle = computed(() => {
  if (!props.form?.elementStyles?.remark?.enabled) return {}
  const style = props.form.elementStyles.remark
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

const mediaStyle = computed(() => {
  if (!props.form?.elementStyles?.media?.enabled) return {}
  const style = props.form.elementStyles.media
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

// 是否显示标题
const shouldShowTitle = computed(() => {
  return props.form?.showTitle !== false && props.form?.title
})

// 是否显示内容
const shouldShowValue = computed(() => {
  return props.form?.showValue !== false && props.form?.value
})

// 是否显示备注
const shouldShowRemark = computed(() => {
  return props.form?.showRemark !== false && props.form?.remark
})

// 是否显示媒体
const shouldShowMedia = computed(() => {
  return props.form?.showMedia !== false && props.form?.media
})

// 处理命令
const handleCommand = (command) => {
  switch (command) {
    case 'edit':
      editForm()
      break
    case 'delete':
      deleteForm()
      break
  }
}

// 处理双击事件
const handleDblClick = (event) => {
  event.stopPropagation()
  emit('dblclick', props.form)
}

// 处理鼠标按下事件（拖拽开始）
const handleMouseDown = (event) => {
  event.stopPropagation()
  
  // 如果点击的是调整大小的手柄，则不处理拖拽
  if (event.target.classList.contains('resize-handle')) {
    return
  }
  
  isDragging.value = true
  startX.value = event.clientX
  startY.value = event.clientY
  startLeft.value = props.position.x
  startTop.value = props.position.y
  
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', stopDrag)
  
  emit('mousedown', props.form)
}

// 处理拖拽过程
const handleDrag = (event) => {
  if (!isDragging.value) return
  
  const deltaX = event.clientX - startX.value
  const deltaY = event.clientY - startY.value
  
  const newX = startLeft.value + deltaX
  const newY = startTop.value + deltaY
  
  emit('update-position', props.form.id, newX, newY)
}

// 停止拖拽
const stopDrag = () => {
  isDragging.value = false
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
}

// 开始调整大小
const startResize = (event) => {
  event.stopPropagation()
  isResizing.value = true
  startX.value = event.clientX
  startY.value = event.clientY
  startWidth.value = props.size.width
  startHeight.value = props.size.height
  startLeft.value = props.position.x
  startTop.value = props.position.y
  
  document.addEventListener('mousemove', handleResize)
  document.addEventListener('mouseup', stopResize)
}

// 处理调整大小过程
const handleResize = (event) => {
  if (!isResizing.value) return
  
  const deltaX = event.clientX - startX.value
  const deltaY = event.clientY - startY.value
  
  const newWidth = Math.max(100, startWidth.value + deltaX)
  const newHeight = Math.max(50, startHeight.value + deltaY)
  const newX = startLeft.value
  const newY = startTop.value
  
  emit('update-size', props.form.id, newWidth, newHeight)
  emit('update-position', props.form.id, newX, newY)
}

// 停止调整大小
const stopResize = () => {
  isResizing.value = false
  document.removeEventListener('mousemove', handleResize)
  document.removeEventListener('mouseup', stopResize)
}

// 编辑表单
const editForm = () => {
  emit('edit', props.form)
}

// 删除表单
const deleteForm = () => {
  emit('delete', props.form.id)
}

// 组件卸载前清理事件监听器
onBeforeUnmount(() => {
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
  document.removeEventListener('mousemove', handleResize)
  document.removeEventListener('mouseup', stopResize)
})
</script>

<style scoped>
.single-form-card-wrapper {
  cursor: move;
  transition: box-shadow 0.3s ease;
}

.single-form-card-wrapper:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.single-form-card {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.single-form-card.editable {
  cursor: move;
}

.single-form-card :deep(.el-card__body) {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 12px;
  overflow: hidden;
}

.form-title,
.form-value,
.form-remark {
  margin-bottom: 8px;
  word-wrap: break-word;
  white-space: pre-wrap;
}

.form-title:last-child,
.form-value:last-child,
.form-remark:last-child {
  margin-bottom: 0;
}

.form-media {
  margin-top: 8px;
}

.media-image,
.media-video {
  max-width: 100%;
  max-height: 200px;
  object-fit: contain;
  border-radius: 4px;
}

.form-actions {
  position: absolute;
  top: 8px;
  right: 8px;
  z-index: 10;
}

.action-button {
  background: rgba(255, 255, 255, 0.9);
  border: 1px solid #dcdfe6;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.resize-handle {
  position: absolute;
  bottom: 0;
  right: 0;
  width: 12px;
  height: 12px;
  background-color: #409eff;
  cursor: se-resize;
  z-index: 10;
  opacity: 0;
  transition: opacity 0.3s ease;
}

.single-form-card-wrapper:hover .resize-handle {
  opacity: 1;
}

.no-style {
  background: transparent !important;
  border: none !important;
  box-shadow: none !important;
}

.no-style :deep(.el-card__body) {
  padding: 0 !important;
  background: transparent !important;
}
</style>