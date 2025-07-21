<template>
  <div 
    ref="containerRef"
    class="draggable-list"
    :class="{ 'dragging': isDragging }"
  >
    <transition-group 
      tag="div" 
      name="list-move"
      @before-enter="beforeEnter"
      @enter="enter"
      @leave="leave"
    >
      <div
        v-for="(item, index) in modelValue"
        :key="item.id || item.key || index"
        class="draggable-item"
        :class="{
          'active': activeIndex === index,
          'dragging': draggedIndex === index
        }"
        :style="getItemStyle(index)"
        @mousedown="startDrag($event, index)"
        @touchstart="startDrag($event, index)"
      >
        <div class="draggable-item-content">
          <div v-if="showDragHandle" class="drag-handle">
            <slot name="handle">
              <div class="default-handle">
                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <line x1="3" y1="12" x2="21" y2="12"></line>
                  <line x1="3" y1="6" x2="21" y2="6"></line>
                  <line x1="3" y1="18" x2="21" y2="18"></line>
                </svg>
              </div>
            </slot>
          </div>

          <slot 
            :item="item" 
            :index="index" 
            :is-active="activeIndex === index"
          ></slot>

          <div v-if="showActions" class="item-actions">
            <slot 
              name="actions" 
              :item="item" 
              :index="index"
              :remove="() => removeItem(index)"
            >
              <button 
                v-if="allowRemove" 
                class="remove-btn" 
                @click.stop="removeItem(index)"
                aria-label="Remove item"
              >
                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <line x1="18" y1="6" x2="6" y2="18"></line>
                  <line x1="6" y1="6" x2="18" y2="18"></line>
                </svg>
              </button>
            </slot>
          </div>
        </div>
      </div>
    </transition-group>

    <!-- 添加项目按钮 -->
    <div v-if="allowAdd && showAddButton" class="add-item-wrapper">
      <slot name="add-button">
        <button class="add-item-btn" @click="addItem">
          <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <line x1="12" y1="5" x2="12" y2="19"></line>
            <line x1="5" y1="12" x2="19" y2="12"></line>
          </svg>
          <span>{{ addButtonText }}</span>
        </button>
      </slot>
    </div>
  </div>
</template>

<script>
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { generateId } from '../utils/helpers'

export default {
  name: 'DraggableList',
  props: {
    modelValue: {
      type: Array,
      required: true
    },
    disabled: {
      type: Boolean,
      default: false
    },
    allowRemove: {
      type: Boolean,
      default: true
    },
    allowAdd: {
      type: Boolean,
      default: true
    },
    itemHeight: {
      type: Number,
      default: null
    },
    showDragHandle: {
      type: Boolean,
      default: true
    },
    showActions: {
      type: Boolean,
      default: true
    },
    showAddButton: {
      type: Boolean,
      default: true
    },
    addButtonText: {
      type: String,
      default: '添加项目'
    },
    defaultNewItem: {
      type: [Object, Function],
      default: () => ({})
    }
  },
  emits: ['update:modelValue', 'item-added', 'item-removed', 'reorder', 'item-click'],
  setup(props, { emit }) {
    const containerRef = ref(null)
    const isDragging = ref(false)
    const draggedIndex = ref(-1)
    const activeIndex = ref(-1)
    const startY = ref(0)
    const currentY = ref(0)

    // 创建新项目
    const createNewItem = () => {
      const newItem = typeof props.defaultNewItem === 'function'
        ? props.defaultNewItem()
        : { ...props.defaultNewItem }

      // 确保项目有唯一ID
      if (!newItem.id) {
        newItem.id = generateId()
      }

      return newItem
    }

    // 添加项目
    const addItem = () => {
      if (props.disabled || !props.allowAdd) return

      const newItem = createNewItem()
      const newList = [...props.modelValue, newItem]

      emit('update:modelValue', newList)
      emit('item-added', newItem, newList.length - 1)

      // 激活新添加的项目
      activeIndex.value = newList.length - 1
    }

    // 移除项目
    const removeItem = (index) => {
      if (props.disabled || !props.allowRemove) return

      const item = props.modelValue[index]
      const newList = [...props.modelValue]
      newList.splice(index, 1)

      emit('update:modelValue', newList)
      emit('item-removed', item, index)

      // 更新激活项目
      if (activeIndex.value === index) {
        activeIndex.value = Math.min(index, newList.length - 1)
      } else if (activeIndex.value > index) {
        activeIndex.value--
      }
    }

    // 开始拖动
    const startDrag = (event, index) => {
      if (props.disabled) return

      // 只有通过拖动手柄拖动 或 全局允许拖动
      if (props.showDragHandle) {
        // 检查是否点击在拖动手柄上
        const target = event.target
        const isDragHandle = target.closest('.drag-handle')
        if (!isDragHandle) return
      }

      // 阻止默认行为和冒泡
      event.preventDefault()

      // 设置拖动状态
      isDragging.value = true
      draggedIndex.value = index
      activeIndex.value = index

      // 记录起始位置
      startY.value = event.clientY || (event.touches && event.touches[0].clientY) || 0
      currentY.value = startY.value

      // 添加事件监听
      window.addEventListener('mousemove', onDrag)
      window.addEventListener('touchmove', onDrag)
      window.addEventListener('mouseup', stopDrag)
      window.addEventListener('touchend', stopDrag)

      // 添加拖动中的全局样式
      document.body.classList.add('dragging-active')
    }

    // 拖动中
    const onDrag = (event) => {
      if (!isDragging.value) return

      const clientY = event.clientY || (event.touches && event.touches[0].clientY) || 0
      currentY.value = clientY

      // 计算移动距离
      const deltaY = currentY.value - startY.value

      // 根据移动距离计算目标索引
      const itemHeight = getActualItemHeight()
      if (itemHeight <= 0) return

      const moveCount = Math.round(deltaY / itemHeight)
      const targetIndex = Math.max(0, Math.min(props.modelValue.length - 1, draggedIndex.value + moveCount))

      // 如果目标索引不同，重新排序列表
      if (targetIndex !== draggedIndex.value) {
        const newList = [...props.modelValue]
        const [removed] = newList.splice(draggedIndex.value, 1)
        newList.splice(targetIndex, 0, removed)

        emit('update:modelValue', newList)
        emit('reorder', {
          item: removed,
          oldIndex: draggedIndex.value,
          newIndex: targetIndex
        })

        // 更新拖动索引和起始位置
        draggedIndex.value = targetIndex
        startY.value = currentY.value
      }
    }

    // 停止拖动
    const stopDrag = () => {
      isDragging.value = false

      // 移除事件监听
      window.removeEventListener('mousemove', onDrag)
      window.removeEventListener('touchmove', onDrag)
      window.removeEventListener('mouseup', stopDrag)
      window.removeEventListener('touchend', stopDrag)

      // 移除全局样式
      document.body.classList.remove('dragging-active')

      // 短暂延迟后重置拖动索引，以便动画完成
      setTimeout(() => {
        draggedIndex.value = -1
      }, 200)
    }

    // 获取实际的项目高度
    const getActualItemHeight = () => {
      if (props.itemHeight) return props.itemHeight

      // 动态计算项目高度
      const container = containerRef.value
      if (!container) return 0

      const items = container.querySelectorAll('.draggable-item')
      if (items.length === 0) return 0

      // 使用第一个项目的高度
      return items[0].offsetHeight
    }

    // 获取项目样式
    const getItemStyle = (index) => {
      if (!isDragging.value || draggedIndex.value !== index) {
        return {}
      }

      // 拖动中的项目样式
      return {
        transform: `translateY(${currentY.value - startY.value}px)`,
        zIndex: 10,
        opacity: 0.8
      }
    }

    // 动画相关方法
    const beforeEnter = (el) => {
      el.style.opacity = 0
      el.style.height = '0'
    }

    const enter = (el, done) => {
      const height = el.scrollHeight

      // 使用 requestAnimationFrame 确保过渡效果正常
      requestAnimationFrame(() => {
        el.style.opacity = 1
        el.style.height = `${height}px`

        // 监听过渡完成
        el.addEventListener('transitionend', done, { once: true })
      })
    }

    const leave = (el, done) => {
      el.style.opacity = 0
      el.style.height = '0'

      // 监听过渡完成
      el.addEventListener('transitionend', done, { once: true })
    }

    // 选择项目
    const selectItem = (index) => {
      if (props.disabled) return

      activeIndex.value = index
      emit('item-click', props.modelValue[index], index)
    }

    // 生命周期钩子
    onMounted(() => {
      // 初始化容器宽度等
    })

    onBeforeUnmount(() => {
      // 清理事件监听
      window.removeEventListener('mousemove', onDrag)
      window.removeEventListener('touchmove', onDrag)
      window.removeEventListener('mouseup', stopDrag)
      window.removeEventListener('touchend', stopDrag)

      document.body.classList.remove('dragging-active')
    })

    return {
      containerRef,
      isDragging,
      draggedIndex,
      activeIndex,
      startDrag,
      addItem,
      removeItem,
      selectItem,
      getItemStyle,
      beforeEnter,
      enter,
      leave
    }
  }
}
</script>

<style scoped>
.draggable-list {
  width: 100%;
  position: relative;
}

.draggable-item {
  padding: 8px;
  margin-bottom: 8px;
  background-color: white;
  border-radius: 4px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  transition: transform 0.2s ease, opacity 0.2s ease, box-shadow 0.2s ease, height 0.3s ease;
  cursor: pointer;
  position: relative;
  overflow: hidden;
  will-change: transform, opacity;
}

.draggable-item:hover {
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.15);
}

.draggable-item.active {
  box-shadow: 0 0 0 2px var(--primary-color, #42b883), 0 2px 6px rgba(0, 0, 0, 0.1);
}

.draggable-item.dragging {
  z-index: 10;
  opacity: 0.8;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

.draggable-item-content {
  display: flex;
  align-items: center;
  width: 100%;
}

.drag-handle {
  cursor: grab;
  margin-right: 8px;
  color: #999;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.drag-handle:hover {
  color: #666;
}

.default-handle {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border-radius: 4px;
}

.item-actions {
  margin-left: auto;
  display: flex;
  align-items: center;
  flex-shrink: 0;
}

.remove-btn {
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  color: #999;
  cursor: pointer;
  border-radius: 4px;
  transition: background-color 0.2s ease, color 0.2s ease;
}

.remove-btn:hover {
  background-color: #f8f8f8;
  color: #f56c6c;
}

.add-item-wrapper {
  margin-top: 8px;
}

.add-item-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: 8px 16px;
  background-color: #f8f8f8;
  border: 1px dashed #dcdfe6;
  border-radius: 4px;
  color: #606266;
  cursor: pointer;
  transition: all 0.3s;
}

.add-item-btn:hover {
  background-color: #f2f6fc;
  border-color: var(--primary-color, #42b883);
  color: var(--primary-color, #42b883);
}

.add-item-btn svg {
  margin-right: 4px;
}

/* 动画 */
.list-move {
  transition: transform 0.3s ease;
}

.list-enter-active,
.list-leave-active {
  transition: opacity 0.3s ease, height 0.3s ease;
  overflow: hidden;
}

.list-enter-from,
.list-leave-to {
  opacity: 0;
  height: 0 !important;
}
</style>
