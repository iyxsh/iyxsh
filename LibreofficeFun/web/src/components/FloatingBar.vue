<template>
  <transition name="fade">
    <div v-show="visible" class="floating-bar" :style="barStyle" @mousedown.self="startDrag">
      <el-button type="primary" @click="addForm">Add Form</el-button>
      <el-popconfirm title="Are you sure you want to clear all content on this page? This action cannot be undone. Continue?" @confirm="clearCurrentPageForms">
        <template #reference>
          <el-button type="warning">Clear Current Page</el-button>
        </template>
      </el-popconfirm>
      <el-button type="info" @click="toggleBar" style="margin-left:8px;">{{ visible ? 'Hide' : 'Show' }}</el-button>
      <el-button type="success" @click="toggleCardStyle" style="margin-left:8px;">{{ cardStyleOn ? 'Hide Card Style' : 'Show Card Style' }}</el-button>
    </div>
  </transition>
  <div v-show="!visible" class="floating-bar-toggle" @click="toggleBar">▶</div>
</template>
<script setup>
import { ref, reactive, onMounted } from 'vue'
const props = defineProps({
  clearCurrentPageForms: Function,
  addForm: Function,
  onToggleCardStyle: Function // 新增回调
})
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
onMounted(() => {
  // Ensure initial position is within window
  barStyle.left = Math.min(window.innerWidth - 200, parseInt(barStyle.left)) + 'px'
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
  min-width: 180px;
  user-select: none;
  transition: box-shadow 0.2s;
  display: flex;
  gap: 8px;
  align-items: center;
}
.floating-bar-toggle {
  position: fixed;
  right: 0;
  top: 120px;
  z-index: 101;
  background: #409eff;
  color: #fff;
  padding: 4px 8px;
  border-radius: 4px 0 0 4px;
  cursor: pointer;
  font-size: 16px;
}
.fade-enter-active, .fade-leave-active {
  transition: opacity 0.3s;
}
.fade-enter-from, .fade-leave-to {
  opacity: 0;
}
</style>
