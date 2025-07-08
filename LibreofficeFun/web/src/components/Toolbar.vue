<template>
  <div class="toolbar">
    <el-button type="primary" @click="onAddPage">New Page</el-button>
    <el-button type="danger" @click="clearAllPages">Clear All Pages</el-button>
  </div>
</template>
<script setup>
const props = defineProps({
  onAddPage: Function,
  clearAllPages: Function
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
</style>
