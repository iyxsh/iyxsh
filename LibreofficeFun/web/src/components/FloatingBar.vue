<template>
  <div class="floating-bar" :style="barStyle" @mousedown="startDrag">
    <transition name="slide-fade">
      <!-- 主按钮 - 类似手机主页按钮 -->
      <div 
        v-if="!visible" 
        class="home-button" 
        @click="visible = true"
      >
        <el-tooltip effect="dark" content="功能菜单" placement="top">
          <div class="home-button-inner">
            <el-icon>
              <Grid />
            </el-icon>
          </div>
        </el-tooltip>
      </div>
      
      <!-- 九宫格功能面板 -->
      <div 
        v-else 
        class="floating-content" 
        ref="floatingContent"
      >
        <!-- 第一行：添加表单、清空当前页、样式开关 -->
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

        <!-- 第二行：上一页、页面类型切换、下一页 -->
        <el-tooltip effect="dark" content="上一页" placement="top">
          <el-button type="primary" size="small" circle @click="handlePrevPage">
            <el-icon>
              <ArrowLeft />
            </el-icon>
          </el-button>
        </el-tooltip>

        <!-- 页面类型切换 -->
        <div class="page-type-toggle">
          <el-tooltip effect="dark" :content="`切换到${currentPageType === 'form' ? '卡片页' : '表单页'}`" placement="top">
            <el-button type="primary" size="small" circle @click="togglePageType">
              <el-icon>
                <component :is="currentPageType === 'form' ? 'Tickets' : 'Document'" />
              </el-icon>
            </el-button>
          </el-tooltip>
        </div>

        <el-tooltip effect="dark" content="下一页" placement="top">
          <el-button type="primary" size="small" circle @click="handleNextPage">
            <el-icon>
              <ArrowRight />
            </el-icon>
          </el-button>
        </el-tooltip>

        <!-- 第三行：编辑页面名称、删除当前页面、切换页面编辑状态 -->
        <el-tooltip effect="dark" content="编辑页面名称" placement="top">
          <el-button 
            type="primary" 
            size="small" 
            circle 
            @click="handleEditPageName(currentPageIdx)"
            :disabled="!pages || pages.length === 0"
          >
            <el-icon>
              <Edit />
            </el-icon>
          </el-button>
        </el-tooltip>

        <el-tooltip effect="dark" content="删除当前页面" placement="top">
          <div>
            <el-popconfirm 
              :title="`确定要删除页面 '${currentPage.name || '新页面'}' 吗？`"
              @confirm="handleDeletePage(currentPageIdx)"
              confirm-button-text="删除" 
              cancel-button-text="取消"
            >
              <template #reference>
                <el-button 
                  type="danger" 
                  size="small" 
                  circle
                  :disabled="!pages || pages.length === 0"
                >
                  <el-icon>
                    <Delete />
                  </el-icon>
                </el-button>
              </template>
            </el-popconfirm>
          </div>
        </el-tooltip>

        <el-tooltip 
          effect="dark" 
          :content="editPageIdx === currentPageIdx ? '锁定页面' : '解锁页面'" 
          placement="top"
        >
          <el-button 
            type="primary" 
            size="small" 
            circle 
            @click="handleToggleEditPage(currentPageIdx)"
            :disabled="!pages || pages.length === 0"
          >
            <el-icon>
              <Lock v-if="editPageIdx !== currentPageIdx" />
              <Unlock v-else />
            </el-icon>
          </el-button>
        </el-tooltip>
      </div>
    </transition>
  </div>
</template>

<script>
import { ref, nextTick, computed, onMounted, onBeforeUnmount } from 'vue'
import { Plus, Delete, Grid, ArrowLeft, ArrowRight, Menu, Document, Edit, Check, Lock, Unlock, Tickets } from '@element-plus/icons-vue'

export default {
  name: 'FloatingBar',
  components: {
    Plus,
    Delete,
    Grid,
    ArrowLeft,
    ArrowRight,
    Menu,
    Document,
    Edit,
    Check,
    Lock,
    Unlock,
    Tickets
  },
  props: {
    clearCurrentPageForms: {
      type: Function,
      required: true
    },
    editable: {
      type: Boolean,
      default: false
    },
    onToggleCardStyle: {
      type: Function,
      required: true
    },
    pages: {
      type: Array,
      default: () => []
    },
    currentPageIdx: {
      type: Number,
      default: 0
    },
    editPageIdx: {
      type: Number,
      default: -1
    },
    editIdxMap: {
      type: Object,
      default: () => ({})
    },
    editName: {
      type: String,
      default: ''
    },
    currentPageType: {
      type: String,
      default: 'form'
    }
  },
  emits: [
    'add-form', 
    'toggle-card-style', 
    'prev-page', 
    'next-page', 
    'clear-current-page',
    'select-page',
    'edit-page-name',
    'save-page-name',
    'delete-page',
    'toggle-edit-page',
    'change-page-type',
    'add-page',
    'update:edit-name'
  ],
  setup(props, { emit }) {
    const visible = ref(false) // 默认隐藏九宫格功能面板
    const barStyle = ref({
      top: '20px',
      left: '20px'
    })
    const editInput = ref(null)
    const floatingContent = ref(null)
    const homeButton = ref(null)
    
    // 计算当前页面
    const currentPage = computed(() => {
      if (props.pages && props.pages.length > 0 && props.currentPageIdx < props.pages.length) {
        return props.pages[props.currentPageIdx] || {}
      }
      return {}
    })

    let isDragging = false
    let dragStartX, dragStartY, initialX, initialY
    let animationFrameId = null // 添加animationFrameId变量定义

    // 点击其他地方收起功能面板
    const handleClickOutside = (event) => {
      if (visible.value) {
        // 检查点击的元素是否在功能面板内
        if (floatingContent.value && floatingContent.value.contains(event.target)) {
          return
        }
        // 检查点击的元素是否是主按钮
        if (homeButton.value && homeButton.value.contains(event.target)) {
          return
        }
        visible.value = false
      }
    }

    // 挂载时添加事件监听器
    onMounted(() => {
      document.addEventListener('mousedown', handleClickOutside)
      // 获取主按钮引用
      homeButton.value = document.querySelector('.home-button')
    })

    // 卸载前移除事件监听器
    onBeforeUnmount(() => {
      document.removeEventListener('mousedown', handleClickOutside)
      // 确保清理动画帧
      if (animationFrameId !== null) {
        cancelAnimationFrame(animationFrameId)
        animationFrameId = null
      }
    })

    const startDrag = (e) => {
      // 只有在点击左键时才开始拖拽
      if (e.button !== 0) return
      
      isDragging = true
      dragStartX = e.clientX
      dragStartY = e.clientY
      initialX = barStyle.value.left ? parseInt(barStyle.value.left) : 20
      initialY = barStyle.value.top ? parseInt(barStyle.value.top) : 20
      
      // 添加鼠标移动和松开事件监听器
      document.addEventListener('mousemove', drag)
      document.addEventListener('mouseup', stopDrag)
      
      // 防止文本选择
      e.preventDefault()
    }

    const drag = (e) => {
      if (!isDragging) return;
      
      // 使用requestAnimationFrame优化拖动性能
      if (animationFrameId) {
        cancelAnimationFrame(animationFrameId)
      }
      
      animationFrameId = requestAnimationFrame(() => {
        const dx = e.clientX - dragStartX
        const dy = e.clientY - dragStartY
        
        barStyle.value.left = `${initialX + dx}px`
        barStyle.value.top = `${initialY + dy}px`
        animationFrameId = null
      })
    };

    const stopDrag = () => {
      isDragging = false
      document.removeEventListener('mousemove', drag)
      document.removeEventListener('mouseup', stopDrag)
      
      // 清理可能存在的动画帧
      if (animationFrameId !== null) {
        cancelAnimationFrame(animationFrameId)
        animationFrameId = null
      }
    };

    const handleAddForm = () => {
      emit('add-form')
      visible.value = false // 执行操作后自动收起面板
    }

    const handleClearCurrentPageForms = () => {
      emit('clear-current-page')
      visible.value = false // 执行操作后自动收起面板
    }

    const handleToggleCardStyle = () => {
      emit('toggle-card-style')
      visible.value = false // 执行操作后自动收起面板
    }

    const handlePrevPage = () => {
      emit('prev-page')
      visible.value = false // 执行操作后自动收起面板
    }

    const handleNextPage = () => {
      emit('next-page')
      visible.value = false // 执行操作后自动收起面板
    }

    // 切换页面类型
    const togglePageType = () => {
      const newType = props.currentPageType === 'form' ? 'cards' : 'form'
      emit('change-page-type', newType)
      // 页面类型切换后不自动收起面板，用户可能还需要其他操作
    }

    // 处理编辑页面名称
    const handleEditPageName = (idx) => {
      emit('edit-page-name', idx)
      nextTick(() => {
        if (editInput.value && editInput.value.focus) {
          editInput.value.focus()
        }
      })
      visible.value = false // 执行操作后自动收起面板
    }

    // 处理删除页面
    const handleDeletePage = (idx) => {
      emit('delete-page', idx)
      // 删除操作后不自动收起面板，因为可能需要确认
    }

    // 处理切换页面编辑状态
    const handleToggleEditPage = (idx) => {
      emit('toggle-edit-page', idx)
      visible.value = false // 执行操作后自动收起面板
    }

    // 处理编辑名称输入
    const handleEditNameInput = (value) => {
      emit('update:edit-name', value)
    }

    return {
      visible,
      barStyle,
      editInput,
      floatingContent,
      currentPage,
      startDrag,
      handleAddForm,
      handleClearCurrentPageForms,
      handleToggleCardStyle,
      handlePrevPage,
      handleNextPage,
      togglePageType,
      handleEditPageName,
      handleDeletePage,
      handleToggleEditPage,
      handleEditNameInput
    }
  }
}
</script>
<style scoped>
.floating-bar {
  position: fixed;
  z-index: 2000;
  background: transparent;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  left: v-bind('barStyle.left');
  top: v-bind('barStyle.top');
  will-change: transform; /* 提升动画性能 */
}

/* 主按钮样式 - 类似手机主页按钮 */
.home-button {
  width: 56px;
  height: 56px;
  border-radius: 50%;
  background: rgba(64, 158, 255, 0.9);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.3s ease;
  backdrop-filter: blur(8px);
  border: 1px solid rgba(255, 255, 255, 0.3);
}

.home-button:hover {
  background: rgba(64, 158, 255, 1);
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.2);
  transform: scale(1.05);
}

.home-button-inner {
  color: white;
  font-size: 24px;
}

/* 九宫格功能面板样式 */
.floating-content {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-template-rows: repeat(3, 1fr);
  grid-gap: 12px;
  background: rgba(255, 255, 255, 0.95);
  border: 1px solid rgba(0, 0, 0, 0.08);
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.15);
  border-radius: 24px;
  backdrop-filter: blur(12px);
  padding: 16px;
  position: relative;
  transform-origin: top left;
  width: 200px;
  height: 200px;
}

.slide-fade-enter-active {
  transition: all 0.3s ease-out;
}

.slide-fade-leave-active {
  transition: all 0.3s cubic-bezier(1, 0.5, 0.8, 1);
}

.slide-fade-enter-from,
.slide-fade-leave-to {
  transform: scale(0.8);
  opacity: 0;
}

.page-type-toggle {
  display: flex;
  justify-content: center;
  align-items: center;
}

.page-type-toggle :deep(.el-button) {
  background: #67c23a;
  border-color: #67c23a;
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
}

.page-type-toggle :deep(.el-button:hover) {
  background: #85ce61;
  border-color: #85ce61;
}

.page-type-toggle :deep(.el-button i) {
  font-size: 18px;
}

:deep(.el-button:disabled) {
  opacity: 0.5;
  cursor: not-allowed;
}

:deep(.el-button) {
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
  margin: 0 auto;
}

:deep(.el-button i) {
  font-size: 18px;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .home-button {
    width: 48px;
    height: 48px;
  }
  
  .home-button-inner {
    font-size: 20px;
  }
  
  .floating-content {
    grid-gap: 8px;
    padding: 12px;
    width: 180px;
    height: 180px;
  }
  
  :deep(.el-button) {
    width: 40px;
    height: 40px;
  }
  
  :deep(.el-button i) {
    font-size: 16px;
  }
}

@media (max-width: 480px) {
  .floating-content {
    grid-gap: 6px;
    padding: 10px;
    width: 160px;
    height: 160px;
  }
  
  :deep(.el-button) {
    width: 36px;
    height: 36px;
  }
  
  :deep(.el-button i) {
    font-size: 14px;
  }
}
</style>