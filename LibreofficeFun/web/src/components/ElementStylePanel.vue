<template>
  <div class="element-style-panel">
    <div class="style-control-group">
      <el-form-item label="启用特定样式">
        <el-switch v-model="localStyleConfig.enabled" />
      </el-form-item>
    </div>
    
    <div class="style-control-group">
      <el-form-item label="文字颜色">
        <el-color-picker v-model="localStyleConfig.color" show-alpha :disabled="!localStyleConfig.enabled" />
      </el-form-item>
    </div>
    
    <div class="style-control-group">
      <el-form-item label="字体大小">
        <el-input-number 
          v-model="localStyleConfig.fontSize" 
          :min="12" 
          :max="32" 
          :step="1" 
          controls-position="right" 
          size="small" 
          style="width: 120px"
          :disabled="!localStyleConfig.enabled"
        />
      </el-form-item>
    </div>
    
    <div class="style-control-group">
      <el-form-item label="字重">
        <el-select v-model="localStyleConfig.fontWeight" size="small" style="width: 120px" :disabled="!localStyleConfig.enabled">
          <el-option label="正常" value="normal"></el-option>
          <el-option label="粗体" value="bold"></el-option>
          <el-option label="更粗" value="bolder"></el-option>
        </el-select>
      </el-form-item>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, watch } from 'vue'

const props = defineProps({
  styleConfig: {
    type: Object,
    required: true
  }
})

const emit = defineEmits(['update:style'])

// 创建本地副本以避免直接修改props
const localStyleConfig = reactive({
  enabled: props.styleConfig.enabled || false,
  color: props.styleConfig.color || '#333333',
  fontSize: props.styleConfig.fontSize || 14,
  fontWeight: props.styleConfig.fontWeight || 'normal'
})

// 监听本地样式变化并发送更新事件
watch(localStyleConfig, (newVal) => {
  emit('update:style', { ...newVal })
}, { deep: true })

// 监听props变化以同步到本地状态
watch(() => props.styleConfig, (newVal) => {
  if (newVal) {
    localStyleConfig.enabled = newVal.enabled || false
    localStyleConfig.color = newVal.color || '#333333'
    localStyleConfig.fontSize = newVal.fontSize || 14
    localStyleConfig.fontWeight = newVal.fontWeight || 'normal'
  }
}, { deep: true })
</script>

<style scoped>
.element-style-panel {
  margin-top: 10px;
  padding: 10px;
  background-color: #f8f9fa;
  border-radius: 4px;
  border: 1px solid #e1e1e1;
}

.style-control-group {
  margin-bottom: 10px;
}

.style-control-group:last-child {
  margin-bottom: 0;
}

.style-control-group .el-form-item {
  margin-bottom: 0;
}
</style>