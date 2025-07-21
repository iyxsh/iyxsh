<template>
  <div class="floating-bar" :class="{ 'collapsed': !visible }" :style="barStyle" @mousedown.self="startDrag">
    <transition name="slide-fade">
      <div v-if="visible" class="floating-content">
        <el-tooltip effect="dark" content="添加表单" placement="top">
          <el-button type="primary" size="small" circle @click="handleAddForm">
            <el-icon>
              <Plus />
            </el-icon>
          </el-button>
        </el-tooltip>

        <el-tooltip effect="dark" content="清空当前页" placement="top">
          <el-popconfirm title="确定要清空当前页面吗？" @confirm="clearCurrentPageForms" confirm-button-type="danger">
            <template #reference>
              <el-button type="danger" size="small" circle>
                <el-icon>
                  <Delete />
                </el-icon>
              </el-button>
            </template>
          </el-popconfirm>
        </el-tooltip>

        <el-tooltip effect="dark" :content="cardStyleOn ? '隐藏卡片样式' : '显示卡片样式'" placement="top">
          <el-button type="success" size="small" circle @click="toggleCardStyle">
            <el-icon>
              <Grid />
            </el-icon>
          </el-button>
        </el-tooltip>
      </div>
    </transition>
    <el-button class="toggle-btn" type="info" size="small" circle @click="toggleBar">
      <el-icon>
        <ArrowLeft v-if="visible" />
        <ArrowRight v-else />
      </el-icon>
    </el-button>
  </div>
</template>
<script setup>
import { ref, computed, reactive, onMounted, onBeforeUnmount } from 'vue'
import { ElMessage, ElMessageBox, ElDialog } from 'element-plus'  // 添加了 ElMessageBox 和 ElDialog 导入
import { useEventBus } from '../utils/eventBus'

// 添加组件加载调试信息
console.log('FloatingBar component initializing');

// 定义props
const props = defineProps({
  clearCurrentPageForms: Function,
  editable: Boolean,
  onToggleCardStyle: Function // 新增回调
})

// 定义emits
const emit = defineEmits(['add-form'])

// 本地状态
const visible = ref(true)
const cardStyleOn = ref(true)
let drag = false, offsetX = 0, offsetY = 0

const barStyle = reactive({ transform: 'translate(0, 0)' }) // 替换原来的left/top

function startDrag(e) {
  drag = true
  offsetX = e.clientX - barStyle.x
  offsetY = e.clientY - barStyle.y
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
}
function onDrag(e) {
  if (!drag) return
  barStyle.x = e.clientX - offsetX
  barStyle.y = e.clientY - offsetY
  barStyle.transform = `translate(${barStyle.x}px, ${barStyle.y}px)`
}
function stopDrag() {
  drag = false
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
}
function toggleBar() {
  visible.value = !visible.value
}
function toggleCardStyle() {
  cardStyleOn.value = !cardStyleOn.value
  if (props.onToggleCardStyle) props.onToggleCardStyle(cardStyleOn.value)
}

function clearCurrentPageForms() {
  if (props.editable && props.clearCurrentPageForms) {
    props.clearCurrentPageForms();
  } else if (!props.editable) {
    showUnlockMessage();
  }
}

function handleAddForm() {
  if (props.editable) {
    emit('add-form')
  } else {
    showUnlockMessage()
  }
}

function showUnlockMessage() {
  // 使用导入的 Element Plus 消息提示组件
  ElMessage.warning('Please unlock the page before performing this action');
}

// 资源清理
const cleanupTasks = []

// 添加内存泄漏检查
function checkMemoryUsage() {
  if (performance.memory) {
    const memoryUsage = performance.memory
    console.log('FloatingBar Memory Usage:', {
      usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
      totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
      jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
    })
    
    // 如果内存使用超过阈值，触发警告
    if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
      console.warn('FloatingBar: Memory usage is high, consider optimizing')
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

// 添加内存泄漏检查
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
  console.log('FloatingBar component mounted', {
    editable: props.editable
  });
});

// 在组件卸载前添加调试信息
onBeforeUnmount(() => {
  console.log('FloatingBar component about to be unmounted');
});
</script>
<style scoped>
.floating-bar {
  position: fixed;
  z-index: 2000;
  background: rgba(255,255,255,0.95);
  border: 1px solid rgba(0,0,0,0.08);
  box-shadow: 0 4px 12px rgba(0,0,0,0.12);
  border-radius: 24px;
  backdrop-filter: blur(8px);
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px;
  right: 24px;
  bottom: 24px;
  width: auto;
  min-width: 32px;
  max-width: 400px;
  height: 32px;
  transform: translate(v-bind('barStyle.transform'));
  will-change: transform; /* 提升动画性能 */
  
  &:hover {
    box-shadow: 0 6px 16px rgba(0,0,0,0.16);
  }
}

.floating-content {
  display: flex;
  gap: 4px;
  align-items: center;
  margin-right: 4px;
}

.toggle-btn {
  transition: transform 0.3s ease;
  &:hover {
    transform: scale(1.1);
  }
}

.slide-fade-enter-active {
  transition: all 0.3s ease-out;
}

.slide-fade-leave-active {
  transition: all 0.3s cubic-bezier(1, 0.5, 0.8, 1);
}

.slide-fade-enter-from,
.slide-fade-leave-to {
  opacity: 0;
  transform: translateX(10px);
}

.floating-bar.collapsed {
  .floating-content {
    display: none;
  }
  .toggle-btn {
    margin: 0;
  }
}

.el-button {
  --el-button-size: 22px;
  width: var(--el-button-size);
  height: var(--el-button-size);
  
  :deep(.el-icon) {
    font-size: 12px;
  }
}

.toggle-btn {
  margin-left: 1px;
  padding: 2px;
}
</style>