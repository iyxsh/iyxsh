<template>
  <div 
    class="single-form-card-wrapper"
    :style="cardStyle"
    @mousedown="handleMouseDown"
    @dblclick="handleDoubleClick"
  >
    <el-card 
      :class="['single-form-card', { 'editable': editable, 'no-style': !cardStyleOn }]"
      :body-style="cardStyleOn ? getDefaultCardStyles() : {}"
    >
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
import { ref, computed } from 'vue'
import { ElCard, ElDropdown, ElDropdownMenu, ElDropdownItem, ElButton, ElIcon, ElMessage } from 'element-plus'
import { MoreFilled, Edit, Delete } from '@element-plus/icons-vue'
import { getDefaultCardStyles } from '@/services/cardStyleService'

// 定义props
const props = defineProps({
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
const emit = defineEmits([
  'edit', 
  'delete', 
  'dblclick',
  'mousedown',
  'update-position',
  'update-size'
])

// 计算是否应该显示各个部分
const shouldShowTitle = computed(() => {
  return props.form.showTitle !== false && props.form.title
})

const shouldShowValue = computed(() => {
  return props.form.showValue !== false && props.form.value
})

const shouldShowRemark = computed(() => {
  return props.form.showRemark !== false && props.form.remark
})

const shouldShowMedia = computed(() => {
  return props.form.showMedia !== false && props.form.media
})

// 计算标题样式
const titleStyle = computed(() => {
  // 如果使用元素特定样式且已启用
  if (props.form.elementStyles && props.form.elementStyles.title && props.form.elementStyles.title.enabled) {
    const titleStyle = props.form.elementStyles.title
    return {
      fontSize: `${titleStyle.fontSize}px`,
      color: titleStyle.color,
      fontWeight: titleStyle.fontWeight || 'normal',
      marginBottom: '10px'
    }
  }
  
  // 否则使用通用样式设置
  if (props.form.style) {
    return {
      color: props.form.style.color,
      fontSize: `${props.form.style.fontSize}px`,
      fontWeight: 'normal',
      marginBottom: '10px'
    }
  }
  
  // 否则使用旧的样式属性
  return {
    fontSize: (props.form.titleFontSize || 16) + 'px',
    color: props.form.titleColor || '#333',
    fontWeight: 'bold'
  }
})

// 计算内容样式
const valueStyle = computed(() => {
  // 如果使用元素特定样式且已启用
  if (props.form.elementStyles && props.form.elementStyles.value && props.form.elementStyles.value.enabled) {
    const valueStyle = props.form.elementStyles.value
    return {
      fontSize: `${valueStyle.fontSize}px`,
      color: valueStyle.color,
      fontWeight: valueStyle.fontWeight || 'normal',
      marginBottom: '10px',
      whiteSpace: 'pre-wrap'
    }
  }
  
  // 否则使用通用样式设置
  if (props.form.style) {
    return {
      color: props.form.style.color,
      fontSize: `${props.form.style.fontSize}px`,
      fontWeight: 'normal',
      marginBottom: '10px',
      whiteSpace: 'pre-wrap'
    }
  }
  
  // 否则使用旧的样式属性
  return {
    fontSize: (props.form.valueFontSize || 16) + 'px',
    color: props.form.valueColor || '#333'
  }
})

// 计算备注样式
const remarkStyle = computed(() => {
  // 如果使用元素特定样式且已启用
  if (props.form.elementStyles && props.form.elementStyles.remark && props.form.elementStyles.remark.enabled) {
    const remarkStyle = props.form.elementStyles.remark
    return {
      fontSize: `${remarkStyle.fontSize}px`,
      color: remarkStyle.color,
      fontWeight: remarkStyle.fontWeight || 'normal',
      whiteSpace: 'pre-wrap'
    }
  }
  
  // 否则使用通用样式设置
  if (props.form.style) {
    return {
      color: props.form.style.color,
      fontSize: `${(props.form.style.fontSize || 14) - 2}px`, // 备注字体稍小
      fontWeight: 'normal',
      whiteSpace: 'pre-wrap'
    }
  }
  
  // 否则使用旧的样式属性
  return {
    fontSize: (props.form.remarkFontSize || 14) + 'px',
    color: props.form.remarkColor || '#666'
  }
})

// 计算媒体样式
const mediaStyle = computed(() => {
  // 如果使用元素特定样式且已启用
  if (props.form.elementStyles && props.form.elementStyles.media && props.form.elementStyles.media.enabled) {
    const mediaStyle = props.form.elementStyles.media
    return {
      fontSize: `${mediaStyle.fontSize}px`,
      color: mediaStyle.color,
      fontWeight: mediaStyle.fontWeight || 'normal'
    }
  }
  
  // 否则使用通用样式设置
  if (props.form.style) {
    return {
      color: props.form.style.color,
      fontSize: `${props.form.style.fontSize}px`,
      fontWeight: 'normal'
    }
  }
  
  // 否则使用默认样式
  return {}
})

// 计算卡片样式
const cardStyle = computed(() => {
  return {
    position: 'absolute',
    left: `${props.position.x}px`,
    top: `${props.position.y}px`,
    width: `${props.size.width}px`,
    height: `${props.size.height}px`,
    zIndex: props.form.zIndex || 1
  }
})

// 处理双击事件
const handleDoubleClick = () => {
  if (props.editable) {
    emit('dblclick', props.form)
  }
}

// 处理鼠标按下事件
const handleMouseDown = (event) => {
  // 只有在卡片本身（非操作按钮）上按下鼠标才触发
  if (event.target.classList.contains('action-button') || 
      event.target.closest('.form-actions') ||
      event.target.classList.contains('resize-handle')) {
    return
  }
  
  // 开始拖拽
  startDrag(event)
}

// 处理菜单命令
const handleCommand = (command) => {
  switch (command) {
    case 'edit':
      emit('edit', props.form)
      break
    case 'delete':
      emit('delete', props.form)
      break
  }
}

// 开始拖拽
const startDrag = (event) => {
  if (!props.editable) {
    // 在非编辑状态提示用户
    ElMessage.info('请先解锁页面再进行拖拽操作');
    return;
  }
  
  // 只有在卡片本身（非操作按钮）上按下鼠标才触发拖拽
  if (event.target.classList.contains('action-button') || 
      event.target.closest('.form-actions') ||
      event.target.classList.contains('resize-handle')) {
    return
  }
  
  event.preventDefault()
  handleDragStart(event) // 调用内部拖拽处理逻辑
}

// 开始调整大小
const startResize = (event) => {
  if (!props.editable) {
    // 在非编辑状态提示用户
    ElMessage.info('请先解锁页面再进行尺寸调整操作');
    return;
  }
  
  event.preventDefault()
  event.stopPropagation() // 阻止事件冒泡
  handleResizeStart(event) // 调用内部调整大小处理逻辑
}

// 内部拖拽处理逻辑
const handleDragStart = (event) => {
  // 确保必要的属性存在
  if (!props.form || !props.form.id) {
    console.warn('[SingleFormShow] 表单数据不完整，无法拖拽');
    return;
  }
  
  if (!props.position || typeof props.position.x === 'undefined' || typeof props.position.y === 'undefined') {
    console.warn('[SingleFormShow] 表单位置信息不完整，无法拖拽');
    return;
  }

  const startX = event.clientX
  const startY = event.clientY
  const startLeft = props.position.x
  const startTop = props.position.y

  const handleDragMove = (moveEvent) => {
    // 防止事件未定义
    if (!moveEvent) return;
    
    const deltaX = moveEvent.clientX - startX
    const deltaY = moveEvent.clientY - startY
    const newX = startLeft + deltaX
    const newY = startTop + deltaY
    
    emit('update-position', { 
      formId: props.form.id, 
      position: { x: newX, y: newY } 
    })
  }

  const handleDragEnd = () => {
    document.removeEventListener('mousemove', handleDragMove)
    document.removeEventListener('mouseup', handleDragEnd)
  }

  document.addEventListener('mousemove', handleDragMove)
  document.addEventListener('mouseup', handleDragEnd)
}

// 内部调整大小处理逻辑
const handleResizeStart = (event) => {
  // 确保必要的属性存在
  if (!props.form || !props.form.id) {
    console.warn('[SingleFormShow] 表单数据不完整，无法调整大小');
    return;
  }
  
  if (!props.size || typeof props.size.width === 'undefined' || typeof props.size.height === 'undefined') {
    console.warn('[SingleFormShow] 表单尺寸信息不完整，无法调整大小');
    return;
  }

  const startX = event.clientX
  const startY = event.clientY
  const startWidth = props.size.width
  const startHeight = props.size.height

  const handleResizeMove = (moveEvent) => {
    // 防止事件未定义
    if (!moveEvent) return;
    
    const deltaX = moveEvent.clientX - startX
    const deltaY = moveEvent.clientY - startY
    const newWidth = Math.max(100, startWidth + deltaX) // 最小宽度100px
    const newHeight = Math.max(50, startHeight + deltaY) // 最小高度50px
    
    emit('update-size', { 
      formId: props.form.id, 
      size: { width: newWidth, height: newHeight } 
    })
  }

  const handleResizeEnd = () => {
    document.removeEventListener('mousemove', handleResizeMove)
    document.removeEventListener('mouseup', handleResizeEnd)
  }

  document.addEventListener('mousemove', handleResizeMove)
  document.addEventListener('mouseup', handleResizeEnd)
}
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

.form-content {
  flex: 1;
  overflow-y: auto;
  padding: 4px;
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