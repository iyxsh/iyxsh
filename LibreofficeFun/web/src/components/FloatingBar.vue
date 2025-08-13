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
        <!-- 第一行：添加表单、样式开关 -->
        <el-tooltip effect="dark" content="添加表单" placement="top">
          <el-button type="primary" size="small" circle @click="handleAddForm">
            <el-icon>
              <Plus />
            </el-icon>
          </el-button>
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

        <el-tooltip effect="dark" content="下一页" placement="top">
          <el-button type="primary" size="small" circle @click="handleNextPage">
            <el-icon>
              <ArrowRight />
            </el-icon>
          </el-button>
        </el-tooltip>

        <el-tooltip effect="dark" content="切换页面类型" placement="top">
          <el-button type="success" size="small" circle @click="togglePageType">
            <el-icon>
              <Document />
            </el-icon>
          </el-button>
        </el-tooltip>
      </div>
    </transition>
  </div>
</template>

<script>
import { ref, nextTick, computed, onMounted, onBeforeUnmount } from 'vue'
import { Plus, Grid, ArrowLeft, ArrowRight, Document } from '@element-plus/icons-vue'
import { getCurrentInstance } from 'vue'
import { ElMessage } from 'element-plus'
import errorLogService from '../services/errorLogService'

export default {
  name: 'FloatingBar',
  components: {
    Plus,
    Grid,
    ArrowLeft,
    ArrowRight,
    Document
  },
  props: {
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
    'change-page-type'
  ],
  setup(props, { emit }) {
    const visible = ref(false)
    const barStyle = ref({
      left: '20px',
      top: '200px'
    })
    const instance = getCurrentInstance()
    const floatingContent = ref(null)
    let isDragging = false
    let dragStartX, dragStartY
    let initialX, initialY
    let animationFrameId = null

    // 拖动开始
    const startDrag = (e) => {
      // 防止事件冒泡到父级元素
      e.stopPropagation()
      
      // 如果点击的是功能按钮，不进行拖动
      if (e.target.closest('.el-button') || e.target.closest('.home-button-inner')) {
        return
      }
      
      isDragging = true
      dragStartX = e.clientX
      dragStartY = e.clientY
      initialX = parseInt(barStyle.value.left)
      initialY = parseInt(barStyle.value.top)
      
      document.addEventListener('mousemove', drag)
      document.addEventListener('mouseup', stopDrag)
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
      if (animationFrameId) {
        cancelAnimationFrame(animationFrameId)
        animationFrameId = null
      }
    };

    // 获取ApiServiceManager实例
    const getApiService = () => {
      return instance?.appContext.config.globalProperties.$apiService
    }

    // 显示需要解锁页面的提示信息
    const showUnlockMessage = () => {
      ElMessage.warning('请先解锁页面再执行此操作')
    }

    const handleAddForm = async () => {
      // 检查页面是否可编辑
      if (!props.editable) {
        showUnlockMessage()
        return
      }
      // 添加表单空表单不发送后台
      emit('add-form')
    };

    const handleToggleCardStyle = async () => {
      emit('toggle-card-style')
      // 卡片样式切换是本地操作，不需要同步到服务器
    }

    const handlePrevPage = async () => {
      emit('prev-page')
      // 页面切换是本地操作，不需要同步到服务器
    }

    const handleNextPage = async () => {
      emit('next-page')
      // 页面切换是本地操作，不需要同步到服务器
    }

    // 切换页面类型（表单/卡片）
    const togglePageType = async () => {
      // 检查页面是否可编辑
      if (!props.editable) {
        showUnlockMessage()
        return
      }
      
      const newType = props.currentPageType === 'form' ? 'cards' : 'form'
      emit('change-page-type', newType)
      // 页面类型切换是本地操作，不需要同步到服务器
    }

    // 点击外部区域收起面板
    const handleClickOutside = (event) => {
      if (visible.value) {
        // 检查点击的元素是否在浮动内容内部
        if (floatingContent.value && floatingContent.value.contains(event.target)) {
          return
        }
        // 检查点击的元素是否是主按钮
        // 修复：使用更精确的选择器来查找当前组件的主按钮
        const homeButton = instance?.refs?.floatingContent?.parentElement?.querySelector?.('.home-button') || 
                          document.querySelector('.home-button');
        if (homeButton && homeButton.contains(event.target)) {
          return
        }
        visible.value = false
      }
    }

    // 挂载时添加事件监听器
    onMounted(() => {
      document.addEventListener('mousedown', handleClickOutside)
    })

    // 卸载前移除事件监听器
    onBeforeUnmount(() => {
      document.removeEventListener('mousedown', handleClickOutside)
      // 确保清理动画帧
      if (animationFrameId !== null) {
        cancelAnimationFrame(animationFrameId)
      }
    })

    return {
      visible,
      barStyle,
      floatingContent,
      startDrag,
      handleAddForm,
      handleToggleCardStyle,
      handlePrevPage,
      handleNextPage,
      togglePageType
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
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  color: white;
  font-size: 24px;
  user-select: none;
}

.home-button:hover {
  background: rgba(64, 158, 255, 1);
  transform: scale(1.05);
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.2);
}

/* 九宫格功能面板样式 */
.floating-content {
  position: absolute;
  left: 65px;
  top: 50%;
  transform: translateY(-50%);
  background: rgba(255, 255, 255, 0.95);
  border-radius: 16px;
  padding: 16px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.15);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(0, 0, 0, 0.05);
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

/* 滑动淡入动画 */
.slide-fade-enter-active {
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.slide-fade-leave-active {
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.slide-fade-enter-from {
  opacity: 0;
  transform: translateX(-10px) translateY(-50%);
}

.slide-fade-leave-to {
  opacity: 0;
  transform: translateX(-10px) translateY(-50%);
}

/* 响应式设计 */
@media (max-width: 768px) {
  .floating-content {
    left: unset;
    right: 65px;
    top: 50%;
    transform: translateY(-50%);
  }
  
  .slide-fade-enter-from {
    opacity: 0;
    transform: translateX(10px) translateY(-50%);
  }

  .slide-fade-leave-to {
    opacity: 0;
    transform: translateX(10px) translateY(-50%);
  }
}

/* 小屏幕适配 */
@media (max-width: 480px) {
  .home-button {
    width: 48px;
    height: 48px;
    font-size: 20px;
  }
  
  .floating-content {
    padding: 12px;
    gap: 8px;
  }
}
</style>