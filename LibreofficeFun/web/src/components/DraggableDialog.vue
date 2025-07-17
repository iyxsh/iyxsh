
<template>
  <div 
    v-show="visible" 
    class="draggable-dialog" 
    :style="dialogStyle"
    :class="{ 'is-dragging': isDragging }"
  >
    <!-- 标题栏/拖动区域 -->
    <div class="dialog-header" @mousedown="startDrag">
      <div class="dialog-title">{{ title }}</div>
      <button class="close-btn" @click="close">×</button>
    </div>

    <!-- 内容区域 -->
    <div class="dialog-content">
      <slot></slot>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';

const props = defineProps({
  title: {
    type: String,
    default: '设置'
  },
  visible: {
    type: Boolean,
    default: false
  },
  initialPosition: {
    type: Object,
    default: () => ({ x: 100, y: 100 })
  },
  zIndex: {
    type: Number,
    default: 1000
  }
});

const emit = defineEmits(['update:visible', 'close']);

// 弹窗位置
const position = ref({
  x: props.initialPosition.x,
  y: props.initialPosition.y
});

// 拖拽状态
const isDragging = ref(false);
const dragOffset = ref({ x: 0, y: 0 });

// 计算样式
const dialogStyle = computed(() => ({
  left: `${position.value.x}px`,
  top: `${position.value.y}px`,
  zIndex: props.zIndex
}));

// 开始拖拽
const startDrag = (event) => {
  isDragging.value = true;
  dragOffset.value = {
    x: event.clientX - position.value.x,
    y: event.clientY - position.value.y
  };
  document.addEventListener('mousemove', onDrag);
  document.addEventListener('mouseup', stopDrag);
};

// 拖拽中
const onDrag = (event) => {
  if (isDragging.value) {
    position.value = {
      x: event.clientX - dragOffset.value.x,
      y: event.clientY - dragOffset.value.y
    };
  }
};

// 结束拖拽
const stopDrag = () => {
  isDragging.value = false;
  document.removeEventListener('mousemove', onDrag);
  document.removeEventListener('mouseup', stopDrag);
};

// 关闭弹窗
const close = () => {
  emit('update:visible', false);
  emit('close');
};

// 组件卸载前清理
onBeforeUnmount(() => {
  document.removeEventListener('mousemove', onDrag);
  document.removeEventListener('mouseup', stopDrag);
});

// 暴露给父组件的方法
defineExpose({
  close
});
</script>

<style scoped>
.draggable-dialog {
  position: fixed;
  background-color: #fff;
  border-radius: 6px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  min-width: 320px;
  max-width: 90vw;
  max-height: 80vh;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  transition: box-shadow 0.3s;
}

.draggable-dialog.is-dragging {
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.2);
  opacity: 0.95;
}

.dialog-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 16px;
  background-color: #f5f7fa;
  cursor: move;
  user-select: none;
  border-bottom: 1px solid #ebeef5;
}

.dialog-title {
  font-weight: 500;
  font-size: 16px;
  color: #303133;
}

.close-btn {
  border: none;
  background: none;
  font-size: 20px;
  color: #909399;
  cursor: pointer;
  transition: color 0.2s;
  padding: 0 4px;
}

.close-btn:hover {
  color: #f56c6c;
}

.dialog-content {
  padding: 20px;
  overflow-y: auto;
  max-height: calc(80vh - 50px);
}
</style>
