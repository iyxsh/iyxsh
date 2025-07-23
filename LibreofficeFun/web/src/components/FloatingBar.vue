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
import { ref, reactive, onMounted, onBeforeUnmount } from 'vue'
import { ElMessage } from 'element-plus'

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

const barStyle = reactive({ 
  x: 0, 
  y: 0, 
  transform: 'translate(0, 0)' 
})

// 错误处理函数
const handleError = (error, context = '未知上下文') => {
  console.error(`[FloatingBar Error] ${context}`, error);
  ElMessage.error({
    message: `操作失败: ${context}`,
    duration: 5000,
    type: 'error'
  });
}

function startDrag(e) {
  try {
    drag = true
    offsetX = e.clientX - barStyle.x
    offsetY = e.clientY - barStyle.y
    document.addEventListener('mousemove', onDrag)
    document.addEventListener('mouseup', stopDrag)
  } catch (error) {
    handleError(error, '开始拖拽失败')
  }
}

function onDrag(e) {
  try {
    if (!drag) return
    barStyle.x = e.clientX - offsetX
    barStyle.y = e.clientY - offsetY
    barStyle.transform = `translate(${barStyle.x}px, ${barStyle.y}px)`
  } catch (error) {
    handleError(error, '拖拽过程中失败')
  }
}

function stopDrag() {
  try {
    drag = false
    document.removeEventListener('mousemove', onDrag)
    document.removeEventListener('mouseup', stopDrag)
  } catch (error) {
    handleError(error, '停止拖拽失败')
  }
}

function toggleBar() {
  try {
    visible.value = !visible.value
  } catch (error) {
    handleError(error, '切换工具栏失败')
  }
}

function toggleCardStyle() {
  try {
    cardStyleOn.value = !cardStyleOn.value
    if (props.onToggleCardStyle) props.onToggleCardStyle(cardStyleOn.value)
  } catch (error) {
    handleError(error, '切换卡片样式失败')
  }
}

function clearCurrentPageForms() {
  try {
    if (!props.editable && !props.clearCurrentPageForms) {
      showUnlockMessage();
      return
    }
    
    if (props.clearCurrentPageForms) {
      props.clearCurrentPageForms();
    }
  } catch (error) {
    handleError(error, '清除当前页面表单失败')
  }
}

function handleAddForm() {
  try {
    if (!props.editable) {
      showUnlockMessage()
      return
    }
    
    emit('add-form')
  } catch (error) {
    handleError(error, '添加表单失败')
  }
}

function showUnlockMessage() {
  try {
    // 使用导入的 Element Plus 消息提示组件
    ElMessage.warning({
      message: '请先解锁页面再执行此操作',
      duration: 3000
    });
  } catch (error) {
    console.error('显示解锁提示失败:', error);
  }
}

// 在组件卸载时清理事件监听器，防止内存泄漏
onBeforeUnmount(() => {
  try {
    // 确保释放所有事件监听器
    document.removeEventListener('mousemove', onDrag)
    document.removeEventListener('mouseup', stopDrag)
    console.log('[FloatingBar] 成功清理事件监听器')
  } catch (error) {
    console.error('[FloatingBar] 清理事件监听器失败:', error)
  }
})
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