<template>
  <div class="card-manager-container">
    <div class="controls-section">
      <div class="left-controls">
        <el-button type="primary" @click="addNewCardGroup">添加新卡片组</el-button>
        <el-button type="success" @click="saveAllCards">保存所有卡片</el-button>
        <el-button type="warning" @click="exportAllCards">导出所有卡片</el-button>
      </div>
      <div class="right-controls">
        <el-switch
          v-model="managerSettings.showBorder"
          active-text="显示边框"
          inactive-text="隐藏边框"
          @change="updateManagerStyle"
          class="style-switch"
        />
        <el-switch
          v-model="managerSettings.showBackground"
          active-text="显示背景"
          inactive-text="隐藏背景"
          @change="updateManagerStyle"
          class="style-switch"
        />
        <el-radio-group v-model="managerSettings.layoutDirection" @change="updateCardLayoutDirection">
          <el-radio-button label="horizontal">横排</el-radio-button>
          <el-radio-button label="vertical">竖排</el-radio-button>
        </el-radio-group>
        <el-tooltip content="卡片组间距">
          <div class="spacing-control">
            <span>间距:</span>
            <el-input-number
              v-model="managerSettings.gridGap"
              :min="0"
              :max="50"
              :step="5"
              size="small"
              @change="updateGridLayout"
            />
          </div>
        </el-tooltip>
      </div>
    </div>

    <div class="card-groups-container" :class="{ 'vertical-layout': managerSettings.layoutDirection === 'vertical' }" :style="gridLayoutStyle">
      <div v-for="(group, index) in cardGroups" :key="group.id" class="card-group">
        <div class="settings-button-container">
          <el-button
            class="card-settings-button"
            size="small"
            type="primary"
            icon="Setting"
            circle
            @click="openCardSettings(index)"
          ></el-button>
        </div>
        <ChineseCharCard 
          ref="cardRefs"
          :groupId="group.id"
          @cards-updated="handleCardsUpdated(group.id, $event)"
          @remove-group="removeCardGroup(index)"
        />
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed,onMounted, reactive, watch } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import ChineseCharCard from './chinesecharcard.vue'

// 卡片管理器全局设置
const managerSettings = reactive({
  showBorder: true, // 是否显示卡片管理器边框
  showBackground: true, // 是否显示卡片管理器背景
  fontSize: 36, // 默认字体大小，与卡片保持一致
  layoutDirection: 'horizontal', // 默认布局方向：横排
  gridGap: 15, // 网格间距
  cardGroupWidth: 300, // 卡片组默认宽度
  columnsCount: 0 // 自动计算列数，0表示自动
})

// 卡片组列表
const cardGroups = ref([])
const cardRefs = ref([])

// 控制设置按钮可见性的数组
const visibleSettingsButtons = ref([])

// 卡片数据存储
const cardsData = ref({})

// 计算网格布局样式
const gridLayoutStyle = computed(() => {
  const gap = `${managerSettings.gridGap}px`
  
  if (managerSettings.layoutDirection === 'vertical') {
    return {
      display: 'grid',
      gridTemplateColumns: 'repeat(auto-fill, minmax(300px, 1fr))',
      gap: gap,
      gridAutoFlow: 'dense'
    }
  } else {
    return {
      display: 'flex',
      flexDirection: 'column',
      gap: gap
    }
  }
})

// 添加新的卡片组
// 更新卡片管理器样式
const updateManagerStyle = () => {
  try {
    // 检查DOM API是否可用
    if (!document || !document.querySelectorAll) {
      console.warn('DOM API不可用，可能组件已卸载')
      return
    }

    const cardGroups = document.querySelectorAll('.card-group')
    if (!cardGroups || cardGroups.length === 0) {
      console.warn('找不到卡片组元素')
      return
    }
    
    cardGroups.forEach(group => {
      // 控制边框显示
      if (managerSettings.showBorder) {
        group.style.border = '1px solid #eaeaea'
        group.style.boxShadow = '0 2px 4px rgba(0,0,0,0.1)'
      } else {
        group.style.border = 'none'
        group.style.boxShadow = 'none'
      }
      
      // 控制背景显示
      if (managerSettings.showBackground) {
        group.style.backgroundColor = '#f9f9f9'
      } else {
        group.style.backgroundColor = 'transparent'
      }
    })
    
    // 将样式也应用到卡片组件
    if (cardRefs.value && cardRefs.value.length > 0) {
      cardRefs.value.forEach(cardRef => {
        if (cardRef && typeof cardRef.updateCardStyle === 'function') {
          cardRef.updateCardStyle({
            showBorder: managerSettings.showBorder,
            showBackground: managerSettings.showBackground
          })
        }
      })
    }
  } catch (error) {
    console.error('更新管理器样式时出错:', error)
  }
}

// 从卡片组件获取字体大小并应用到管理器
const updateManagerSize = (fontSize) => {
  try {
    if (fontSize && fontSize > 0) {
      managerSettings.fontSize = fontSize
      
      // 检查DOM API是否可用
      if (!document || !document.documentElement) {
        console.warn('DOM API不可用，可能组件已卸载')
        return
      }
      
      document.documentElement.style.setProperty('--manager-font-size', `${fontSize}px`)
    
    // 更新所有卡片组件的字体大小
    if (cardRefs.value && cardRefs.value.length > 0) {
      cardRefs.value.forEach(cardRef => {
        if (cardRef && typeof cardRef.updateFontSize === 'function') {
          cardRef.updateFontSize(fontSize)
        }
      })
    }
  }
  } catch (error) {
    console.warn('更新管理器尺寸时出错，组件可能已卸载', error)
  }
}

// 监听卡片字体大小变化
watch(() => managerSettings.fontSize, (newSize) => {
  try {
    // 检查DOM API是否可用
    if (!document || !document.documentElement) {
      console.warn('DOM API不可用，可能组件已卸载')
      return
    }
    document.documentElement.style.setProperty('--manager-font-size', `${newSize}px`)
  } catch (error) {
    console.warn('更新字体大小CSS变量时出错，组件可能已卸载', error)
  }
})

// 监听网格间距变化
watch(() => managerSettings.gridGap, (newGap) => {
  updateGridLayout()
})

// 更新网格布局
const updateGridLayout = () => {
  try {
    // 设置CSS变量用于卡片组件间距
    document.documentElement.style.setProperty('--card-group-gap', `${managerSettings.gridGap}px`)
    
    // 通知所有卡片组件更新间距
    if (cardRefs.value && cardRefs.value.length > 0) {
      cardRefs.value.forEach(cardRef => {
        if (cardRef && typeof cardRef.updateCardSpacing === 'function') {
          cardRef.updateCardSpacing(managerSettings.gridGap)
        }
      })
    }
  } catch (error) {
    console.warn('更新网格布局时出错', error)
  }
}

const addNewCardGroup = () => {
  const newGroup = {
    id: Date.now() + Math.random().toString(36).substr(2, 9),
    name: `卡片组 ${cardGroups.value.length + 1}`
  }
  cardGroups.value.push(newGroup)
  
  // 为新添加的卡片组添加对应的设置按钮可见性状态
  visibleSettingsButtons.value.push(true)

  // 初始化这个组的卡片数据
  cardsData.value[newGroup.id] = []
  
  // 等待DOM更新后显示设置面板并初始化默认内容
  setTimeout(() => {
    try {
      if (cardRefs.value && cardRefs.value.length > 0) {
        const newCardRef = cardRefs.value[cardRefs.value.length - 1]
        if (newCardRef) {
          // 先初始化默认内容
          if (typeof newCardRef.initializeDefaultContent === 'function') {
            newCardRef.initializeDefaultContent()
          } else {
            console.warn('卡片组件缺少initializeDefaultContent方法')
          }
          // 然后显示设置面板
          if (typeof newCardRef.showSettings === 'function') {
            newCardRef.showSettings()
          } else {
            console.warn('卡片组件缺少showSettings方法')
          }
        } else {
          console.warn('无法获取新创建的卡片组件引用')
        }
      } else {
        console.warn('cardRefs引用为空或长度为0')
      }
    } catch (error) {
      console.error('初始化新卡片组时出错:', error)
    }
  }, 100)
}

// 删除卡片组
const removeCardGroup = (index) => {
  ElMessageBox.confirm(
    '确定要删除这个卡片组吗？所有未保存的内容将丢失。',
    '警告',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    }
  )
    .then(() => {
      const groupId = cardGroups.value[index].id

      // 删除对应的卡片数据
      if (cardsData.value[groupId]) {
        delete cardsData.value[groupId]
      }

      // 删除卡片组
      cardGroups.value.splice(index, 1)
      // 同时删除对应的设置按钮可见性状态
      visibleSettingsButtons.value.splice(index, 1)
      ElMessage.success('卡片组已删除')
    })
    .catch(() => {
      // 用户取消删除
    })
}

// 打开卡片设置
const openCardSettings = (index) => {
  try {
    if (cardRefs.value && cardRefs.value[index]) {
      const cardRef = cardRefs.value[index]
      if (cardRef && typeof cardRef.showSettings === 'function') {
        cardRef.showSettings()
      } else {
        console.warn('卡片组件缺少showSettings方法')
      }
    }
  } catch (error) {
    console.error('打开卡片设置时出错:', error)
  }
}

// 处理卡片更新事件
const handleCardsUpdated = (groupId, cards, settings) => {
  cardsData.value[groupId] = cards
  
  // 如果收到了来自卡片的设置信息，更新管理器的字体大小
  if (settings && settings.fontSize) {
    updateManagerSize(settings.fontSize)
  }
}

// 更新卡片布局方向
const updateCardLayoutDirection = () => {
  try {
    // 检查DOM API是否可用
    if (!document || !document.querySelector) {
      console.warn('DOM API不可用，可能组件已卸载')
      return
    }

    // 设置卡片组的布局方向
    const container = document.querySelector('.card-groups-container')
    if (container) {
      if (managerSettings.layoutDirection === 'vertical') {
        container.classList.add('vertical-layout')
      } else {
        container.classList.remove('vertical-layout')
      }
    }
  
    // 遍历所有卡片引用，更新其布局方向
    if (cardRefs.value && cardRefs.value.length > 0) {
      cardRefs.value.forEach(cardRef => {
        if (cardRef && typeof cardRef.updateLayoutDirection === 'function') {
          cardRef.updateLayoutDirection(managerSettings.layoutDirection)
        }
      })
    }
  } catch (error) {
    console.warn('更新卡片布局方向时出错', error)
  }
  
  // 更新卡片组样式以适应新的布局方向
  setTimeout(() => {
    try {
      // 检查DOM API是否可用
      if (!document || !document.documentElement) {
        console.warn('DOM API不可用，可能组件已卸载')
        return
      }
      
      // 更新管理器样式，包括尺寸调整
      updateManagerStyle()
      // 应用布局相关的CSS变量
      document.documentElement.style.setProperty('--card-group-layout-direction', managerSettings.layoutDirection)
    } catch (error) {
      console.warn('更新卡片布局时出错，组件可能已卸载', error)
    }
  }, 50)
}

// 保存所有卡片
// 保存卡片管理器设置
const saveManagerSettings = () => {
  try {
    localStorage.setItem('chineseCardManagerSettings', JSON.stringify(managerSettings))
  } catch (error) {
    console.error('保存卡片管理器设置时出错:', error)
  }
}

const saveAllCards = () => {
  try {
    // 检查是否有卡片组
    if (cardGroups.value.length === 0) {
      ElMessage.warning('没有卡片组可保存')
      return
    }

    // 保存所有卡片组的数据到本地存储
    localStorage.setItem('chineseCardGroups', JSON.stringify(cardGroups.value))
    localStorage.setItem('chineseCardsData', JSON.stringify(cardsData.value))
    
    // 同时保存管理器设置
    saveManagerSettings()

    ElMessage.success('所有卡片组已保存')
  } catch (error) {
    console.error('保存卡片时出错:', error)
    ElMessage.error('保存失败，请重试')
  }
}

// 导出所有卡片
const exportAllCards = () => {
  try {
    // 检查是否有卡片组
    if (cardGroups.value.length === 0) {
      ElMessage.warning('没有卡片组可导出')
      return
    }

    // 准备导出数据
    const exportData = {
      groups: cardGroups.value,
      cards: cardsData.value,
      exportTime: new Date().toISOString()
    }

    // 转换为JSON并创建下载链接
    const dataStr = JSON.stringify(exportData, null, 2)
    const dataUri = 'data:application/json;charset=utf-8,' + encodeURIComponent(dataStr)

    const exportFileName = `chinese_cards_export_${new Date().toISOString().slice(0, 10)}.json`

    const linkElement = document.createElement('a')
    linkElement.setAttribute('href', dataUri)
    linkElement.setAttribute('download', exportFileName)
    linkElement.click()

    ElMessage.success('卡片数据已导出')
  } catch (error) {
    console.error('导出卡片时出错:', error)
    ElMessage.error('导出失败，请重试')
  }
}

// 从本地存储加载保存的卡片组
const loadSavedCardGroups = () => {
  try {
    const savedGroups = localStorage.getItem('chineseCardGroups')
    const savedCardsData = localStorage.getItem('chineseCardsData')

    if (savedGroups && savedCardsData) {
      const parsedGroups = JSON.parse(savedGroups)
      const parsedData = JSON.parse(savedCardsData)
      
      // 检查是否有有效数据
      let hasValidData = false
      for (const groupId in parsedData) {
        if (parsedData[groupId] && parsedData[groupId].length > 0) {
          hasValidData = true
          break
        }
      }
      
      if (hasValidData) {
        // 只有当有有效数据时才加载保存的卡片组
        cardGroups.value = parsedGroups
        cardsData.value = parsedData
        
        if (cardGroups.value.length > 0) {
          ElMessage.success(`已加载 ${cardGroups.value.length} 个保存的卡片组`)
        }
      }
      // 如果没有有效数据，则不创建默认卡片组，保持为空
    }
    // 没有表单数据时默认不保留卡片组
  } catch (error) {
    console.error('加载保存的卡片组时出错:', error)
    // 出错时不自动创建默认卡片组
  }
}

// 提供方法让父组件可以访问卡片内容
const loadFormsToAllCards = (forms) => {
  try {
    if (!forms || forms.length === 0) {
      ElMessage.warning('表单内容为空')
      return
    }

    // 清空现有卡片组，确保每个表单都能创建新的卡片组
    // 先备份当前的卡片组，以防用户想保留
    const hasExistingGroups = cardGroups.value.length > 0
    
    if (hasExistingGroups) {
      ElMessageBox.confirm(
        '转换将创建新的卡片组，是否保留当前的卡片组？',
        '提示',
        {
          confirmButtonText: '保留',
          cancelButtonText: '不保留',
          type: 'info',
        }
      )
        .then(() => {
          // 用户选择保留现有卡片组，继续添加新卡片组
          createCardGroupsFromForms(forms)
          // 更新设置按钮可见性数组，匹配新的卡片组数量
          visibleSettingsButtons.value = Array(cardGroups.value.length).fill(false)
        })
        .catch(() => {
          // 用户选择不保留，清空现有卡片组后再创建新的
          cardGroups.value = []
          cardsData.value = {}
          visibleSettingsButtons.value = []
          createCardGroupsFromForms(forms)
        })
    } else {
      // 没有现有卡片组，直接创建新的
      createCardGroupsFromForms(forms)
    }
  } catch (error) {
    console.error('加载表单到卡片时出错:', error)
    ElMessage.error('加载表单失败，请重试')
  }
}

// 从表单创建卡片组
const createCardGroupsFromForms = (forms) => {
  // 首先创建所有卡片组
  const newGroups = forms.map(form => ({
    id: Date.now() + Math.random().toString(36).substr(2, 9),
    name: form.title || `表单 ${cardGroups.value.length + 1}`,
    form: form // 将表单与组关联
  }))
  
  // 一次性添加所有卡片组
  cardGroups.value = [...cardGroups.value, ...newGroups]
  
  // 初始化这些组的卡片数据
  newGroups.forEach(group => {
    cardsData.value[group.id] = []
  })
  
  // 更新设置按钮可见性数组，为每个新添加的卡片组添加对应的状态
  visibleSettingsButtons.value = Array(cardGroups.value.length).fill(false)
  
  // 使用nextTick等待DOM更新后加载表单内容
  setTimeout(() => {
    // 获取所有卡片组组件引用
    if (cardRefs.value && cardRefs.value.length > 0) {
      // 只处理新创建的卡片组
      const startIndex = cardRefs.value.length - newGroups.length
      
      // 为每个新卡片组加载对应的表单
      for (let i = 0; i < newGroups.length; i++) {
        const cardRef = cardRefs.value[startIndex + i]
        const form = newGroups[i].form
        
        if (cardRef && typeof cardRef.loadFromForms === 'function' && form) {
          // 只将当前表单加载到这个卡片组
          cardRef.loadFromForms([form])
        }
      }
      
      ElMessage.success(`已为 ${forms.length} 个表单创建 ${forms.length} 个独立的卡片组`)
    }
  }, 300) // 增加延迟，确保DOM完全更新
}

// 加载管理器设置
const loadManagerSettings = () => {
  try {
    const savedSettings = localStorage.getItem('chineseCardManagerSettings')
    if (savedSettings) {
      const parsedSettings = JSON.parse(savedSettings)
      Object.assign(managerSettings, parsedSettings)
    }
  } catch (error) {
    console.error('加载卡片管理器设置时出错:', error)
  }
}

// 组件加载时
onMounted(() => {
  // 初始化设置按钮可见性数组
  visibleSettingsButtons.value = Array(cardGroups.value.length).fill(true)
  
  // 加载管理器设置
  loadManagerSettings()
  
  // 应用管理器样式
  updateManagerStyle()
  
  // 设置CSS变量
  document.documentElement.style.setProperty('--manager-font-size', `${managerSettings.fontSize}px`)
  
  // 初始化布局方向
  updateCardLayoutDirection()
  
  // 尝试从本地存储加载数据
  loadSavedCardGroups()
  
})

// 暴露给父组件的方法
defineExpose({
  loadFormsToAllCards,
  addNewCardGroup,
  saveAllCards,
  exportAllCards,
  updateManagerSize, // 暴露更新尺寸方法
  updateManagerStyle, // 暴露更新样式方法
  updateGridLayout, // 暴露网格布局更新方法
  openCardSettings // 暴露打开卡片设置方法
})
</script>

<style scoped>
.card-manager-container {
  display: flex;
  flex-direction: column;
  gap: 20px;
  width: 100%;
}

.controls-section {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
  margin-bottom: 10px;
}

.left-controls {
  display: flex;
  gap: 10px;
}

.right-controls {
  display: flex;
  align-items: center;
  gap: 15px;
}

.spacing-control {
  display: flex;
  align-items: center;
  gap: 5px;
  white-space: nowrap;
}

.style-switch {
  min-width: 120px;
}

.card-groups-container {
  display: flex;
  flex-direction: column;
  gap: var(--card-group-gap, 15px); /* 使用CSS变量控制间距 */
  width: 100%;
  transition: all 0.3s ease;
  /* 横排时从上到下排列 */
}
.card-group {
  position: relative; /* 添加相对定位，用于设置按钮的绝对定位 */
  border: 1px solid #eaeaea;
  border-radius: 8px;
  background-color: #f9f9f9;
  /* 减小卡片组高度，更好地适配汉字卡片 */
  display: flex;
  flex-direction: column;
  /* 使用CSS变量设置尺寸 */
  font-size: var(--manager-font-size, 36px);
  transition: all 0.3s ease;
  /* 根据字体大小调整内边距 */
  padding: var(--card-group-padding, calc(var(--manager-font-size, 36px) * 0.2));
  /* 根据卡片大小自适应宽高 */
  width: 100%; /* 填满父容器可用空间 */
  height: auto;
  box-sizing: border-box;
  /* 确保宽度不会超过父容器 */
  max-width: 100%;
  /* 移除固定比例，跟随卡片组尺寸变化 */
  /* 添加悬停效果 */
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.card-group:hover {
  transform: translateY(-3px);
  box-shadow: 0 5px 15px rgba(0,0,0,0.08);
}

/* 设置按钮容器样式 */
.settings-button-container {
  position: absolute;
  top: 10px;
  right: 10px;
  z-index: 10;
  opacity: 1;
}

.card-settings-button {
  opacity: 1;
}

.card-group-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  padding-bottom: 10px;
  border-bottom: 1px solid #eaeaea;
}

.card-group-actions {
  display: flex;
  gap: 8px;
}

/* 横排布局 */
.card-groups-container.vertical-layout {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: var(--card-group-gap, 15px);
  width: 100%;
}

/* 竖排布局下的卡片组样式 */
.card-groups-container.vertical-layout .card-group {
  box-sizing: border-box;
  margin: 0; /* 移除外边距，使用gap控制间距 */
  min-width: 250px; /* 确保最小宽度适合内容 */
  /* 根据汉字卡片大小调整尺寸 */
  padding: var(--card-group-padding, calc(var(--manager-font-size, 36px) * 0.2));
  /* 自适应卡片组尺寸 */
  width: 100%; /* 填满分配空间 */
  height: auto; /* 高度自动适应卡片组内容 */
}

/* 在小屏幕上改为单列 */
@media (max-width: 768px) {
  .card-groups-container.vertical-layout .card-group {
    width: 100%;
  }
}
</style>