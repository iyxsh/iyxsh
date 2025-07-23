<template>
  <div class="toolbar-container" :class="{ 'toolbar-dragging': isDragging, 'toolbar-docked': isDocked, 'toolbar-minimized': isMinimized, 'toolbar-floating': !isDocked }" ref="toolbarContainer" :style="toolbarPosition">
    <!-- 拖动手柄 -->
    <div class="toolbar-drag-handle" @mousedown="startDrag">
      <el-icon><Rank /></el-icon>
    </div>

    <!-- 工具栏操作按钮 -->
    <div class="toolbar-actions">
      <el-button circle size="small" @click="toggleMinimize" class="toolbar-action-button">
        <el-icon>
          <component :is="isMinimized ? 'Plus' : 'Minus'"></component>
        </el-icon>
      </el-button>
      <el-button circle size="small" @click="toggleDock" class="toolbar-action-button">
        <el-icon>
          <component :is="isDocked ? 'TakeawayBox' : 'Position'"></component>
        </el-icon>
      </el-button>
    </div>

    <!-- 主工具栏内容 -->
    <div class="toolbar" v-show="!isMinimized">
      <div class="toolbar-section toolbar-left">
        <el-tooltip content="切换卡片样式" placement="bottom" :disabled="!isEditable">
          <el-button @click="toggleCardStyle" :disabled="!isEditable" type="primary" plain>
            <el-icon><Grid /></el-icon>
            <span class="button-text">{{ cardStyleOn ? '隐藏样式' : '显示样式' }}</span>
          </el-button>
        </el-tooltip>

        <el-tooltip content="清除当前页面" placement="bottom" :disabled="!isEditable">
          <el-button @click="clearCurrentPageForms" :disabled="!isEditable" type="danger" plain>
            <el-icon><Delete /></el-icon>
            <span class="button-text">清除页面</span>
          </el-button>
        </el-tooltip>

        <el-tooltip content="添加新页面" placement="bottom" :disabled="!isEditable">
          <el-button @click="addPage" :disabled="!isEditable" type="success" plain>
            <el-icon><DocumentAdd /></el-icon>
            <span class="button-text">添加页面</span>
          </el-button>
        </el-tooltip>
      </div>

      <div class="toolbar-section toolbar-right">
        <el-tooltip content="切换编辑/预览模式" placement="bottom">
          <el-switch
            :model-value="isEditable"
            @update:model-value="toggleEditMode"
            active-text="编辑"
            inactive-text="预览"
            inline-prompt
          />
        </el-tooltip>

        <el-tooltip content="选择要编辑的页面" placement="bottom">
          <el-select v-model="selectedPage" placeholder="选择页面" @change="changePage" size="small">
            <el-option
              v-for="(page, index) in pages"
              :key="index"
              :label="`页面 ${index + 1}`"
              :value="index"
            />
          </el-select>
        </el-tooltip>

        <el-button-group>
          <el-tooltip content="导出数据" placement="bottom">
            <el-button @click="exportData" type="info" plain>
              <el-icon><Upload /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="导入数据" placement="bottom">
            <el-button @click="importData" type="info" plain>
              <el-icon><Download /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="保存全部" placement="bottom">
            <el-button @click="saveAll" type="primary">
              <el-icon><Save /></el-icon>
              <span class="button-text">保存</span>
            </el-button>
          </el-tooltip>
        </el-button-group>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, computed, nextTick } from 'vue'
import { ElMessage, ElMessageBox, ElDialog, ElButton, ElTooltip } from 'element-plus'
import { useEventBus } from '../utils/eventBus'
import { DocumentAdd, Grid, Delete, Upload, Download, Position, Rank, Plus, Minus } from '@element-plus/icons-vue'
import { Share as Save } from '@element-plus/icons-vue'
import { CollectionTag as TakeawayBox } from '@element-plus/icons-vue'

// 添加组件加载调试信息
console.log('Toolbar component initializing');

// 定义props
const props = defineProps({
  pages: Array,
  currentPage: Number,
  cardStyleOn: Boolean,
  editable: Boolean,
  onAddPage: {
    type: Function,
    required: true
  },
  clearAllPages: Function,
  rotatePage: Function,
  showRotateButton: Boolean
})

// 创建计算属性或本地状态以避免直接修改props
const isEditable = ref(props.editable)

// 定义emits
const emit = defineEmits(['toggle-card-style', 'clear-current-page-forms', 'add-form', 'toggle-edit-mode', 'change-page', 'export-data', 'import-data', 'save-all', 'on-add-page'])

// 使用事件总线
const { on, off } = useEventBus()

// 本地状态
const selectedPage = ref(props.currentPage)

// 拖拽相关状态
const isDragging = ref(false)
const isDocked = ref(false) // 默认为浮动状态
const isMinimized = ref(false)
const toolbarContainer = ref(null)
const dragOffset = reactive({ x: 0, y: 0 })
const position = reactive({ x: 20, y: 20 })

// 计算工具栏位置样式
const toolbarPosition = computed(() => {
  if (isDocked.value) return {}
  return {
    left: `${position.x}px`,
    top: `${position.y}px`,
    position: 'fixed',
    zIndex: 1000,
    margin: 0, // 覆盖默认margin
    width: 'auto' // 让宽度自适应内容
  }
})

// 错误处理函数
const handleError = (error, context = '未知上下文') => {
  console.error(`[Toolbar Error] ${context}`, error);
  ElMessage.error({
    message: `操作失败: ${context}`,
    duration: 5000,
    type: 'error'
  });
  
  // 记录错误日志
  errorLogService.addErrorLog(error, context, 'error');
}

// 处理页面切换
const changePage = (pageIndex) => {
  try {
    if (pageIndex < 0 || pageIndex >= props.pages.length) {
      throw new Error(`无效的页面索引: ${pageIndex}`);
    }
    
    selectedPage.value = pageIndex
    emit('change-page', pageIndex)
  } catch (error) {
    handleError(error, '页面切换失败')
  }
}

// 添加页面方法
const addPage = () => {
  try {
    if (!props.onAddPage) {
      throw new Error('onAddPage 方法未定义')
    }
    
    props.onAddPage()
  } catch (error) {
    handleError(error, '添加页面失败')
  }
}

// 清空当前页
const clearCurrentPageForms = () => {
  try {
    if (!isEditable.value) {
      showUnlockMessage()
      return
    }
    
    ElMessageBox.confirm(
      '确定要清除当前页面的所有表单吗？此操作无法撤销。',
      '清除确认',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    ).then(() => {
      emit('clear-current-page-forms')
    }).catch(() => {
      // 用户取消操作
    })
  } catch (error) {
    handleError(error, '清除表单失败')
  }
}

// 切换卡片样式
const toggleCardStyle = () => {
  try {
    if (!isEditable.value) {
      showUnlockMessage()
      return
    }
    
    emit('toggle-card-style')
  } catch (error) {
    handleError(error, '切换卡片样式失败')
  }
}

// 切换编辑模式
const toggleEditMode = (value) => {
  try {
    isEditable.value = value
    emit('toggle-edit-mode', value)
  } catch (error) {
    handleError(error, '切换编辑模式失败')
  }
}

// 导出数据
const exportData = () => {
  try {
    emit('export-data')
  } catch (error) {
    handleError(error, '导出数据失败')
  }
}

// 导入数据
const importData = () => {
  try {
    emit('import-data')
  } catch (error) {
    handleError(error, '导入数据失败')
  }
}

// 保存所有数据
const saveAll = () => {
  try {
    emit('save-all')
  } catch (error) {
    handleError(error, '保存数据失败')
  }
}

// 显示解锁提示
const showUnlockMessage = () => {
  ElMessage.warning({
    message: '请先切换到编辑模式再执行此操作',
    duration: 3000
  })
}

// 资源清理
const cleanupTasks = []

// 添加内存泄漏检查
function checkMemoryUsage() {
  if (performance.memory) {
    const memoryUsage = performance.memory
    console.log('Toolbar Memory Usage:', {
      usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
      totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
      jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
    })

    // 如果内存使用超过阈值，触发警告
    if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
      console.warn('Toolbar: Memory usage is high, consider optimizing')
    }
  }
}

// 格式化字节大小
function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes'
  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

// 优化内存泄漏检查
function setupMemoryLeakCheck() {
  // 检查内存使用情况
  const memoryCheckInterval = setInterval(() => {
    checkMemoryUsage()
  }, 5 * 60 * 1000)

  // 添加清理任务
  cleanupTasks.push(() => clearInterval(memoryCheckInterval))
}

// 添加资源清理
function cleanupResources() {
  // 清理内存检查
  cleanupTasks.forEach(task => task())
}

// 在组件挂载时添加调试信息
onMounted(() => {
  // 初始化内存泄漏检查
  setupMemoryLeakCheck()

  console.log('Toolbar component mounted', {
    currentPage: props.currentPage,
    cardStyleOn: props.cardStyleOn,
    editable: props.editable
  });
})

// 工具栏拖动相关方法
const startDrag = (event) => {
  // 防止事件冒泡和默认行为
  event.stopPropagation()
  event.preventDefault()
  
  console.log('开始拖动工具栏', { isDocked: isDocked.value, clientX: event.clientX, clientY: event.clientY })
  
  // 如果工具栏已停靠，点击拖动手柄时解除停靠
  if (isDocked.value) {
    isDocked.value = false
    localStorage.setItem('toolbar-docked', 'false')
    
    // 设置初始位置在点击的位置附近
    position.x = Math.max(0, event.clientX - 100)
    position.y = Math.max(0, event.clientY - 20)
    
    // 确保视图更新后再开始拖动
    nextTick(() => {
      isDragging.value = true
      dragOffset.x = 100
      dragOffset.y = 20
      document.addEventListener('mousemove', handleDrag)
      document.addEventListener('mouseup', stopDrag)
    })
    return
  }
  
  // 已经是浮动状态，直接开始拖动
  isDragging.value = true
  
  // 计算鼠标点击位置相对于工具栏左上角的偏移量
  dragOffset.x = event.clientX - position.x
  dragOffset.y = event.clientY - position.y
  
  console.log('计算拖动偏移量', { dragOffset, clientPos: { x: event.clientX, y: event.clientY }, position })
  
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', stopDrag)
  
  // 防止选择文本
  event.preventDefault()
}

const handleDrag = (event) => {
  if (!isDragging.value) return
  
  position.x = event.clientX - dragOffset.x
  position.y = event.clientY - dragOffset.y
  
  // 确保工具栏不超出屏幕边界
  const { clientWidth, clientHeight } = document.documentElement
  const toolbar = toolbarContainer.value
  
  if (toolbar) {
    const { width, height } = toolbar.getBoundingClientRect()
    position.x = Math.max(0, Math.min(position.x, clientWidth - width))
    position.y = Math.max(0, Math.min(position.y, clientHeight - height))
  }
}

const stopDrag = () => {
  isDragging.value = false
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
  
  // 保存位置到本地存储
  localStorage.setItem('toolbar-position', JSON.stringify(position))
}

// 工具栏停靠/浮动切换
const toggleDock = () => {
  isDocked.value = !isDocked.value
  localStorage.setItem('toolbar-docked', isDocked.value.toString())
  
  if (!isDocked.value) {
    // 如果未停靠，确保位置合理
    nextTick(() => {
      const toolbar = toolbarContainer.value
      if (toolbar) {
        const { width, height } = toolbar.getBoundingClientRect()
        const { clientWidth, clientHeight } = document.documentElement
        
        position.x = Math.max(0, Math.min(position.x, clientWidth - width))
        position.y = Math.max(0, Math.min(position.y, clientHeight - height))
      }
    })
  }
}

// 最小化/展开工具栏
const toggleMinimize = () => {
  isMinimized.value = !isMinimized.value
  localStorage.setItem('toolbar-minimized', isMinimized.value.toString())
}

// 加载保存的工具栏状态
const loadToolbarState = () => {
  try {
    // 加载位置
    const savedPosition = localStorage.getItem('toolbar-position')
    if (savedPosition) {
      try {
        const parsed = JSON.parse(savedPosition)
        position.x = parsed.x || 20
        position.y = parsed.y || 20
      } catch (e) {
        console.error('解析工具栏位置失败', e)
        position.x = 20
        position.y = 20
      }
    }
    
    // 加载停靠状态，默认为浮动状态（非固定）
    const savedDocked = localStorage.getItem('toolbar-docked')
    if (savedDocked !== null) {
      isDocked.value = savedDocked === 'true'
    } else {
      // 默认为浮动状态
      isDocked.value = false
      localStorage.setItem('toolbar-docked', 'false')
    }
    
    // 加载最小化状态
    const savedMinimized = localStorage.getItem('toolbar-minimized')
    if (savedMinimized !== null) {
      isMinimized.value = savedMinimized === 'true'
    }
  } catch (error) {
    console.error('加载工具栏状态失败', error)
  }
}

// 在组件挂载时加载状态
onMounted(() => {
  // 初始化内存泄漏检查
  setupMemoryLeakCheck()
  
  // 加载工具栏状态
  loadToolbarState()

  console.log('Toolbar component mounted', {
    currentPage: props.currentPage,
    cardStyleOn: props.cardStyleOn,
    editable: props.editable,
    isDocked: isDocked.value,
    isMinimized: isMinimized.value
  });
  
  // 监听窗口大小变化，确保工具栏位置有效
  window.addEventListener('resize', handleWindowResize)
  cleanupTasks.push(() => window.removeEventListener('resize', handleWindowResize))
})

// 处理窗口大小变化
const handleWindowResize = () => {
  if (!isDocked.value && toolbarContainer.value) {
    const { width, height } = toolbarContainer.value.getBoundingClientRect()
    const { clientWidth, clientHeight } = document.documentElement
    
    position.x = Math.max(0, Math.min(position.x, clientWidth - width))
    position.y = Math.max(0, Math.min(position.y, clientHeight - height))
  }
}

// 在组件卸载前执行清理
onBeforeUnmount(() => {
  // 执行资源清理
  cleanupResources()
  
  // 移除拖动相关的事件监听
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)

  console.log('Toolbar component about to be unmounted');
})
</script>

<style scoped>
/* 工具栏容器样式 */
.toolbar-container {
  width: 100%;
  background-color: #ffffff;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  transition: all 0.3s ease;
  padding: 8px;
  margin-bottom: 10px;
  border: 1px solid rgba(0, 0, 0, 0.1);
}

/* 浮动工具栏 */
.toolbar-floating {
  position: fixed !important;
  z-index: 2000 !important;
  border-radius: 10px !important;
  min-width: 300px;
  max-width: 90vw;
  backdrop-filter: blur(10px);
  background-color: rgba(255, 255, 255, 0.95);
  animation: fadein 0.3s;
  margin: 0 !important; /* 覆盖默认margin */
  box-sizing: border-box;
  width: auto !important;
}

/* 拖动时样式 */
.toolbar-dragging {
  opacity: 0.9;
  cursor: grabbing;
  box-shadow: 0 5px 20px rgba(0, 0, 0, 0.15);
  transform: scale(1.01);
  border: 1px dashed #409EFF;
}

/* 最小化工具栏 */
.toolbar-minimized {
  padding: 4px;
  max-width: 200px;
}

/* 拖动手柄 */
.toolbar-drag-handle {
  cursor: grab;
  height: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 4px;
  border-radius: 4px;
  background-color: rgba(0, 0, 0, 0.03);
  padding-bottom: 4px;
  border-bottom: 1px dashed #dcdfe6;
  user-select: none; /* 防止文本选择 */
  touch-action: none; /* 防止触摸操作 */
  z-index: 10; /* 确保在其他元素上层 */
}

.toolbar-drag-handle:hover {
  background-color: rgba(0, 0, 0, 0.05);
}

.toolbar-drag-handle .el-icon {
  color: #909399;
  font-size: 16px;
}

/* 工具栏操作按钮 */
.toolbar-actions {
  display: flex;
  position: absolute;
  top: 5px;
  right: 5px;
  gap: 5px;
}

.toolbar-action-button {
  padding: 2px !important;
  height: 24px !important;
  width: 24px !important;
}

/* 主工具栏内容 */
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
}

.toolbar-section {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

/* 按钮样式 */
.toolbar .el-button {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-weight: 500;
}

.toolbar .el-button .button-text {
  margin-left: 2px;
  white-space: nowrap;
}

.toolbar .el-icon {
  font-size: 16px;
}

.toolbar .el-select {
  width: 140px;
}

/* 屏幕阅读器友好样式 */
.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border-width: 0;
}

/* 动画 */
@keyframes fadein {
  from { opacity: 0; transform: translateY(-10px); }
  to { opacity: 1; transform: translateY(0); }
}

/* 响应式设计 */
@media (max-width: 768px) {
  .toolbar {
    flex-direction: column;
    align-items: stretch;
    gap: 15px;
  }

  .toolbar-section {
    width: 100%;
    justify-content: space-between;
  }
  
  .toolbar-right {
    justify-content: flex-end;
  }

  .button-text {
    display: none;
  }
  
  .toolbar .el-button {
    padding: 8px;
    min-width: unset;
  }
  
  .toolbar .el-select {
    width: 120px;
  }
  
  .toolbar-container:not(.toolbar-docked):not(.toolbar-minimized) {
    width: 90vw;
    max-height: 90vh;
    overflow-y: auto;
  }
}

.toolbar-drag-handle:hover {
  color: #409EFF;
}

/* 停靠时不显示拖动手柄 */
.toolbar-docked .toolbar-drag-handle {
  display: none;
}

/* 工具栏操作按钮 */
.toolbar-actions {
  position: absolute;
  top: 5px;
  right: 5px;
  display: flex;
  gap: 4px;
}

.toolbar-action-button {
  width: 24px !important;
  height: 24px !important;
  padding: 0 !important;
  min-height: 24px !important;
  border: none !important;
}

.toolbar-action-button .el-icon {
  font-size: 12px !important;
}

.toolbar-action-button:hover {
  background-color: #f2f6fc !important;
}

/* 工具栏主内容 */
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  margin: 0 auto;
  transition: all 0.3s ease;
  gap: 10px;
}

.toolbar-section {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.toolbar .el-button {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
  padding: 6px 12px;
  justify-content: center;
  border-radius: 6px;
  height: 32px;
}

.toolbar .el-icon {
  font-size: 14px;
}

.toolbar .el-select {
  width: 120px;
}

.toolbar .el-switch {
  margin-right: 8px;
}

.toolbar-section.toolbar-right {
  justify-content: flex-end;
}

/* 按钮组样式优化 */
.el-button-group {
  display: flex;
  align-items: center;
}

/* 动画效果 */
@keyframes fadein {
  from { opacity: 0; transform: translateY(-10px); }
  to { opacity: 1; transform: translateY(0); }
}

/* 响应式布局 */
@media (max-width: 768px) {
  .toolbar {
    flex-direction: column;
    align-items: stretch;
    gap: 10px;
  }

  .toolbar-section {
    width: 100%;
    justify-content: center;
  }

  .toolbar-section.toolbar-right {
    justify-content: center;
  }

  .button-text {
    display: none;
  }

  .toolbar .el-button {
    padding: 6px;
    min-width: unset;
  }

  .toolbar-container:not(.toolbar-docked) {
    min-width: 200px;
  }
}

/* 在小屏幕上隐藏按钮文本，只显示图标 */
@media (max-width: 480px) {
  .button-text {
    display: none;
  }

  .toolbar .el-button {
    padding: 6px;
    min-width: unset;
  }

  .toolbar-container:not(.toolbar-docked) {
    min-width: 200px;
  }
}
</style>