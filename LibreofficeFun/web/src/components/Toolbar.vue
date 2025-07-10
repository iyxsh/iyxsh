<template>
  <div class="toolbar">
    <el-button type="primary" @click="onAddPage">New Page</el-button>
    <el-button type="danger" @click="clearAllPages">Clear All Pages</el-button>
    <el-button type="warning" @click="rotatePage" v-if="showRotateButton">
      <el-icon class="rotate-icon"><Refresh /></el-icon>
      旋转页面
    </el-button>
  </div>
</template>
<script setup>
import { Refresh } from '@element-plus/icons-vue'

const props = defineProps({
  onAddPage: Function,
  clearAllPages: Function,
  rotatePage: Function,
  showRotateButton: {
    type: Boolean,
    default: false
  }
})
</script>
<style scoped>
.toolbar {
  display: flex;
  gap: 12px;
  padding: 12px 16px;
  background: #fff;
  border-bottom: 1px solid #eee;
  position: sticky;
  top: 0;
  z-index: 10;
}

.rotate-icon {
  margin-right: 5px;
  transition: transform 0.3s ease;
}

.toolbar button:hover .rotate-icon {
  transform: rotate(45deg);
}
</style>
