<template>
  <div class="toolbar" :style="toolbarStyle" @mousedown.self="startDrag">
    <!-- 新增按钮提示 -->
    <el-tooltip effect="dark" content="新建页面" placement="top">
      <el-button type="primary" circle @click="onAddPage">
        <el-icon><DocumentAdd /></el-icon>
      </el-button>
    </el-tooltip>
    
    <el-tooltip effect="dark" content="清空所有页面" placement="top">
      <el-popconfirm title="确定要清空所有页面吗？" @confirm="clearAllPages">
        <template #reference>
          <el-button type="danger" circle>
            <el-icon><Delete /></el-icon>
          </el-button>
        </template>
      </el-popconfirm>
    </el-tooltip>

    <el-tooltip effect="dark" content="旋转当前页面" placement="top" v-if="showRotateButton">
      <el-button type="warning" circle @click="rotatePage">
        <el-icon class="rotate-icon"><Refresh /></el-icon>
      </el-button>
    </el-tooltip>
  </div>
</template>

<script setup>
import { DocumentAdd, Delete, Refresh } from '@element-plus/icons-vue'
import { ref, reactive } from 'vue'

const props = defineProps({
  onAddPage: Function,
  clearAllPages: Function,
  rotatePage: Function,
  showRotateButton: Boolean
})

// 拖拽逻辑
const toolbarStyle = reactive({ x: 0, y: 0, transform: 'translate(0,0)' })
let dragging = false, startX = 0, startY = 0

const startDrag = (e) => {
  dragging = true
  startX = e.clientX - toolbarStyle.x
  startY = e.clientY - toolbarStyle.y
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
}

const onDrag = (e) => {
  if (!dragging) return
  toolbarStyle.x = e.clientX - startX
  toolbarStyle.y = e.clientY - startY
  toolbarStyle.transform = `translate(${toolbarStyle.x}px, ${toolbarStyle.y}px)`
}

const stopDrag = () => {
  dragging = false
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
}
</script>

<style scoped>
.toolbar {
  position: fixed;
  top: 20px;
  left: 20px;
  display: flex;
  gap: 8px;
  padding: 8px;
  background: rgba(255,255,255,0.95);
  border-radius: 24px;
  box-shadow: 0 4px 12px rgba(0,0,0,0.12);
  backdrop-filter: blur(8px);
  z-index: 2000;
  cursor: move;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  
  &:hover {
    box-shadow: 0 6px 16px rgba(0,0,0,0.16);
  }
}

.el-button {
  --size: 36px;
  width: var(--size);
  height: var(--size);
  transition: transform 0.2s ease;
  
  &:hover {
    transform: scale(1.1);
  }
}

.rotate-icon {
  transition: transform 0.3s ease;
}

.toolbar:hover .rotate-icon {
  transform: rotate(90deg);
}
</style>
