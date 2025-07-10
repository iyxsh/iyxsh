<template>
  <div class="char-cards-container">
    <!-- 内容区域 -->
    <div class="content-area">
      <!-- 设置按钮 -->
      <div class="settings-button-container">
        <el-button 
          type="primary" 
          size="small" 
          icon="Setting" 
          @click="toggleSettings"
        >
          设置
        </el-button>
      </div>
      <!-- 占位区域，可以放置卡片预览或其他内容 -->
      
      <!-- 设置面板 -->
      <div class="settings-panel" :class="{ 'visible': showSettings }">
        <h3 class="settings-title">卡片设置</h3>
        <el-form :inline="true" class="control-panel" size="small">
          <!-- 文字内容输入 -->
          <div class="settings-row">
            <el-form-item label="文字内容" style="width: 100%">
              <el-input
                v-model="textContent"
                placeholder="输入汉字"
                @input="updateCards"
              />
            </el-form-item>
          </div>
          <!-- 卡片边框设置 -->
          <div class="settings-row">
            <el-form-item label="显示卡片边框">
              <el-switch
                v-model="globalSettings.showCardStyle"
                @change="updateCards"
              />
            </el-form-item>
            <el-form-item label="显示组边框">
              <el-switch
                v-model="globalSettings.showGroupBorder"
                @change="updateCards"
              />
            </el-form-item>
            <el-form-item>
              <el-button size="small" type="danger" @click="removeThisCardGroup">删除卡片组</el-button>
            </el-form-item>
          </div>
          <!-- 第一行：基础设置 -->
          <div class="settings-row">
            <el-form-item label="字体">
              <el-select v-model="globalSettings.fontFamily" placeholder="选择字体" style="width: 130px">
                <el-option
                  v-for="(label, value) in fontMap"
                  :key="value"
                  :label="label"
                  :value="value"
                >
                  <span :style="{ fontFamily: value }">{{ label }}</span>
                </el-option>
              </el-select>
            </el-form-item>
            <el-form-item label="字号">
              <el-input-number
                v-model="globalSettings.fontSize"
                :min="12"
                :max="72"
                style="width: 90px"
              />
            </el-form-item>
            <el-form-item label="排列方式">
              <el-radio-group v-model="globalSettings.layoutDirection">
                <el-radio label="horizontal">横排</el-radio>
                <el-radio label="vertical">竖排</el-radio>
              </el-radio-group>
            </el-form-item>
          </div>
          <!-- 第二行：颜色和样式设置 -->
          <div class="settings-row">
            <el-form-item label="文字颜色">
              <el-color-picker v-model="globalSettings.color" size="small" />
            </el-form-item>
            <el-form-item label="背景颜色">
              <el-color-picker v-model="globalSettings.backgroundColor" size="small" />
            </el-form-item>
            <el-form-item label="显示卡片样式">
              <el-switch v-model="globalSettings.showCardStyle" />
            </el-form-item>
          </div>
          <!-- 第三行：尺寸设置 -->
          <div class="settings-row">
            <el-form-item label="卡片间距">
              <el-input-number
                v-model="globalSettings.cardSpacing"
                :min="0"
                :max="50"
                :step="1"
                style="width: 90px"
              />
            </el-form-item>
            <el-form-item label="最小卡片尺寸">
              <el-input-number
                v-model="globalSettings.minCardSize"
                :min="12"
                :max="72"
                style="width: 90px"
              />
            </el-form-item>
          </div>
        </el-form>
      </div>

      <!-- 卡片容器 -->
      <div class="cards-container">
        <draggable 
          v-model="cards" 
          item-key="id"
          :class="['cards-grid', globalSettings.layoutDirection === 'vertical' ? 'vertical-layout' : '']"
          :animation="150"
          ghost-class="ghost-card"
        >
          <template #item="{ element, index }">
            <div 
              class="char-card" 
              :style="getCardStyle(element)"
              :data-line-break="element.lineBreak"
              @click="editCard(index)"
              @dblclick="quickEditCard(index)"
              :key="element.id"
            >
              <span class="char-content" :style="getTextStyle(element)">{{ element.text }}</span>
              <div class="card-controls" v-if="element.isEditing">
                <el-button 
                  type="primary" 
                  size="small" 
                  icon="EditPen"
                  @click.stop="showEditDialog(index)"
                >
                  编辑
                </el-button>
              </div>
            </div>
          </template>
        </draggable>
      </div>
    </div>

    <!-- 编辑对话框 -->
    <el-dialog 
      v-model="editDialogVisible" 
      title="编辑卡片" 
      width="400px"
      destroy-on-close
    >
      <el-form label-width="100px">
        <el-form-item label="文字内容">
          <el-input v-model="editingCard.text" maxlength="1" show-word-limit />
        </el-form-item>
        <el-form-item label="字体">
          <el-select v-model="editingCard.fontFamily" placeholder="选择字体">
            <el-option
              v-for="(label, value) in fontMap"
              :key="value"
              :label="label"
              :value="value"
            >
              <span :style="{ fontFamily: value }">{{ label }} - 示例文字</span>
            </el-option>
          </el-select>
        </el-form-item>
        <el-form-item label="字号">
          <el-input-number 
            v-model="editingCard.fontSize" 
            :min="12" 
            :max="72"
          />
        </el-form-item>
        <el-form-item label="文字颜色">
          <el-color-picker v-model="editingCard.color" />
        </el-form-item>
        <el-form-item label="背景颜色">
          <el-color-picker v-model="editingCard.backgroundColor" />
        </el-form-item>
        <el-form-item label="使用全局样式">
          <el-switch 
            v-model="editingCard.useGlobalStyle" 
            @change="applyGlobalStyle"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="editDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="saveCardEdit">
            确认
          </el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, computed, watch, onMounted, nextTick } from 'vue'
import { EditPen, Setting } from '@element-plus/icons-vue'
import draggable from 'vuedraggable'
import { ElMessage } from 'element-plus'
// 控制设置面板显示
const showSettings = ref(false)

// 文本内容
const textContent = ref('')

// 字体映射对象
const fontMap = {
  'KaiTi': '楷体',
  'SimSun': '宋体',
  'SimHei': '黑体',
  'FangSong': '仿宋',
  'Microsoft YaHei': '微软雅黑',
  'STZhongsong': '华文中宋',
  'STFangsong': '华文仿宋',
  'STKaiti': '华文楷体',
  'STXihei': '华文细黑',
  'STSong': '华文宋体',
  'LiSu': '隶书',
  'YouYuan': '幼圆',
  'STXingkai': '华文行楷',
  'NSimSun': '新宋体',
  'STCaiyun': '华文彩云',
  'Arial': 'Arial',
  'Times New Roman': 'Times New Roman',
  'Courier New': 'Courier New'
}

// 获取字体显示名称
const getFontDisplayName = (fontFamily) => {
  return fontMap[fontFamily] || fontFamily
}

// 全局样式设置
const globalSettings = reactive({
  fontFamily: 'KaiTi',
  fontSize: 36,
  color: '#333333',
  backgroundColor: '#f5f5f5',
  cardSpacing: 10, // 添加卡片间距设置，默认10px
  layoutDirection: 'horizontal', // 新增：排列方式，横排或竖排
  minCardSize: 12,
  showCardStyle: true,
  showGroupBorder: true // 新增：是否显示卡片组边框
})

// 卡片数据
const cards = ref([])

// 从表单加载内容到汉字卡片
const loadFromForms = (forms) => {
  if (!forms || forms.length === 0) {
    ElMessage.warning('表单内容为空')
    return
  }

  // 将表单内容拼接成完整的文本，每个表单属性作为单独一行
  let combinedText = '';

  forms.forEach((form, formIndex) => {
    // 添加标题
    if (form.title && form.showTitle !== false) {
      combinedText += form.title + '\n';
    }

    // 添加值
    if (form.value && form.showValue !== false) {
      combinedText += form.value + '\n';
    }

    // 添加备注
    if (form.remark && form.showRemark !== false) {
      combinedText += form.remark + '\n';
    }

    // 在每个表单之间添加额外的换行（除了最后一个表单）
    if (formIndex < forms.length - 1) {
      combinedText += '\n';
    }
  });

  // 将组合的文本设置到文字内容输入框
  textContent.value = combinedText;

  // 使用 updateCards 函数基于新的文本内容更新卡片
  updateCards();

  ElMessage.success(`已加载 ${forms.length} 个表单的内容到文字内容框`);
}

// 切换设置面板显示状态
const toggleSettings = () => {
  showSettings.value = !showSettings.value;
};



// 导出方法供外部调用

// 定义事件
const emit = defineEmits(['remove-group'])

// 删除当前卡片组
const removeThisCardGroup = () => {
  emit('remove-group');
}

// 快速编辑卡片内容（双击时触发）
const quickEditCard = (index) => {
  try {
    const card = cards.value[index]

    // 显示输入提示框
    const newText = prompt('修改卡片内容', card.text)

    // 如果用户输入了内容（非null且非空）
    if (newText !== null && newText !== '') {
      // 只取第一个字符
      const firstChar = newText.charAt(0)

      // 更新卡片内容
      card.text = firstChar
    }
  } catch (error) {
    console.error('Error in quickEditCard:', error);
  }
}

// 更新卡片
const updateCards = () => {
  const newText = textContent.value
  const existingIds = new Set(cards.value.map(card => card.id))
  const updatedCards = []

  // 处理输入的文本
  if (globalSettings.layoutDirection === 'vertical' && newText.length > 0) {
    // 竖排模式下，创建一个卡片包含所有文本
    const id = `char-group-${Date.now()}`
    updatedCards.push({
      id,
      text: newText, // 整个文本作为一个卡片
      useGlobalStyle: true,
      fontFamily: globalSettings.fontFamily,
      fontSize: globalSettings.fontSize,
      color: globalSettings.color,
      backgroundColor: globalSettings.backgroundColor,
      layoutDirection: 'vertical', // 明确设置为竖排
      isEditing: false
    })
  } else {
    // 横排模式，每个字符一个卡片
    for (let i = 0; i < newText.length; i++) {
      const char = newText[i]
      const id = `char-${i}-${Date.now()}`

      // 创建新卡片
      updatedCards.push({
        id,
        text: char,
        useGlobalStyle: true,
        fontFamily: globalSettings.fontFamily,
        fontSize: globalSettings.fontSize,
        color: globalSettings.color,
        backgroundColor: globalSettings.backgroundColor,
        layoutDirection: 'horizontal', // 明确设置为横排
        isEditing: false
      })
    }
  }

  cards.value = updatedCards
  
  // 更新卡片后重新计算容器尺寸
  nextTick(() => {
    updateContainerSize()
  })
}

// 更新容器尺寸的函数
const updateContainerSize = (layoutDirection, fontSize, card) => {
  try {
    // 获取当前所有卡片
    const cardsCount = cards.value.length || 1
    const spacing = globalSettings.cardSpacing

    // 如果没有传入参数，使用全局设置
    const direction = layoutDirection || globalSettings.layoutDirection
    const size = fontSize || globalSettings.fontSize

    if (direction === 'vertical') {
      // 竖排模式下，容器宽度等于卡片基础宽度
      document.documentElement.style.setProperty('--container-width', `${size}px`)

      // 高度计算
      let totalHeight = 0
      if (card) {
        // 单个卡片高度：文本长度 * 字体大小
        totalHeight = card.text ? card.text.length * size : size
      } else {
        // 所有卡片的累计高度
        totalHeight = cards.value.reduce((sum, c) => {
          // 使用卡片自身的字体大小或默认大小
          const cardFontSize = c.fontSize || size
          return sum + (c.text ? c.text.length * cardFontSize : cardFontSize)
        }, 0)
        // 添加卡片间距
        if (cardsCount > 1) {
          totalHeight += (cardsCount - 1) * spacing
        }
      }
      document.documentElement.style.setProperty('--container-height', `${totalHeight}px`)
    } else {
      // 横排模式下，容器宽度等于所有卡片宽度总和 + 间距
      const totalWidth = size * cardsCount + (cardsCount > 1 ? spacing * (cardsCount - 1) : 0)
      document.documentElement.style.setProperty('--container-width', `${totalWidth}px`)
      // 高度等于卡片基础高度
      document.documentElement.style.setProperty('--container-height', `${size}px`)
    }
  } catch (error) {
    console.warn('计算卡片容器尺寸时出错', error)
  }
}

// 获取卡片样式
const getCardStyle = (card) => {
  // 检查卡片布局方向，优先使用卡片自身的布局方向，如果没有则使用全局设置
  const layoutDirection = card.layoutDirection || globalSettings.layoutDirection
  // 使用1.0比例，使卡片尺寸与字体大小完全一致
  const fontSize = card.fontSize || globalSettings.fontSize
  const style = {}
  
  if (layoutDirection === 'vertical') {
    // 竖排模式
    const textLength = card.text ? card.text.length : 0
    // 根据文本长度动态计算卡片高度，确保至少为字体大小
    const cardHeight = Math.max(fontSize * textLength, fontSize)
    
    Object.assign(style, {
      '--card-size': `${fontSize}px`,
      width: 'var(--card-size)',
      height: `${cardHeight}px`,
      writingMode: 'vertical-rl',
      // 设置为mixed允许文字自然竖排
      textOrientation: 'mixed',
      // 调整文字对齐方式，确保在卡片中居中
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center',
      // 确保竖排时文字顺序正确
      direction: 'rtl' // 修改为rtl，修复卡片内文字顺序问题
    })
  } else {
    // 横排模式 - 保持原有的正方形卡片样式
    Object.assign(style, {
      '--card-size': `${fontSize}px`,
      width: 'var(--card-size)',
      height: 'var(--card-size)',
    })
  }
  
  // 设置卡片组容器的尺寸根据汉字卡片总尺寸的1.0倍比例
  // 这里fontSize就代表了卡片的基础尺寸
  document.documentElement.style.setProperty('--card-base-size', `${fontSize}px`)
  
  updateContainerSize(layoutDirection, fontSize, card)

  // 应用背景色，即使不显示卡片样式也应该有背景色
  style.backgroundColor = card.backgroundColor || globalSettings.backgroundColor
  
  // 只有当显示卡片样式开关打开时才应用边框和阴影样式
  if (globalSettings.showCardStyle) {
    Object.assign(style, {
      border: '1px solid #ddd',
      boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
      borderRadius: '4px'
    })
  }

  // 设置卡片间距
  document.documentElement.style.setProperty('--card-spacing', `${globalSettings.cardSpacing}px`)
  document.documentElement.style.setProperty('--min-card-size', `${globalSettings.minCardSize}px`)
  
  // 根据showGroupBorder设置卡片组边框样式
  try {
    const cardsContainer = document.querySelector('.cards-container')
    if (cardsContainer) {
      if (globalSettings.showGroupBorder) {
        cardsContainer.style.borderColor = '#ddd'
        cardsContainer.style.boxShadow = '0 2px 8px rgba(0,0,0,0.1)'
      } else {
        cardsContainer.style.borderColor = 'transparent'
        cardsContainer.style.boxShadow = 'none'
      }
    }
  } catch (error) {
    console.warn('无法更新卡片组样式，组件可能已卸载', error)
  }

  return style
}

const getTextStyle = (card) => {
  return {
    color: card.color || globalSettings.color,
    fontFamily: card.fontFamily || globalSettings.fontFamily,
    fontSize: `${card.fontSize || globalSettings.fontSize}px`,
    lineHeight: '1',
    margin: '0',
    padding: '0',
    display: 'flex',
    justifyContent: 'center',
    alignItems: 'center',
    width: '100%',
    height: '100%'
  }
}

// 编辑卡片相关
const editingIndex = ref(-1)
const editingCard = reactive({
  text: '',
  fontFamily: '',
  fontSize: 36,
  color: '',
  backgroundColor: '',
  layoutDirection: 'horizontal',
  useGlobalStyle: true
})
const editDialogVisible = ref(false)

// 显示编辑对话框
const showEditDialog = (index) => {
  editingIndex.value = index
  const card = cards.value[index]
  Object.assign(editingCard, {
    text: card.text,
    fontFamily: card.fontFamily,
    fontSize: card.fontSize,
    color: card.color,
    backgroundColor: card.backgroundColor,
    layoutDirection: card.layoutDirection || globalSettings.layoutDirection,
    useGlobalStyle: card.useGlobalStyle
  })
  editDialogVisible.value = true
}

// 应用全局样式
const applyGlobalStyle = () => {
  if (editingCard.useGlobalStyle) {
    editingCard.fontFamily = globalSettings.fontFamily
    editingCard.fontSize = globalSettings.fontSize
    editingCard.color = globalSettings.color
    editingCard.backgroundColor = globalSettings.backgroundColor
    editingCard.layoutDirection = globalSettings.layoutDirection
  }
}

// 编辑卡片
const editCard = (index) => {
  // 重置其他卡片编辑状态
  cards.value.forEach((card, i) => {
    card.isEditing = i === index ? !card.isEditing : false
  })
}

// 保存卡片编辑
const saveCardEdit = () => {
  if (editingIndex.value >= 0 && editingIndex.value < cards.value.length) {
    const card = cards.value[editingIndex.value]
    Object.assign(card, {
      text: editingCard.text,
      fontFamily: editingCard.fontFamily,
      fontSize: editingCard.fontSize,
      color: editingCard.color,
      backgroundColor: editingCard.backgroundColor,
      layoutDirection: editingCard.layoutDirection,
      useGlobalStyle: editingCard.useGlobalStyle
    })
    editDialogVisible.value = false
  }
}

// 监听全局样式变化，自动应用到使用全局样式的卡片
watch(globalSettings, () => {
  cards.value.forEach(card => {
    if (card.useGlobalStyle) {
      card.fontFamily = globalSettings.fontFamily
      card.fontSize = globalSettings.fontSize
      card.color = globalSettings.color
      card.backgroundColor = globalSettings.backgroundColor
      card.layoutDirection = globalSettings.layoutDirection
    }
  })
  
  // 全局样式变化时更新容器尺寸
  nextTick(() => {
    updateContainerSize()
  })
}, { deep: true })

// 更新布局方向
const updateLayoutDirection = (direction) => {
  if (direction === 'horizontal' || direction === 'vertical') {
    globalSettings.layoutDirection = direction
  }
}

// 更新字体大小
const updateFontSize = (size) => {
  if (typeof size === 'number' && size >= 12 && size <= 72) {
    globalSettings.fontSize = size
    
    // 更新所有使用全局样式的卡片
    cards.value.forEach(card => {
      if (card.useGlobalStyle) {
        card.fontSize = globalSettings.fontSize
      }
    })
  }
}

// 组件挂载时初始化卡片
onMounted(() => {
  // 初始化时调用默认内容初始化方法
  initializeDefaultContent()
  
  // 初始化容器尺寸
  nextTick(() => {
    updateContainerSize()
  })
})

// 初始化新卡片的默认内容
const initializeDefaultContent = () => {
  textContent.value = '汉字卡片';

  // 重置为默认全局设置
  Object.assign(globalSettings, {
    fontFamily: 'KaiTi',
    fontSize: 36,
    color: '#333333',
    backgroundColor: '#f5f5f5',
    cardSpacing: 10,
    layoutDirection: 'horizontal',
    minCardSize: 12,
    showCardStyle: true,
    showGroupBorder: true
  });

  // 更新卡片内容
  updateCards();
}

// 刷新现有卡片的内容
const refreshContent = () => {
  // 调用updateCards()即可，它已经包含了所有必要的逻辑
  updateCards();
  console.log('卡片内容已刷新');
}

// 导出方法供外部调用
defineExpose({ 
  loadFromForms,
  toggleSettings,
  updateLayoutDirection,
  updateFontSize,
  initializeDefaultContent,
  refreshContent
})
</script>

<style scoped>
.char-cards-container {
  display: flex;
  flex-direction: column;
  gap: 10px;
  width: 100%;
  padding: 5px;
  position: relative;
}

.content-area {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
  min-height: auto;
}

.text-input-container {
  margin-bottom: 15px;
  z-index: 1;
}

.settings-panel {
  position: fixed;
  top: 20%;
  left: 50%;
  transform: translateX(-50%) translateY(-10px);
  width: 80%;
  max-width: 800px;
  background-color: #f9f9f9;
  border-radius: 8px;
  padding: 15px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  z-index: 100;
  opacity: 0;
  pointer-events: none;
  transition: all 0.3s ease;
  max-height: 0;
  overflow: hidden;
}

.settings-title {
  margin-top: 0;
  margin-bottom: 15px;
  font-size: 16px;
  color: #333;
}

.settings-row {
  display: flex;
  flex-wrap: wrap;
  gap: 15px;
  margin-bottom: 10px;
}

.control-panel {
  display: flex;
  flex-direction: column;
  width: 100%;
}

.settings-panel.visible {
  opacity: 1;
  transform: translateX(-50%) translateY(0);
  pointer-events: auto;
  max-height: 1000px;
  overflow: visible;
}

.cards-container {
  margin-top: 10px;
  position: relative;
  /* 卡片组边框样式，由showGroupBorder控制显示 */
  border: 1px solid transparent;
  border-radius: 8px;
  padding: 10px;
  transition: all 0.3s ease;
  /* 设置容器大小为卡片总尺寸的1.0倍比例 */
  width: var(--container-width, fit-content);
  min-width: calc(var(--card-base-size, 36px) * 1.0);
  height: var(--container-height, auto);
  min-height: calc(var(--card-base-size, 36px) * 1.0);
}

.cards-grid {
  display: flex;
  flex-wrap: wrap;
  gap: var(--card-spacing, 10px);
  width: 100%;
  align-items: center;
  justify-content: flex-start;
}

/* 竖排从右到左的卡片容器 */
.cards-grid.vertical-layout {
  flex-direction: row;
  flex-wrap: nowrap;
  justify-content: flex-start;
  align-items: flex-start;
  height: auto;
  min-height: 300px;
  writing-mode: vertical-rl;
  /* 去掉upright设置，允许汉字自然竖排 */
  text-orientation: mixed;
  direction: ltr; /* 改为从左到右排列，修复顺序问题 */
}

.char-card {
  position: relative;
  min-width: 12px;
  min-height: 12px;
  display: flex;
  justify-content: center;
  align-items: center;
  border-radius: 0;
  cursor: move;
  transition: all 0.3s ease;
  user-select: none;
  padding: 0;
  overflow: hidden;
  background: transparent;
  border: none;
  box-shadow: none;
}

.char-card[data-line-break="true"] {
  width: 100% !important;
  height: 0 !important;
  margin: 0 !important;
  padding: 0 !important;
  flex-basis: 100%;
  border: none !important;
  box-shadow: none !important;
}

.char-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 6px 12px rgba(0, 0, 0, 0.15);
}

.char-content {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  text-align: center;
  word-break: keep-all; /* 防止汉字被打断 */
  white-space: nowrap; /* 防止文字换行 */
  font-size: inherit;
  line-height: normal;
  /* 确保内容继承父元素的writing-mode和text-orientation */
  writing-mode: inherit;
  text-orientation: inherit;
  direction: inherit; /* 确保继承方向设置 */
}

.card-controls {
  position: absolute;
  bottom: -40px;
  left: 50%;
  transform: translateX(-50%);
  display: flex;
  justify-content: center;
  gap: 10px;
  z-index: 10;
  white-space: nowrap;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 4px;
  padding: 2px;
}

.ghost-card {
  opacity: 0.5;
  background: #c8ebfb;
}

.settings-button-container {
  position: absolute;
  top: 10px;
  right: 10px;
  z-index: 101;
}
</style>