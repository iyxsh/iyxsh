<template>
  <div class="simple-card-converter">
    <div class="converter-header">
      <h3>表单转卡片</h3>
      <div class="converter-controls">
        <el-switch v-model="converterSettings.showBorder" active-text="显示边框" inactive-text="隐藏边框"
          @change="updateStyle" />
        <el-switch v-model="converterSettings.showBackground" active-text="显示背景" inactive-text="隐藏背景"
          @change="updateStyle" />
        <span class="control-label">字体大小：</span>
        <el-input-number v-model="converterSettings.fontSize" :min="16" :max="72" size="small" @change="updateStyle" />
        <span class="control-label">背景颜色：</span>
        <el-color-picker v-model="converterSettings.backgroundColor" size="small" @change="updateStyle" show-alpha
          :predefine="predefineColors" />
        <span class="control-label">旋转角度：</span>
        <el-input-number v-model="converterSettings.textRotation" :min="0" :max="360" :step="1" size="small"
          @change="updateStyle" />
        <span class="control-label">间距：</span>
        <el-input-number v-model="converterSettings.spacing" :min="0" :max="50" size="small" @change="updateStyle" />
      </div>
    </div>

    <div class="cards-display-area">
      <div class="card-group" v-for="(group, groupIndex) in cardGroups" :key="groupIndex" :style="getCardGroupStyle()">
        <div class="group-title">{{ group.title || '表单卡片组' }}</div>
        <div class="cards-container">
          <div v-for="(card, cardIndex) in group.cards" :key="cardIndex" class="text-card" :style="getCardStyle(card)"
            @click="editCard(groupIndex, cardIndex)">
            <span class="card-text">{{ card.text }}</span>
          </div>
        </div>
      </div>
    </div>

    <!-- 编辑卡片对话框 -->
    <el-dialog v-model="editDialogVisible" title="编辑卡片" width="300px">
      <el-input v-model="editingCard.text" maxlength="10" show-word-limit />
      <template #footer>
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveCardEdit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, computed, nextTick } from 'vue'
import { ElMessage } from 'element-plus'

// 转换器设置
const converterSettings = reactive({
  showBorder: true,
  showBackground: true,  // 新增背景显示开关
  fontSize: 36,
  spacing: 8,
  textRotation: 0,
  backgroundColor: '#f5f5f5'
})

// 添加预定义颜色
const predefineColors = [
  '#ffffff',
  '#f5f5f5',
  '#e6f7ff',
  '#f0f5ff',
  '#f9f0ff',
  '#fff0f6',
  'rgba(255, 69, 0, 0.68)'
]

// 卡片组数据
const cardGroups = ref([])

// 编辑功能相关
const editDialogVisible = ref(false)
const editingCard = reactive({
  text: '',
  groupIndex: -1,
  cardIndex: -1
})

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

// 获取卡片样式
const getCardStyle = (card) => {
  const baseSize = converterSettings.fontSize
  const style = {
    width: `${baseSize * 1.0}px`,
    height: `${baseSize * 1.0}px`,
    margin: `${converterSettings.spacing}px`,
    fontSize: `${baseSize}px`,
    color: card.color || '#333',
    backgroundColor: converterSettings.showBackground ? converterSettings.backgroundColor : 'transparent',
    transform: `rotate(${converterSettings.textRotation}deg)`,
    border: converterSettings.showBorder ? '1px solid #ddd' : 'none',
    borderRadius: '4px',
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

// 导出组件方法
defineExpose({
  convertFormsToCards
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
</style>