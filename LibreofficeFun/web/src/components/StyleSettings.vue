<template>
  <div class="style-settings">
    <div class="setting-group">
      <label>背景颜色：</label>
      <el-color-picker v-model="settings.backgroundColor" size="small" show-alpha :predefine="predefineColors" />
    </div>
    
    <div class="setting-group">
      <label>间距：</label>
      <el-input-number v-model="settings.spacing" :min="0" :max="50" size="small" />
    </div>
    
    <div class="setting-group">
      <label>行距：</label>
      <el-input-number v-model="settings.rowSpacing" :min="0" :max="100" size="small" />
    </div>
  </div>
</template>

<script setup>
const props = defineProps({
  modelValue: {
    type: Object,
    required: true
  },
  predefineColors: {
    type: Array,
    default: () => [
      '#ffffff', '#f5f7fa', '#e4e7ed', 
      '#d4d8dd', '#c0ccda', '#a3b1bf', 
      '#8299ae', '#5f7d8c', '#333333'
    ]
  }
})

const emit = defineEmits(['update:modelValue'])

const settings = props.modelValue
</script>

<style scoped>
.style-settings {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.setting-group {
  display: flex;
  align-items: center;
  gap: 5px;
}

label {
  font-size: 14px;
}
</style>