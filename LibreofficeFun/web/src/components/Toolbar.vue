<template>
  <div class="toolbar-container">
    <div class="toolbar">
      <div class="toolbar-left">
        <el-button @click="toggleCardStyle" :disabled="!isEditable">
          <el-icon>
            <Grid />
          </el-icon>
          {{ cardStyleOn ? '隐藏卡片样式' : '显示卡片样式' }}
        </el-button>

        <el-button @click="clearCurrentPageForms" :disabled="!isEditable">
          <el-icon>
            <Delete />
          </el-icon>
          清除当前页面
        </el-button>

        <el-button @click="addPage" :disabled="!isEditable">
          <el-icon>
            <DocumentAdd />
          </el-icon>
          添加页面
        </el-button>
      </div>

      <div class="toolbar-right">
        <el-switch
          :model-value="isEditable"
          @update:model-value="toggleEditMode"
          active-text="编辑模式"
          inactive-text="查看模式"
        />

        <el-select v-model="selectedPage" placeholder="选择页面" @change="changePage">
          <el-option
            v-for="(page, index) in pages"
            :key="index"
            :label="`页面 ${index + 1}`"
            :value="index"
          />
        </el-select>

        <el-button @click="exportData">
          导出
        </el-button>

        <el-button @click="importData">
          导入
        </el-button>

        <el-button @click="saveAll">
          <el-icon>
            <Save />
          </el-icon>
          保存全部
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onBeforeUnmount } from 'vue'
import { ElMessage, ElMessageBox, ElDialog, ElButton } from 'element-plus'
import { useEventBus } from '../utils/eventBus'
import { DocumentAdd } from '@element-plus/icons-vue'

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

// 处理页面切换
const changePage = (pageIndex) => {
  selectedPage.value = pageIndex
  emit('change-page', pageIndex)
}

// 添加页面方法
const addPage = () => {
  if (props.onAddPage) {
    props.onAddPage()
  } else {
    console.error('onAddPage 方法未定义')
  }
}

// 清空当前页
const clearCurrentPageForms = () => {
  if (isEditable.value) {
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
  } else {
    showUnlockMessage()
  }
}

// 切换卡片样式
const toggleCardStyle = () => {
  if (isEditable.value) {
    emit('toggle-card-style')
  } else {
    showUnlockMessage()
  }
}

// 切换编辑模式
const toggleEditMode = (value) => {
  isEditable.value = value
  emit('toggle-edit-mode', value)
}

// 导出数据
const exportData = () => {
  emit('export-data')
}

// 导入数据
const importData = () => {
  emit('import-data')
}

// 保存所有数据
const saveAll = () => {
  emit('save-all')
}

// 显示解锁提示
const showUnlockMessage = () => {
  ElMessage.warning('请先切换到编辑模式再执行此操作')
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

// 在组件卸载前执行清理
onBeforeUnmount(() => {
  // 执行资源清理
  cleanupResources()

  console.log('Toolbar component about to be unmounted');
})
</script>

<style scoped>
.toolbar-container {
  width: 100%;
  padding: 8px;
  background-color: #f5f7fa;
  border-bottom: 1px solid #e4e4e4;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.1);
  transition: all 0.3s ease;
}

.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  max-width: 1200px;
  margin: 0 auto;
}

.toolbar-left,
.toolbar-right {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.toolbar .el-button {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 14px;
  padding: 8px 14px;
  min-width: 90px;
  justify-content: center;
}

.toolbar .el-icon {
  font-size: 16px;
}

.toolbar .el-select {
  width: 140px;
}

.toolbar .el-switch {
  margin-right: 12px;
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

@media (max-width: 768px) {
  .toolbar {
    flex-direction: column;
    align-items: flex-start;
  }

  .toolbar-left,
  .toolbar-right {
    width: 100%;
    margin-bottom: 10px;
  }

  .toolbar-right {
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
  }

  .toolbar .el-select {
    width: 100%;
    margin-bottom: 10px;
  }

  /* 移动设备上调整按钮样式 */
  .toolbar .el-button {
    width: calc(50% - 6px);
    justify-content: center;
  }
}
</style>