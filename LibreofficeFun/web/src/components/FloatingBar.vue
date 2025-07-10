<template>
  <div class="floating-bar" :class="{ 'collapsed': !visible }" :style="barStyle" @mousedown.self="startDrag">
    <transition name="fade">
      <div v-if="visible" class="floating-bar-content">
        <el-tooltip content="Please unlock the page before adding forms" placement="top" :disabled="props.editable">
          <div>
            <el-button type="primary" @click="props.editable ? $emit('add-form') : showUnlockMessage()" :disabled="!props.editable">Add Form</el-button>
          </div>
        </el-tooltip>
        <el-tooltip content="Please unlock the page before clearing content" placement="top" :disabled="props.editable">
          <div>
            <el-popconfirm title="Are you sure you want to clear all content on this page? This action cannot be undone. Continue?" @confirm="clearCurrentPageForms">
              <template #reference>
                <el-button type="warning" :disabled="!props.editable">Clear Current Page</el-button>
              </template>
            </el-popconfirm>
          </div>
        </el-tooltip>
        <el-button type="success" @click="toggleCardStyle" style="margin-left:8px;">{{ cardStyleOn ? 'Hide Card Style' : 'Show Card Style' }}</el-button>
      </div>
    </transition>
    <el-button type="info" @click="toggleBar" style="margin-left:8px;">
      {{ visible ? '◀ Hide' : '▶ Show' }}
    </el-button>
  </div>
</template>
<script setup>
import { ref, computed,reactive, onMounted, onBeforeUnmount } from 'vue'
import { ElMessage } from 'element-plus'
const props = defineProps({
  clearCurrentPageForms: Function,
  editable: Boolean,
  onToggleCardStyle: Function // 新增回调
})
const emit = defineEmits(['add-form'])
const visible = ref(true)
const cardStyleOn = ref(true)
const barStyle = reactive({ left: '60%', top: '80px' })
let drag = false, offsetX = 0, offsetY = 0
function startDrag(e) {
  drag = true
  offsetX = e.clientX - parseInt(barStyle.left)
  offsetY = e.clientY - parseInt(barStyle.top)
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
}
function onDrag(e) {
  if (!drag) return
  barStyle.left = e.clientX - offsetX + 'px'
  barStyle.top = e.clientY - offsetY + 'px'
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

function showUnlockMessage() {
  // 使用导入的 Element Plus 消息提示组件
  ElMessage.warning('Please unlock the page before performing this action');
}
onMounted(() => {
  // Ensure initial position is within window
  barStyle.left = Math.min(window.innerWidth - 200, parseInt(barStyle.left)) + 'px'
})

onBeforeUnmount(() => {
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
})
</script>
<style scoped>
.floating-bar {
  position: fixed;
  z-index: 100;
  background: #fff;
  border: 1px solid #eee;
  box-shadow: 0 2px 8px #ccc;
  padding: 10px 16px;
  border-radius: 8px;
  cursor: move;
  user-select: none;
  transition: all 0.3s ease;
  display: flex;
  gap: 8px;
  align-items: center;
  top: 80px;
  right: 20px;
}

.floating-bar.collapsed {
  width: auto;
  min-width: auto;
  padding: 8px;
}

.floating-bar-content {
  display: flex;
  gap: 8px;
  align-items: center;
}

.fade-enter-active, .fade-leave-active {
  transition: opacity 0.3s;
}

.fade-enter-from, .fade-leave-to {
  opacity: 0;
}
</style>