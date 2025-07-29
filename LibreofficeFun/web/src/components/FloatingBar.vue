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
          <div>
            <el-popconfirm title="确定要清空当前页面吗？" @confirm="handleClearCurrentPageForms" confirm-button-type="danger">
              <template #reference>
                <el-button type="danger" size="small" circle>
                  <el-icon>
                    <Delete />
                  </el-icon>
                </el-button>
              </template>
            </el-popconfirm>
          </div>
        </el-tooltip>

        <el-tooltip effect="dark" content="样式开关" placement="top">
          <el-button type="primary" size="small" circle @click="handleToggleCardStyle">
            <el-icon>
              <Grid />
            </el-icon>
          </el-button>
        </el-tooltip>

        <el-tooltip effect="dark" content="上一页" placement="top">
          <el-button type="primary" size="small" circle @click="handlePrevPage">
            <el-icon>
              <ArrowLeft />
            </el-icon>
          </el-button>
        </el-tooltip>

        <el-tooltip effect="dark" content="下一页" placement="top">
          <el-button type="primary" size="small" circle @click="handleNextPage">
            <el-icon>
              <ArrowRight />
            </el-icon>
          </el-button>
        </el-tooltip>
      </div>
    </transition>

    <div class="toggle-btn" @click="visible = !visible">
      <el-icon>
        <ArrowLeft v-if="visible" />
        <ArrowRight v-else />
      </el-icon>
    </div>
  </div>
</template>

<script>
import { ref } from 'vue'
import { Plus, Delete, Grid, ArrowLeft, ArrowRight } from '@element-plus/icons-vue'

export default {
  name: 'FloatingBar',
  components: {
    Plus,
    Delete,
    Grid,
    ArrowLeft,
    ArrowRight
  },
  emits: ['add-form', 'toggle-card-style', 'prev-page', 'next-page', 'clear-current-page'],
  setup(props, { emit }) {
    const visible = ref(true)
    const barStyle = ref({
      top: '20px',
      left: '20px'
    })

    let isDragging = false
    let dragStartX, dragStartY, initialX, initialY

    const startDrag = (e) => {
      isDragging = true
      dragStartX = e.clientX
      dragStartY = e.clientY
      initialX = barStyle.value.left ? parseInt(barStyle.value.left) : 20
      initialY = barStyle.value.top ? parseInt(barStyle.value.top) : 20
      
      document.addEventListener('mousemove', drag)
      document.addEventListener('mouseup', stopDrag)
    }

    const drag = (e) => {
      if (!isDragging) return
      
      const dx = e.clientX - dragStartX
      const dy = e.clientY - dragStartY
      
      barStyle.value.left = `${initialX + dx}px`
      barStyle.value.top = `${initialY + dy}px`
    }

    const stopDrag = () => {
      isDragging = false
      document.removeEventListener('mousemove', drag)
      document.removeEventListener('mouseup', stopDrag)
    }

    const handleAddForm = () => {
      emit('add-form')
    }

    const handleClearCurrentPageForms = () => {
      emit('clear-current-page')
    }

    const handleToggleCardStyle = () => {
      emit('toggle-card-style')
    }

    const handlePrevPage = () => {
      emit('prev-page')
    }

    const handleNextPage = () => {
      emit('next-page')
    }

    return {
      visible,
      barStyle,
      startDrag,
      handleAddForm,
      handleClearCurrentPageForms,
      handleToggleCardStyle,
      handlePrevPage,
      handleNextPage
    }
  }
}
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
  left: v-bind('barStyle.left');
  top: v-bind('barStyle.top');
  width: auto;
  min-width: 32px;
  max-width: 400px;
  height: 32px;
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
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
  
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
  .toggle-button {
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