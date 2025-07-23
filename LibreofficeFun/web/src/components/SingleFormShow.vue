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
          v-if="form.showTitle !== false && form.title" 
          class="form-title"
          :style="{ 
            fontSize: (form.titleFontSize || 16) + 'px', 
            color: form.titleColor || '#333',
            fontWeight: 'bold'
          }"
        >
          {{ form.title }}
        </div>

        <!-- 内容 -->
        <div 
          v-if="form.showValue !== false && form.value" 
          class="form-value"
          :style="{ 
            fontSize: (form.valueFontSize || 16) + 'px', 
            color: form.valueColor || '#333' 
          }"
        >
          {{ form.value }}
        </div>

        <!-- 备注 -->
        <div 
          v-if="form.showRemark !== false && form.remark" 
          class="form-remark"
          :style="{ 
            fontSize: (form.remarkFontSize || 14) + 'px', 
            color: form.remarkColor || '#666' 
          }"
        >
          {{ form.remark }}
        </div>

        <!-- 媒体内容（如果有的话） -->
        <div 
          v-if="form.showMedia !== false && form.media" 
          class="form-media"
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
import { ElCard, ElDropdown, ElDropdownMenu, ElDropdownItem, ElButton, ElIcon } from 'element-plus'
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
  
  emit('mousedown', props.form)
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
  if (!props.editable) return
  
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
  if (!props.editable) return
  
  event.preventDefault()
  event.stopPropagation()
  handleResizeStart(event) // 调用内部调整大小处理逻辑
}

// 添加拖拽处理逻辑
const isDragging = ref(false)
const dragStartPos = ref({ x: 0, y: 0 })
const formStartPos = ref({ x: 0, y: 0 })

// 处理拖拽开始
const handleDragStart = (event) => {
  if (!props.editable) return
  
  isDragging.value = true
  dragStartPos.value = { x: event.clientX, y: event.clientY }
  formStartPos.value = { ...props.position }
  
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', handleDragEnd)
}

// 处理拖拽过程
const handleDrag = (event) => {
  if (!isDragging.value) return
  
  const deltaX = event.clientX - dragStartPos.value.x
  const deltaY = event.clientY - dragStartPos.value.y
  
  const newPosition = {
    x: formStartPos.value.x + deltaX,
    y: formStartPos.value.y + deltaY
  }
  
  emit('update-position', { formId: props.form.id, position: newPosition })
}

// 处理拖拽结束
const handleDragEnd = (event) => {
  if (!isDragging.value) return
  
  isDragging.value = false
  
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', handleDragEnd)
}

// 添加调整大小处理逻辑
const isResizing = ref(false)
const resizeStartPos = ref({ x: 0, y: 0 })
const formStartSize = ref({ width: 0, height: 0 })

// 处理调整大小开始
const handleResizeStart = (event) => {
  if (!props.editable) return
  
  isResizing.value = true
  resizeStartPos.value = { x: event.clientX, y: event.clientY }
  formStartSize.value = { ...props.size }
  
  document.addEventListener('mousemove', handleResize)
  document.addEventListener('mouseup', handleResizeEnd)
  
  event.stopPropagation()
}

// 处理调整大小过程
const handleResize = (event) => {
  if (!isResizing.value) return
  
  const deltaX = event.clientX - resizeStartPos.value.x
  const deltaY = event.clientY - resizeStartPos.value.y
  
  const newSize = {
    width: Math.max(100, formStartSize.value.width + deltaX), // 最小宽度100px
    height: Math.max(80, formStartSize.value.height + deltaY)  // 最小高度80px
  }
  
  emit('update-size', { formId: props.form.id, size: newSize })
}

// 处理调整大小结束
const handleResizeEnd = (event) => {
  if (!isResizing.value) return
  
  isResizing.value = false
  
  document.removeEventListener('mousemove', handleResize)
  document.removeEventListener('mouseup', handleResizeEnd)
}
</script>

<style scoped>
.single-form-card-wrapper {
  position: absolute;
  cursor: move;
  transition: all 0.2s ease;
}

.single-form-card-wrapper:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.single-form-card {
  position: relative;
  width: 100%;
  height: 100%;
  cursor: pointer;
  transition: all 0.3s ease;
  border: 1px solid transparent;
}

.single-form-card:hover {
  border-color: #ddd;
}

.single-form-card.editable:hover {
  border-color: #409eff;
}

.single-form-card.no-style {
  background: none;
  box-shadow: none;
  border: 1px solid transparent;
}

.form-content {
  display: flex;
  flex-direction: column;
  gap: 8px;
  height: calc(100% - 20px);
  overflow: auto;
  padding: 12px;
  box-sizing: border-box;
}

.form-title,
.form-value,
.form-remark {
  word-wrap: break-word;
  word-break: break-all;
  line-height: 1.4;
}

.form-title:empty,
.form-value:empty,
.form-remark:empty {
  display: none;
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
  opacity: 0;
  transition: opacity 0.3s ease;
  z-index: 10;
}

.single-form-card-wrapper:hover .form-actions {
  opacity: 1;
}

.action-button {
  background: rgba(255, 255, 255, 0.9);
  border: 1px solid #ddd;
  backdrop-filter: blur(10px);
}

.resize-handle {
  position: absolute;
  right: 2px;
  bottom: 2px;
  width: 16px;
  height: 16px;
  background: rgba(0, 0, 0, 0.08);
  border-radius: 3px;
  cursor: se-resize;
  user-select: none;
  pointer-events: auto;
  z-index: 1000;
  transition: background-color 0.2s;
}

.resize-handle:hover {
  background: rgba(64, 158, 255, 0.7);
}

.single-form-card:not(.editable) .resize-handle {
  display: none;
}
</style>