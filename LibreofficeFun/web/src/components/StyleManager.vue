<template>
  <div class="style-manager">
    <div class="style-manager-header">
      <h3>样式管理器</h3>
      <el-switch 
        v-model="showAdvanced" 
        active-text="高级设置" 
        size="small"
        @change="onAdvancedToggle"
      />
    </div>
    
    <div class="style-manager-content">
      <!-- 常用设置 -->
      <div class="settings-section">
        <h4>常用设置</h4>
        <div class="settings-grid">
          <div class="setting-item">
            <label>字体大小</label>
            <el-input-number 
              v-model="localStyle.fontSize" 
              :min="1" 
              :max="100" 
              size="small"
              @change="updateStyle"
            />
            <span class="unit">px</span>
          </div>
          
          <div class="setting-item">
            <label>字体颜色</label>
            <el-color-picker 
              v-model="localStyle.color" 
              size="small" 
              show-alpha
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>背景颜色</label>
            <el-color-picker 
              v-model="localStyle.backgroundColor" 
              size="small" 
              show-alpha
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>边框</label>
            <el-select 
              v-model="localStyle.borderStyle" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="无" value="none" />
              <el-option label="实线" value="solid" />
              <el-option label="虚线" value="dashed" />
              <el-option label="点线" value="dotted" />
            </el-select>
          </div>
          
          <div class="setting-item" v-if="localStyle.borderStyle !== 'none'">
            <label>边框宽度</label>
            <el-input-number 
              v-model="localStyle.borderWidth" 
              :min="0" 
              :max="20" 
              size="small"
              @change="updateStyle"
            />
            <span class="unit">px</span>
          </div>
          
          <div class="setting-item" v-if="localStyle.borderStyle !== 'none'">
            <label>边框颜色</label>
            <el-color-picker 
              v-model="localStyle.borderColor" 
              size="small" 
              show-alpha
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>圆角</label>
            <el-input-number 
              v-model="localStyle.borderRadius" 
              :min="0" 
              :max="50" 
              size="small"
              @change="updateStyle"
            />
            <span class="unit">px</span>
          </div>
          
          <div class="setting-item">
            <label>内边距</label>
            <el-input-number 
              v-model="localStyle.padding" 
              :min="0" 
              :max="100" 
              size="small"
              @change="updateStyle"
            />
            <span class="unit">px</span>
          </div>
          
          <div class="setting-item">
            <label>外边距</label>
            <el-input-number 
              v-model="localStyle.margin" 
              :min="0" 
              :max="100" 
              size="small"
              @change="updateStyle"
            />
            <span class="unit">px</span>
          </div>
        </div>
      </div>
      
      <!-- 高级设置 -->
      <div v-if="showAdvanced" class="settings-section advanced-section">
        <h4>高级设置</h4>
        <div class="settings-grid">
          <div class="setting-item">
            <label>字体粗细</label>
            <el-select 
              v-model="localStyle.fontWeight" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="正常" value="normal" />
              <el-option label="粗体" value="bold" />
              <el-option label="100" value="100" />
              <el-option label="200" value="200" />
              <el-option label="300" value="300" />
              <el-option label="400" value="400" />
              <el-option label="500" value="500" />
              <el-option label="600" value="600" />
              <el-option label="700" value="700" />
              <el-option label="800" value="800" />
              <el-option label="900" value="900" />
            </el-select>
          </div>
          
          <div class="setting-item">
            <label>字体样式</label>
            <el-select 
              v-model="localStyle.fontStyle" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="正常" value="normal" />
              <el-option label="斜体" value="italic" />
              <el-option label="倾斜" value="oblique" />
            </el-select>
          </div>
          
          <div class="setting-item">
            <label>文本对齐</label>
            <el-select 
              v-model="localStyle.textAlign" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="左对齐" value="left" />
              <el-option label="居中" value="center" />
              <el-option label="右对齐" value="right" />
              <el-option label="两端对齐" value="justify" />
            </el-select>
          </div>
          
          <div class="setting-item">
            <label>行高</label>
            <el-input-number 
              v-model="localStyle.lineHeight" 
              :min="0" 
              :max="5" 
              :step="0.1"
              size="small"
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>宽度</label>
            <el-input 
              v-model="localStyle.width" 
              size="small"
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>高度</label>
            <el-input 
              v-model="localStyle.height" 
              size="small"
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>显示类型</label>
            <el-select 
              v-model="localStyle.display" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="默认" value="" />
              <el-option label="块级" value="block" />
              <el-option label="行内" value="inline" />
              <el-option label="行内块" value="inline-block" />
              <el-option label="弹性" value="flex" />
              <el-option label="网格" value="grid" />
              <el-option label="隐藏" value="none" />
            </el-select>
          </div>
          
          <div class="setting-item">
            <label>定位</label>
            <el-select 
              v-model="localStyle.position" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="静态" value="static" />
              <el-option label="相对" value="relative" />
              <el-option label="绝对" value="absolute" />
              <el-option label="固定" value="fixed" />
              <el-option label="粘性" value="sticky" />
            </el-select>
          </div>
          
          <div class="setting-item" v-if="localStyle.position !== 'static'">
            <label>左边距</label>
            <el-input 
              v-model="localStyle.left" 
              size="small"
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item" v-if="localStyle.position !== 'static'">
            <label>上边距</label>
            <el-input 
              v-model="localStyle.top" 
              size="small"
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>透明度</label>
            <el-slider 
              v-model="localStyle.opacity" 
              :min="0" 
              :max="1" 
              :step="0.01"
              size="small"
              @change="updateStyle"
            />
            <span class="value-display">{{ localStyle.opacity }}</span>
          </div>
          
          <div class="setting-item">
            <label>阴影</label>
            <el-input 
              v-model="localStyle.boxShadow" 
              size="small"
              placeholder="例: 0 2px 4px rgba(0,0,0,0.1)"
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>变换</label>
            <el-input 
              v-model="localStyle.transform" 
              size="small"
              placeholder="例: rotate(45deg) scale(1.2)"
              @change="updateStyle"
            />
          </div>
        </div>
      </div>
    </div>
    
    <div class="style-manager-footer">
      <el-button @click="resetStyle" size="small">重置</el-button>
      <el-button type="primary" @click="applyStyle" size="small">应用</el-button>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted } from 'vue'

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

// 数据
const showAdvanced = ref(false)
const localStyle = reactive({
  // 常用设置
  fontSize: 16,
  color: '#000000',
  backgroundColor: '#ffffff',
  borderStyle: 'none',
  borderWidth: 1,
  borderColor: '#000000',
  borderRadius: 0,
  padding: 0,
  margin: 0,
  
  // 高级设置
  fontWeight: 'normal',
  fontStyle: 'normal',
  textAlign: 'left',
  lineHeight: 1.2,
  width: '',
  height: '',
  display: '',
  position: 'static',
  left: '',
  top: '',
  opacity: 1,
  boxShadow: '',
  transform: ''
})

// 初始化数据
onMounted(() => {
  // 合并传入的样式和默认样式
  Object.assign(localStyle, props.modelValue)
})

// 监听modelValue变化
watch(() => props.modelValue, (newVal) => {
  if (newVal) {
    Object.assign(localStyle, newVal)
  }
}, { deep: true })

// 更新样式
const updateStyle = () => {
  // 发出更新事件
  emit('update:modelValue', { ...localStyle })
  emit('change', { ...localStyle })
}

// 应用样式
const applyStyle = () => {
  emit('apply', { ...localStyle })
}

// 重置样式
const resetStyle = () => {
  Object.assign(localStyle, {
    fontSize: 16,
    color: '#000000',
    backgroundColor: '#ffffff',
    borderStyle: 'none',
    borderWidth: 1,
    borderColor: '#000000',
    borderRadius: 0,
    padding: 0,
    margin: 0,
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'left',
    lineHeight: 1.2,
    width: '',
    height: '',
    display: '',
    position: 'static',
    left: '',
    top: '',
    opacity: 1,
    boxShadow: '',
    transform: ''
  })
  updateStyle()
}

// 高级设置切换
const onAdvancedToggle = () => {
  // 切换时触发更新
  updateStyle()
}

// 转换为CSS对象
const toCSS = () => {
  const css = {}
  
  // 常用设置
  if (localStyle.fontSize) css['font-size'] = `${localStyle.fontSize}px`
  if (localStyle.color) css['color'] = localStyle.color
  if (localStyle.backgroundColor) css['background-color'] = localStyle.backgroundColor
  
  // 边框设置
  if (localStyle.borderStyle && localStyle.borderStyle !== 'none') {
    css['border-width'] = `${localStyle.borderWidth}px`
    css['border-style'] = localStyle.borderStyle
    css['border-color'] = localStyle.borderColor
  } else {
    css['border'] = 'none'
  }
  
  if (localStyle.borderRadius) css['border-radius'] = `${localStyle.borderRadius}px`
  if (localStyle.padding !== undefined) css['padding'] = `${localStyle.padding}px`
  if (localStyle.margin !== undefined) css['margin'] = `${localStyle.margin}px`
  
  // 高级设置
  if (localStyle.fontWeight && localStyle.fontWeight !== 'normal') css['font-weight'] = localStyle.fontWeight
  if (localStyle.fontStyle && localStyle.fontStyle !== 'normal') css['font-style'] = localStyle.fontStyle
  if (localStyle.textAlign && localStyle.textAlign !== 'left') css['text-align'] = localStyle.textAlign
  if (localStyle.lineHeight && localStyle.lineHeight !== 1.2) css['line-height'] = localStyle.lineHeight
  if (localStyle.width) css['width'] = localStyle.width
  if (localStyle.height) css['height'] = localStyle.height
  if (localStyle.display) css['display'] = localStyle.display
  if (localStyle.position && localStyle.position !== 'static') {
    css['position'] = localStyle.position
    if (localStyle.left) css['left'] = localStyle.left
    if (localStyle.top) css['top'] = localStyle.top
  }
  if (localStyle.opacity !== 1) css['opacity'] = localStyle.opacity
  if (localStyle.boxShadow) css['box-shadow'] = localStyle.boxShadow
  if (localStyle.transform) css['transform'] = localStyle.transform
  
  return css
}

// 暴露方法
defineExpose({
  toCSS
})
</script>

<style scoped>
.style-manager {
  background-color: #fff;
  border-radius: 8px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  padding: 16px;
  max-width: 100%;
}

.style-manager-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.style-manager-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
}

.settings-section {
  margin-bottom: 20px;
}

.settings-section h4 {
  margin: 0 0 12px 0;
  font-size: 14px;
  font-weight: 500;
  color: #333;
  border-bottom: 1px solid #eee;
  padding-bottom: 6px;
}

.settings-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
  gap: 12px;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.setting-item label {
  font-size: 12px;
  color: #666;
}

.setting-item :deep(.el-input__wrapper),
.setting-item :deep(.el-input-number) {
  width: 100%;
}

.unit,
.value-display {
  font-size: 12px;
  color: #999;
  margin-left: 4px;
}

.style-manager-footer {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  margin-top: 16px;
  padding-top: 16px;
  border-top: 1px solid #eee;
}
</style>