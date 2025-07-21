<template>
  <div 
    ref="containerRef" 
    class="virtual-scroller"
    @scroll="onScroll"
    :style="{ height: height ? `${height}px` : '100%' }"
  >
    <div 
      class="virtual-scroller-content"
      :style="{ height: `${totalHeight}px` }"
    >
      <div 
        class="virtual-scroller-items"
        :style="{ transform: `translateY(${offsetY}px)` }"
      >
        <slot 
          v-for="item in visibleItems" 
          :key="item.id || item.key || item.index" 
          :item="item"
          :index="item.index"
        ></slot>
      </div>
    </div>

    <!-- 滚动到顶部按钮 -->
    <div 
      v-if="showScrollTop && scrollTop > 200" 
      class="scroll-to-top-btn"
      @click="scrollToTop"
    >
      <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <polyline points="18 15 12 9 6 15"></polyline>
      </svg>
    </div>
  </div>
</template>

<script>
import { ref, computed, onMounted, onBeforeUnmount, watch } from 'vue'
import { debounce } from 'lodash-es'

export default {
  name: 'VirtualScroller',
  props: {
    // 数据项数组
    items: {
      type: Array,
      required: true
    },
    // 每个项目的高度(px)
    itemHeight: {
      type: Number,
      default: 50
    },
    // 容器高度(px)，不设置则为100%
    height: {
      type: Number,
      default: null
    },
    // 缓冲区项目数量（可视区域上下各预渲染多少项）
    buffer: {
      type: Number,
      default: 5
    },
    // 是否显示滚动到顶部按钮
    showScrollTop: {
      type: Boolean,
      default: true
    }
  },
  emits: ['update:scrollTop', 'scroll-end', 'visible-items-change'],
  setup(props, { emit }) {
    // DOM引用
    const containerRef = ref(null)

    // 滚动状态
    const scrollTop = ref(0)
    const clientHeight = ref(0)
    const isScrolling = ref(false)

    // 计算总高度
    const totalHeight = computed(() => 
      props.items.length * props.itemHeight
    )

    // 计算可见范围的起始索引
    const startIndex = computed(() => {
      const index = Math.floor(scrollTop.value / props.itemHeight) - props.buffer
      return Math.max(0, index)
    })

    // 计算可见范围的结束索引
    const endIndex = computed(() => {
      const visibleCount = Math.ceil(clientHeight.value / props.itemHeight)
      const index = Math.floor(scrollTop.value / props.itemHeight) + visibleCount + props.buffer
      return Math.min(props.items.length - 1, index)
    })

    // 计算Y轴偏移量
    const offsetY = computed(() => 
      startIndex.value * props.itemHeight
    )

    // 计算可见项目
    const visibleItems = computed(() => {
      return props.items
        .slice(startIndex.value, endIndex.value + 1)
        .map((item, idx) => ({
          ...item,
          index: startIndex.value + idx
        }))
    })

    // 处理滚动事件
    const onScroll = (event) => {
      scrollTop.value = event.target.scrollTop
      emit('update:scrollTop', scrollTop.value)

      // 标记正在滚动
      isScrolling.value = true

      // 滚动结束时触发事件
      debouncedHandleScrollEnd()
    }

    // 防抖处理滚动结束
    const debouncedHandleScrollEnd = debounce(() => {
      isScrolling.value = false
      emit('scroll-end', {
        scrollTop: scrollTop.value,
        startIndex: startIndex.value,
        endIndex: endIndex.value
      })
    }, 200)

    // 滚动到顶部
    const scrollToTop = () => {
      if (!containerRef.value) return

      containerRef.value.scrollTo({
        top: 0,
        behavior: 'smooth'
      })
    }

    // 滚动到指定索引
    const scrollToIndex = (index, behavior = 'auto') => {
      if (!containerRef.value) return

      const top = index * props.itemHeight
      containerRef.value.scrollTo({
        top,
        behavior
      })
    }

    // 更新容器高度
    const updateClientHeight = () => {
      if (!containerRef.value) return
      clientHeight.value = containerRef.value.clientHeight
    }

    // 监听可见项目变化
    watch(visibleItems, (newItems) => {
      emit('visible-items-change', newItems)
    })

    // 监听容器尺寸变化
    let resizeObserver = null

    onMounted(() => {
      // 初始化客户端高度
      updateClientHeight()

      // 监听容器大小变化
      if (window.ResizeObserver) {
        resizeObserver = new ResizeObserver(updateClientHeight)
        if (containerRef.value) {
          resizeObserver.observe(containerRef.value)
        }
      } else {
        // 回退方案
        window.addEventListener('resize', updateClientHeight)
      }
    })

    onBeforeUnmount(() => {
      // 清理资源
      if (resizeObserver) {
        resizeObserver.disconnect()
      } else {
        window.removeEventListener('resize', updateClientHeight)
      }
    })

    return {
      containerRef,
      scrollTop,
      clientHeight,
      totalHeight,
      offsetY,
      visibleItems,
      onScroll,
      scrollToTop,
      scrollToIndex
    }
  }
}
</script>

<style scoped>
.virtual-scroller {
  position: relative;
  overflow-y: auto;
  overflow-x: hidden;
  width: 100%;
  will-change: transform;
  -webkit-overflow-scrolling: touch;
}

.virtual-scroller-content {
  position: relative;
  width: 100%;
}

.virtual-scroller-items {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  will-change: transform;
}

.scroll-to-top-btn {
  position: fixed;
  bottom: 20px;
  right: 20px;
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background-color: var(--primary-color, #42b883);
  color: white;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
  transition: transform 0.3s ease, background-color 0.3s ease;
  z-index: 100;
}

.scroll-to-top-btn:hover {
  transform: translateY(-3px);
  background-color: var(--primary-color-hover, #369767);
}
</style>
