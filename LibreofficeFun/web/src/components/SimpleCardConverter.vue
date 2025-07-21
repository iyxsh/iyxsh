<template>
  <div class="simple-card-converter">
    <div class="converter-header">
      <h3>表单转卡片</h3>
      <div class="converter-controls">
        <!-- 添加空值检查 -->
        <el-switch 
          v-if="converterSettings" 
          v-model="converterSettings.showBorder" 
          active-text="显示边框" 
          inactive-text="隐藏边框" 
          @change="updateStyle" 
        />
        <!-- 添加空值检查 -->
        <el-switch 
          v-if="converterSettings" 
          v-model="converterSettings.showBackground" 
          active-text="显示背景" 
          inactive-text="隐藏背景" 
          @change="updateStyle" 
        />
        
        <!-- 添加空值检查 -->
        <StyleSettings
          v-if="converterSettings"
          v-model="converterSettings"
          :predefine-colors="predefineColors"
          @update:model-value="updateStyle"
        />
        
        <!-- 卡片样式选择器 -->
        <span class="control-label">卡片样式：</span>
        <el-select v-model="selectedCardStyle" placeholder="选择卡片样式" size="small">
          <el-option
            v-for="option in cardStyleOptions"
            :key="option.value"
            :label="option.label"
            :value="option.value"
          />
        </el-select>
      </div>
    </div>

    <div class="cards-display-area">
      <div class="card-group" v-for="(group, groupIndex) in cardGroups" :key="groupIndex" :style="getCardGroupStyle()">
        <div class="group-title">{{ group.title || '表单卡片组' }}</div>
        <div class="cards-container">
          <div v-for="(card, cardIndex) in group.cards" :key="cardIndex" class="text-card" :style="getCardStyle(card)"
            @click="editCard(groupIndex, cardIndex)">
            <span class="card-text">{{ card?.text || '示例' }}</span>
          </div>
        </div>
      </div>
    </div>

    <!-- 编辑卡片对话框 -->
    <el-dialog v-model="editDialogVisible" title="编辑卡片" width="300px">
      <!-- 添加空值检查和默认值 -->
      <el-input 
        v-model="editingCard.text"
        maxlength="10"
        show-word-limit
        :value="editingCard.text || ''"
        @input="value => editingCard.text = value"
      />
    </el-dialog>
  </div>
</template>
<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, watch, nextTick } from 'vue';
import StyleSettings from './StyleSettings.vue';
import { useEventBus } from '@/utils/eventBus'; // 使用别名路径提高可维护性

// 创建事件总线实例
const { on, off, emit } = useEventBus()

// 添加组件加载调试信息
console.log('SimpleCardConverter component initializing');

// 在组件挂载时注册事件
onMounted(() => {
  // 确保 converterSettings 已初始化
  if (!converterSettingsRef.value) {
    converterSettingsRef.value = {
      showBorder: true,
      showBackground: true,
      fontSize: 24,
      padding: 16,
      margin: 8,
      spacing: 8,
      textRotation: 0,
      backgroundColor: '#ffffff',
      textColor: '#000000',
      borderStyle: 'none',
      borderRadius: 0,
      boxShadow: 'none',
      gradient: 'none'
    }
  }
  
  // 监听页面尺寸变化事件
  on('page-size-changed', handlePageSizeChange)
  
  console.log('SimpleCardConverter component mounted', {
    cardGroups: cardGroups.value,
    selectedCardStyle: selectedCardStyle.value
  });
})

// 在组件卸载前执行清理
onBeforeUnmount(() => {
  // 执行资源清理
  cleanupResources()
  
  // 移除事件监听
  off('page-size-changed', handlePageSizeChange)
  
  console.log('SimpleCardConverter component about to be unmounted');
})

// 添加资源清理
const cleanupTasks = []

// 添加内存泄漏检查
function checkMemoryUsage() {
  if (performance.memory) {
    const memoryUsage = performance.memory
    console.log('SimpleCardConverter Memory Usage:', {
      usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
      totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
      jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
    })
    
    // 如果内存使用超过阈值，触发警告
    if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
      console.warn('SimpleCardConverter: Memory usage is high, consider optimizing')
    }
  }
}

// 格式化字节大小
function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes'
  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

// 添加预定义颜色
const predefineColors = [
  '#ffffff',
  '#f5f5f5',
  '#e6f7ff',
  '#f0f0ff',
  '#f9f0ff',
  '#fff0f6',
  'rgba(255, 69, 0, 0.68)'
]

// 添加卡片样式选项
const cardStyleOptions = [
  { value: 'default', label: '默认样式' },
  { value: 'shadow', label: '带阴影' },
  { value: 'rounded', label: '圆角卡片' },
  { value: 'outlined', label: '线框样式' },
  { value: 'elevation', label: '立体效果' },
  { value: 'gradient', label: '渐变背景' }
]

// 当前选中的卡片样式
const selectedCardStyle = ref('default')

// 转换器设置
const converterSettings = reactive({
  showBorder: true,
  showBackground: true,  // 新增背景显示开关
  fontSize: 24,
  padding: 16,
  margin: 8,
  spacing: 8,
  textRotation: 0,
  backgroundColor: '#ffffff',
  textColor: '#000000',
  borderStyle: 'none',
  borderRadius: 0,
  boxShadow: 'none',
  gradient: 'none'
})

// 添加 converterSettings 的响应式引用
const converterSettingsRef = ref(converterSettings)

// 在组件挂载时确保 converterSettings 已初始化
onMounted(() => {
  // 确保 converterSettings 已初始化
  if (!converterSettingsRef.value) {
    converterSettingsRef.value = {
      showBorder: true,
      showBackground: true,
      fontSize: 24,
      padding: 16,
      margin: 8,
      spacing: 8,
      textRotation: 0,
      backgroundColor: '#ffffff',
      textColor: '#000000',
      borderStyle: 'none',
      borderRadius: 0,
      boxShadow: 'none',
      gradient: 'none'
    }
  }
  
  // 监听页面尺寸变化事件
  on('page-size-changed', handlePageSizeChange)
  
  console.log('SimpleCardConverter component mounted', {
    cardGroups: cardGroups.value,
    selectedCardStyle: selectedCardStyle.value
  });
})

// 根据选中的卡片样式更新转换设置
watch(selectedCardStyle, (newStyle) => {
  switch (newStyle) {
    case 'default':
      Object.assign(converterSettings, {
        padding: 16,
        margin: 8,
        backgroundColor: '#ffffff',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 0,
        boxShadow: 'none',
        gradient: 'none'
      })
      break
    case 'shadow':
      Object.assign(converterSettings, {
        padding: 16,
        margin: 8,
        backgroundColor: '#ffffff',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 0,
        boxShadow: '0 2px 8px rgba(0, 0, 0, 0.1)',
        gradient: 'none'
      })
      break
    case 'rounded':
      Object.assign(converterSettings, {
        padding: 16,
        margin: 8,
        backgroundColor: '#ffffff',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 8,
        boxShadow: 'none',
        gradient: 'none'
      })
      break
    case 'outlined':
      Object.assign(converterSettings, {
        padding: 16,
        margin: 8,
        backgroundColor: 'transparent',
        textColor: '#333333',
        borderStyle: '1px solid #cccccc',
        borderRadius: 4,
        boxShadow: 'none',
        gradient: 'none'
      })
      break
    case 'elevation':
      Object.assign(converterSettings, {
        padding: 16,
        margin: 8,
        backgroundColor: '#f8f8f8',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 4,
        boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
        gradient: 'none'
      })
      break
    case 'gradient':
      Object.assign(converterSettings, {
        padding: 16,
        margin: 8,
        backgroundColor: 'linear-gradient(135deg, #ffffff, #f0f0f0)',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 0,
        boxShadow: 'none',
        gradient: 'linear-gradient(135deg, #ffffff, #f0f0f0)'
      })
      break
    default:
      break
  }
  
  // 触发样式更新
  updateStyle()
})

// 卡片组数据
const cardGroups = ref([])

// 编辑功能相关
const editDialogVisible = ref(false)
const editingCard = reactive({
  text: '',
  groupIndex: -1,
  cardIndex: -1
})

// 添加页面尺寸处理方法
const setPageSize = (size) => {
  if (size) {
    converterSettings.fontSize = Math.min(size.width, size.height) * 0.8
    // 通知其他组件页面尺寸已更改
    emit('page-size-changed', size)
  }
}

// 添加 handlePageSizeChange 方法
const handlePageSizeChange = (size) => {
  if (size && size.width && size.height) {
    // 根据页面尺寸调整字体大小
    const baseSize = Math.min(size.width, size.height)
    converterSettings.fontSize = baseSize * 0.8
    
    // 更新样式
    updateStyle()
    
    console.log('Page size changed:', size)
  } else {
    console.warn('Invalid page size data received')
  }
}

// 转换表单为卡片
const convertFormsToCards = (forms) => {
  if (!forms || !forms.length) {
    ElMessage.warning('没有可转换的表单数据')
    return
  }

  const newGroups = forms.map(form => {
    // 从表单提取文本内容
    const texts = []

    if (form.title) texts.push(form.title)
    if (form.value) texts.push(form.value)
    if (form.remark) texts.push(form.remark)

    // 合并并清理文本
    const combinedText = texts.join('\n').replace(/\s+/g, ' ').trim()

    // 创建卡片
    const cards = combinedText.length > 0
      ? combinedText.split('').map(char => ({
        text: char,
        color: '#333',
        backgroundColor: converterSettings.backgroundColor
      }))
      : [{ text: '示例', color: '#333', backgroundColor: converterSettings.backgroundColor }]
    
    // 确保每个卡片都有文本
    cards.forEach(card => {
      card.text = card.text ?? '示例'
    })

    return {
      cards
    }
  })

  cardGroups.value = newGroups  // 确保正确赋值卡片组数据
  nextTick(() => {
    updateStyle()
    ElMessage.success(`已将 ${forms.length} 个表单转换为卡片组`)
  })
}

// 更新卡片样式
const updateStyle = () => {
  // 这个函数被调用时会触发重新渲染
  // 由于使用了响应式数据，Vue会自动应用最新样式
  cardGroups.value = [...cardGroups.value] // 强制触发重新渲染
  nextTick(() => {
    ElMessage.success('样式已更新')
  })
}

// 获取卡片组样式
const getCardGroupStyle = () => {
  return {
    marginBottom: '20px',
    border: converterSettings.showBorder ? '1px solid #ddd' : 'none',
    borderRadius: '4px',
    padding: '10px',
    backgroundColor: '#fff'
  }
}

// 修改 getCardStyle 方法，添加空值检查
const getCardStyle = (card) => {
  // 添加空值检查
  if (!card) {
    return {}
  }
  
  const baseSize = converterSettings.fontSize
  const style = {
    width: `${baseSize * 1.0}px`,
    height: `${baseSize * 1.0}px`,
    margin: `${converterSettings.margin}px`,
    padding: `${converterSettings.padding}px`,
    fontSize: `${baseSize}px`,
    color: converterSettings.textColor,
    backgroundColor: converterSettings.showBackground 
      ? converterSettings.gradient === 'none' 
        ? converterSettings.backgroundColor 
        : converterSettings.gradient
      : 'transparent',
    transform: `rotate(${converterSettings.textRotation}deg)`,
    border: converterSettings.borderStyle,
    borderRadius: `${converterSettings.borderRadius}px`,
    boxShadow: converterSettings.boxShadow,
    display: 'flex',
    justifyContent: 'center',
    alignItems: 'center',
    transition: 'all 0.2s ease',
    cursor: 'pointer'
  }

  return style
}

// 编辑卡片
const editCard = (groupIndex, cardIndex) => {
  const card = cardGroups.value[groupIndex]?.cards[cardIndex]
  if (card) {
    editingCard.text = card.text
    editingCard.groupIndex = groupIndex
    editingCard.cardIndex = cardIndex
    editDialogVisible.value = true
  }
}

// 保存卡片编辑
const saveCardEdit = () => {
  const { groupIndex, cardIndex, text } = editingCard
  if (groupIndex >= 0 && cardIndex >= 0 && cardGroups.value[groupIndex]?.cards[cardIndex]) {
    cardGroups.value[groupIndex].cards[cardIndex].text = text
    editDialogVisible.value = false
    ElMessage.success('卡片内容已更新')
  }
}

// 添加内存泄漏检查
function setupMemoryLeakCheck() {
  // 检查内存使用情况
  const memoryCheckInterval = setInterval(() => {
    if (performance.memory) {
      const memoryUsage = performance.memory
      console.log('SimpleCardConverter Memory Usage:', {
        usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
        totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
        jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
      })
      
      // 如果内存使用超过阈值，触发警告
      if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
        console.warn('SimpleCardConverter: Memory usage is high, consider optimizing')
      }
    }
  }, 5 * 60 * 1000)
  
  // 添加清理任务
  cleanupTasks.push(() => clearInterval(memoryCheckInterval))
}

// 添加资源清理
function cleanupResources() {
  // 清理内存检查
  cleanupTasks.forEach(task => task())
}

// 使用 import 替代 require
import { defineExpose } from 'vue'

// 暴露方法
defineExpose({
  convertFormsToCards,
  setPageSize
})
</script>


<style scoped>
.control-label {
  margin-right: 8px;
  font-size: 14px;
  color: #606266;
}

.simple-card-converter {
  width: 100%;
  padding: 15px;
  background-color: #f9f9f9;
  min-height: 300px;
}

.converter-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
  padding: 10px 0;
  background-color: #fff;
  border-radius: 4px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.1);
}

.converter-controls {
  display: flex;
  gap: 15px;
  align-items: center;
  flex-wrap: wrap;
}

.cards-display-area {
  width: 100%;
}

.card-group {
  background-color: #fff;
  margin-bottom: 20px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.1);
  border-radius: 4px;
  overflow: hidden;
}

.group-title {
  font-weight: bold;
  margin-bottom: 10px;
  padding: 8px;
  background-color: #f2f6fc;
  border-bottom: 1px solid #eaeaea;
}

.cards-container {
  display: flex;
  flex-wrap: wrap;
  align-items: flex-start;
  padding: 0;
  margin: calc(-1 * v-bind('converterSettings.spacing + "px"'));
}

.cards-container.vertical-layout {
  flex-direction: column;
  height: auto;
  flex-wrap: wrap;
  max-height: 600px;
}

.text-card {
  box-shadow: none !important;
  border: none !important;
}

.text-card:hover {
  transform: scale(1.1);
  box-shadow: 0 4px 12px 0 rgba(0, 0, 0, 0.15);
  z-index: 10;
}

.card-text {
  line-height: 1;
  user-select: none;
  transform-origin: center;
}

.vertical-layout .card-text {
  writing-mode: vertical-rl;
  text-orientation: mixed;
}

/* 卡片样式选择器样式 */
.card-style-selector {
  margin: 16px 0;
  display: flex;
  justify-content: center;
}

.card-style-selector .el-select {
  width: 200px;
}

/* 基础卡片样式 */
.card-item {
  box-sizing: border-box;
  transition: all 0.3s ease;
  will-change: transform, opacity;
}

/* 默认样式 */
.card-item.default-style {
  padding: 16px;
  margin: 8px;
  background-color: #ffffff;
  color: #000000;
}

/* 带阴影样式 */
.card-item.shadow-style {
  padding: 16px;
  margin: 8px;
  background-color: #ffffff;
  color: #000000;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

/* 圆角卡片样式 */
.card-item.rounded-style {
  padding: 16px;
  margin: 8px;
  background-color: #ffffff;
  color: #000000;
  border-radius: 8px;
}

/* 线框样式 */
.card-item.outlined-style {
  padding: 16px;
  margin: 8px;
  background-color: transparent;
  color: #333333;
  border: 1px solid #cccccc;
  border-radius: 4px;
}

/* 立体效果 */
.card-item.elevation-style {
  padding: 16px;
  margin: 8px;
  background-color: #f8f8f8;
  color: #000000;
  border-radius: 4px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

/* 渐变背景 */
.card-item.gradient-style {
  padding: 16px;
  margin: 8px;
  background: linear-gradient(135deg, #ffffff, #f0f0f0);
  color: #000000;
}
</style>