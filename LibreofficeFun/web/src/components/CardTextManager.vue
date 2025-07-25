<template>
  <div class="card-text-manager">
    <StyleManager
      v-model="localSettings"
      :predefine-colors="predefineColors"
      @change="onSettingsChange"
      @apply="onSettingsApply"
    />
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted } from 'vue'
import StyleManager from './StyleManager.vue'

// 定义props
const props = defineProps({
  modelValue: {
    type: Object,
    default: () => ({})
  },
  predefineColors: {
    type: Array,
    default: () => [
      '#ffffff', '#000000', '#ff0000', '#00ff00', '#0000ff',
      '#ffff00', '#00ffff', '#ff00ff', '#f5f5f5', '#e0e0e0'
    ]
  }
})

// 定义emits
const emit = defineEmits(['update:modelValue', 'change', 'apply'])

// 本地设置
const localSettings = ref({})

// 初始化数据
onMounted(() => {
  // 合并传入的设置和默认设置
  localSettings.value = { ...props.modelValue }
})

// 监听modelValue变化
watch(() => props.modelValue, (newVal) => {
  if (newVal) {
    localSettings.value = { ...newVal }
  }
}, { deep: true })

// 设置变化处理
const onSettingsChange = (settings) => {
  emit('update:modelValue', { ...settings })
  emit('change', { ...settings })
}

// 设置应用处理
const onSettingsApply = (settings) => {
  emit('apply', { ...settings })
}

// 暴露方法
defineExpose({
  getStyle: () => {
    const style = {}
    
    // 字体样式
    if (localSettings.value.fontSize) {
      style.fontSize = `${localSettings.value.fontSize}px`
    }
    
    if (localSettings.value.color) {
      style.color = localSettings.value.color
    }
    
    if (localSettings.value.fontWeight && localSettings.value.fontWeight !== 'normal') {
      style.fontWeight = localSettings.value.fontWeight
    }
    
    if (localSettings.value.fontStyle && localSettings.value.fontStyle !== 'normal') {
      style.fontStyle = localSettings.value.fontStyle
    }
    
    // 背景样式
    if (localSettings.value.showBackground) {
      style.backgroundColor = localSettings.value.backgroundColor || 'transparent'
    } else {
      style.backgroundColor = 'transparent'
    }
    
    // 布局样式
    if (localSettings.value.textAlign) {
      style.textAlign = localSettings.value.textAlign
    }
    
    if (localSettings.value.lineHeight) {
      style.lineHeight = localSettings.value.lineHeight
    }
    
    // 盒模型样式
    if (localSettings.value.padding !== undefined) {
      style.padding = `${localSettings.value.padding}px`
    }
    
    if (localSettings.value.margin !== undefined) {
      style.margin = `${localSettings.value.margin}px`
    }
    
    // 边框样式
    if (localSettings.value.borderStyle && localSettings.value.borderStyle !== 'none') {
      style.borderStyle = localSettings.value.borderStyle
      style.borderWidth = `${localSettings.value.borderWidth || 1}px`
      style.borderColor = localSettings.value.borderColor || '#000000'
    } else {
      style.border = 'none'
    }
    
    if (localSettings.value.borderRadius) {
      style.borderRadius = `${localSettings.value.borderRadius}px`
    }
    
    // 高级样式
    if (localSettings.value.opacity !== undefined && localSettings.value.opacity !== 1) {
      style.opacity = localSettings.value.opacity
    }
    
    if (localSettings.value.boxShadow) {
      style.boxShadow = localSettings.value.boxShadow
    }
    
    if (localSettings.value.transform) {
      style.transform = localSettings.value.transform
    }
    
    return style
  }
})
</script>

<style scoped>
.card-text-manager {
  padding: 10px 0;
}
</style>