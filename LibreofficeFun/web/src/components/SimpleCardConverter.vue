<template>
  <div class="simple-card-converter" :style="containerStyle">
    <!-- 浮动工具条 -->
    <div class="floating-toolbar" :style="toolbarStyle" @mousedown="startDrag">
      <div class="toolbar-header">
        <h3>表单转卡片</h3>
        <div class="drag-handle">⋮⋮</div>
      </div>
      <div class="toolbar-content">
        <!-- 添加空值检查 -->
        <el-switch v-if="converterSettings" v-model="converterSettings.showBorder" active-text="显示边框"
          inactive-text="隐藏边框" @change="updateStyle" />
        <!-- 添加空值检查 -->
        <el-switch v-if="converterSettings" v-model="converterSettings.showBackground" active-text="显示背景"
          inactive-text="隐藏背景" @change="updateStyle" />

        <!-- 添加空值检查 -->
        <StyleSettings v-if="converterSettings" v-model="converterSettings" :predefine-colors="predefineColors"
          @update:model-value="updateStyle" />

        <!-- 添加card-text样式管理器 -->
        <CardTextManager ref="cardTextManager" v-if="converterSettings" v-model="converterSettings"
          :predefine-colors="predefineColors" @change="updateStyle" @apply="updateStyle" />


        <!-- 卡片样式选择器 -->
        <span class="control-label">卡片样式：</span>
        <el-select v-model="selectedCardStyle" placeholder="选择卡片样式" size="small" @change="handleCardStyleChange">
          <el-option v-for="option in cardStyleOptions" :key="option.value" :label="option.label"
            :value="option.value" />
        </el-select>
      </div>
    </div>

    <div class="cards-display-area">
      <div class="card-group" v-for="(group, groupIndex) in cardGroups" :key="group.id || groupIndex"
        :style="getCardGroupStyle()">
        <div class="group-title">{{ group.title || `表单卡片组 ${groupIndex + 1}` }}</div>
        <div class="cards-container" :class="{ 'vertical-layout': converterSettings.layout === 'vertical' }">
          <div v-for="(row, rowIndex) in group.rows" :key="`${group.id || groupIndex}-${rowIndex}`" class="card-row"
            :class="row.type">
            <div v-for="(card, cardIndex) in row.cards" :key="`${group.id || groupIndex}-${rowIndex}-${cardIndex}`"
              class="text-card" :style="getCardStyle(card)" @click="editCard(groupIndex, rowIndex, cardIndex)"
              :class="card.type">
              <span class="card-text" :style="cardTextStyle">{{ card?.text || '示例' }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- 当没有卡片时显示提示 -->
      <div v-if="!cardGroups || cardGroups.length === 0" class="no-cards-placeholder">
        <p>暂无卡片内容</p>
        <p class="hint">请先在表单页面添加表单，然后转换为卡片</p>
      </div>
    </div>

    <!-- 编辑卡片对话框 -->
    <el-dialog v-model="editDialogVisible" title="编辑卡片" width="300px" @close="onDialogClose">
      <!-- 添加空值检查和默认值 -->
      <el-input v-model="editingCard.text" maxlength="10" show-word-limit :value="editingCard.text || ''"
        @input="value => editingCard.text = value" />

      <template #footer>
        <el-button @click="toggleStylePanel">{{ showStylePanel ? '隐藏样式' : '样式设置' }}</el-button>
        <!-- 样式设置面板 -->
        <div v-show="showStylePanel" class="style-panel">
          <CardTextManager v-model="editingCard.style" :predefine-colors="predefineColors" @change="updateCardStyle"
            @apply="updateCardStyle" />
        </div>
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveCardEdit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>
<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, watch, nextTick, computed } from 'vue';
import StyleSettings from './StyleSettings.vue';
import { useEventBus } from '@/utils/eventBus';
import { ElMessage } from 'element-plus';
import CardTextManager from './CardTextManager.vue'; // 新增导入

// 创建事件总线实例
const { on, off, emit } = useEventBus();

// 现在使用CardTextManager管理卡片文本样式
// 通过props传递样式设置

// 添加组件加载调试信息
console.log('SimpleCardConverter component initializing');

// 拖动相关数据
const isDragging = ref(false);
const dragOffset = reactive({ x: 0, y: 0 });
const toolbarPosition = reactive({ x: 20, y: 20 }); // 默认位置

// 添加页面尺寸相关数据
const pageSize = ref({ width: 800, height: 600 }); // 默认页面尺寸，像素单位

// 添加cardTextManager引用
const cardTextManager = ref(null)

// 定义props
const props = defineProps({
  forms: {
    type: Array,
    default: () => []
  },
  // 添加页面尺寸属性
  pageSize: {
    type: Object,
    default: () => ({ width: 800, height: 600 }) // 默认使用像素单位
  }
});

// 在组件挂载时注册事件
onMounted(() => {
  // 监听页面尺寸变化事件
  on('page-size-changed', handlePageSizeChange)

  // 如果有传入的表单数据，自动进行转换
  if (props.forms && props.forms.length > 0) {
    console.log('[SimpleCardConverter] 检测到传入的表单数据，自动进行转换');
    convertFormsToCards(props.forms);
  }

  console.log('SimpleCardConverter component mounted', {
    cardGroups: cardGroups.value,
    selectedCardStyle: selectedCardStyle.value
  });
})

// 监听页面尺寸变化
watch(() => props.pageSize, (newSize) => {
  if (newSize && newSize.width && newSize.height) {
    // 转换单位，从mm转换为px (假设 1mm ≈ 3.78px)
    const convertedSize = { ...newSize };
    if (newSize.unit === 'mm') {
      convertedSize.width = Math.round(newSize.width * 3.78);
      convertedSize.height = Math.round(newSize.height * 3.78);
    } else {
      // 如果已经是像素单位，直接使用
      convertedSize.width = newSize.width;
      convertedSize.height = newSize.height;
    }
    pageSize.value = convertedSize;
  }
}, { immediate: true });

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
  '#f0f5ff',
  '#f9f0ff',
  '#fff0f6',
  'rgba(255, 69, 0, 0.68)'
]

// 卡片样式选项
const cardStyleOptions = [
  { value: 'default', label: '默认样式' },
  { value: 'minimal', label: '极简样式' },
  { value: 'rounded', label: '圆角样式' },
  { value: 'outlined', label: '边框样式' },
  { value: 'elevation', label: '投影样式' },
  { value: 'gradient', label: '渐变样式' }
]

// 选定的卡片样式
const selectedCardStyle = ref('default')

// 转换器设置
const converterSettings = reactive({
  showBorder: true,
  showBackground: false, // 默认不显示背景
  fontSize: 24,
  padding: 0,
  margin: 0,
  spacing: 0,
  rowSpacing: 30, // 添加行距属性，默认值为30px
  textRotation: 0,
  backgroundColor: '#ffffff',
  textColor: '#000000',
  borderStyle: 'none',
  borderWidth: 1,
  borderColor: '#000000',
  borderRadius: 0,
  boxShadow: 'none',
  gradient: 'none',
  layout: 'horizontal',
  fontWeight: 'normal',
  fontStyle: 'normal',
  textAlign: 'center',
  lineHeight: 1.2,
  opacity: 1,
  transform: '',
  width: '',
  height: '',
  display: 'flex',
  position: 'static'
})

// 卡片组数据
const cardGroups = ref([])

// 编辑功能相关
const editDialogVisible = ref(false)
const showStylePanel = ref(false) // 控制样式面板显示
const editingCard = reactive({
  text: '',
  groupIndex: -1,
  rowIndex: -1,
  cardIndex: -1,
  style: {} // 添加样式设置
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

  // 过滤有效的表单（至少有一个字段有内容）
  const validForms = forms.filter(form =>
    (form.title && form.title.trim()) ||
    (form.value && form.value.trim()) ||
    (form.remark && form.remark.trim()) ||
    (form.media && form.media.trim())
  )

  if (validForms.length === 0) {
    ElMessage.warning('没有有效的表单内容可以转换')
    return
  }

  const newGroups = validForms.map((form, index) => {
    // 创建行对象，每行包含一个字段的所有字符卡片
    const rows = [];

    // 处理表单标题（如果有且不为空）
    if (form.title && form.title.trim()) {
      // 为标题中的每个字符创建单独的卡片，并将它们放在同一行
      const titleChars = form.title.trim().split('')
      const titleCards = titleChars.map(char => ({
        text: char,
        type: 'title',
        color: converterSettings.textColor,
        backgroundColor: converterSettings.showBackground ? converterSettings.backgroundColor : 'transparent'
      }))
      rows.push({
        type: 'title-row',
        cards: titleCards
      });
    }

    // 处理表单内容（如果有且不为空）
    if (form.value && form.value.trim()) {
      // 为内容中的每个字符创建单独的卡片，并将它们放在同一行
      const valueChars = form.value.trim().split('')
      const valueCards = valueChars.map(char => ({
        text: char,
        type: 'value',
        color: converterSettings.textColor,
        backgroundColor: converterSettings.showBackground ? converterSettings.backgroundColor : 'transparent'
      }))
      rows.push({
        type: 'value-row',
        cards: valueCards
      });
    }

    // 处理表单备注（如果有且不为空）
    if (form.remark && form.remark.trim()) {
      // 为备注中的每个字符创建单独的卡片，并将它们放在同一行
      const remarkChars = form.remark.trim().split('')
      const remarkCards = remarkChars.map(char => ({
        text: char,
        type: 'remark',
        color: converterSettings.textColor,
        backgroundColor: converterSettings.showBackground ? converterSettings.backgroundColor : 'transparent'
      }))
      rows.push({
        type: 'remark-row',
        cards: remarkCards
      });
    }

    // 如果没有内容，创建一个默认行
    if (rows.length === 0) {
      rows.push({
        type: 'empty-row',
        cards: [{
          text: '空',
          type: 'empty',
          color: converterSettings.textColor,
          backgroundColor: converterSettings.showBackground ? converterSettings.backgroundColor : 'transparent'
        }]
      });
    }

    return {
      id: form.id || `group-${Date.now()}-${index}`,
      title: form.title || `表单 ${index + 1}`,
      rows
    }
  })

  cardGroups.value = newGroups
  nextTick(() => {
    updateStyle()
    ElMessage.success(`成功将 ${validForms.length} 个表单转换为卡片`)
  })
}

// 更新卡片样式
const updateStyle = () => {
  // 这个函数被调用时会触发重新渲染
  // 由于使用了响应式数据，Vue会自动应用最新样式
  cardGroups.value = [...cardGroups.value] // 强制触发重新渲染
  nextTick(() => {
    // ElMessage.success('样式已更新')
  })
}


// 获取卡片组样式
const getCardGroupStyle = () => {
  return {
    marginBottom: `${converterSettings.rowSpacing}px`, // 使用 converterSettings 中的行距
    border: converterSettings.showBorder ? '1px solid #ddd' : 'none',
    borderRadius: '4px',
    padding: '10px',
    backgroundColor: '#fff'
  }
}

// 获取卡片行样式
const getRowStyle = () => {
  return {
    marginBottom: `${rowSpacing.value}px`
  }
}

// 获取卡片样式（包括单独设置的样式）
const getCardStyle = (card) => {
  // 添加空值检查
  if (!card) {
    return {}
  }

  // 确保基础样式属性存在
  const baseSize = converterSettings.fontSize || 24
  const margin = converterSettings.margin !== undefined ? converterSettings.margin : 8
  const padding = converterSettings.padding !== undefined ? converterSettings.padding : 16
  const textColor = converterSettings.textColor || '#000000'
  const backgroundColor = converterSettings.backgroundColor || '#ffffff'
  const borderStyle = converterSettings.borderStyle || 'none'
  const borderRadius = converterSettings.borderRadius !== undefined ? converterSettings.borderRadius : 0
  const boxShadow = converterSettings.boxShadow || 'none'
  const textRotation = converterSettings.textRotation || 0
  const showBackground = converterSettings.showBackground !== undefined ? converterSettings.showBackground : true
  const gradient = converterSettings.gradient || 'none'
  const spacing = converterSettings.spacing !== undefined ? converterSettings.spacing : 0 // 添加 spacing

  // 合并全局样式和卡片单独样式
  const cardStyle = card.style || {}

  const style = {
    width: `${baseSize + padding}px`,
    height: `${baseSize + padding}px`,
    margin: margin > 0 ? `${margin}px` : '0',
    padding: padding > 0 ? `${padding}px` : '0',
    fontSize: `${baseSize}px`,
    color: cardStyle.color || textColor,
    backgroundColor: showBackground
      ? cardStyle.backgroundColor || (gradient === 'none' ? backgroundColor : gradient)
      : 'transparent',
    transform: `rotate(${textRotation}deg)`,
    border: borderStyle,
    borderRadius: borderRadius > 0 ? `${borderRadius}px` : '0',
    boxShadow: boxShadow,
    display: 'flex',
    justifyContent: 'center',
    alignItems: 'center',
    transition: 'all 0.3s ease',
    cursor: 'pointer',
    wordBreak: 'break-all',
    textAlign: 'center',
    lineHeight: '1.2',
    // 应用间距设置
    marginRight: spacing > 0 ? `${spacing}px` : '0',
    marginBottom: spacing > 0 ? `${spacing}px` : '0'
  }

  // 应用卡片单独设置的样式
  Object.assign(style, cardStyle)

  return style
}

// 编辑卡片
const editCard = (groupIndex, rowIndex, cardIndex) => {
  // 现在需要三个索引：组索引、行索引和卡片索引
  const cardGroup = cardGroups.value[groupIndex];
  if (!cardGroup) return;

  const row = cardGroup.rows[rowIndex];
  if (!row) return;

  const card = row.cards[cardIndex];
  if (!card) return;

  editingCard.text = card.text
  editingCard.groupIndex = groupIndex
  editingCard.rowIndex = rowIndex
  editingCard.cardIndex = cardIndex
  // 添加卡片样式设置
  editingCard.style = card.style || {}
  editDialogVisible.value = true
}

// 保存卡片编辑
const saveCardEdit = () => {
  const { groupIndex, rowIndex, cardIndex, text, style } = editingCard
  if (groupIndex >= 0 && rowIndex >= 0 && cardIndex >= 0 &&
    cardGroups.value[groupIndex]?.rows[rowIndex]?.cards[cardIndex]) {
    // 更新卡片文本
    cardGroups.value[groupIndex].rows[rowIndex].cards[cardIndex].text = text
    // 更新卡片样式
    cardGroups.value[groupIndex].rows[rowIndex].cards[cardIndex].style = { ...style }
    editDialogVisible.value = false
    showStylePanel.value = false // 关闭样式面板
    ElMessage.success('卡片内容已更新')
  }
}

// 处理卡片样式变化
const handleCardStyleChange = (styleValue) => {
  console.log('Card style changed to:', styleValue);

  // 根据选择的样式更新设置
  switch (styleValue) {
    case 'minimal':
      Object.assign(converterSettings, {
        padding: 2,
        margin: 3,
        backgroundColor: '#ffffff',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 0,
        boxShadow: 'none',
        gradient: 'none',
        fontWeight: 'normal',
        fontStyle: 'normal',
        textAlign: 'center',
        lineHeight: 1.2
      })
      break
    case 'rounded':
      Object.assign(converterSettings, {
        padding: 2,
        margin: 3,
        backgroundColor: '#ffffff',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 5,
        boxShadow: 'none',
        gradient: 'none',
        fontWeight: 'normal',
        fontStyle: 'normal',
        textAlign: 'center',
        lineHeight: 1.2
      })
      break
    case 'outlined':
      Object.assign(converterSettings, {
        padding: 2,
        margin: 3,
        backgroundColor: 'transparent',
        textColor: '#333333',
        borderStyle: 'solid',
        borderWidth: 1,
        borderColor: '#cccccc',
        borderRadius: 5,
        boxShadow: 'none',
        gradient: 'none',
        fontWeight: 'normal',
        fontStyle: 'normal',
        textAlign: 'center',
        lineHeight: 1.2
      })
      break
    case 'elevation':
      Object.assign(converterSettings, {
        padding: 2,
        margin: 3,
        backgroundColor: '#f8f8f8',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 5,
        boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
        gradient: 'none',
        fontWeight: 'normal',
        fontStyle: 'normal',
        textAlign: 'center',
        lineHeight: 1.2
      })
      break
    case 'gradient':
      Object.assign(converterSettings, {
        padding: 2,
        margin: 3,
        backgroundColor: 'linear-gradient(135deg, #ffffff, #f0f0f0)',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 5,
        boxShadow: 'none',
        gradient: 'linear-gradient(135deg, #ffffff, #f0f0f0)',
        fontWeight: 'normal',
        fontStyle: 'normal',
        textAlign: 'center',
        lineHeight: 1.2
      })
      break
    default:
      // 默认样式
      Object.assign(converterSettings, {
        padding: 2,
        margin: 3,
        backgroundColor: '#ffffff',
        textColor: '#000000',
        borderStyle: 'none',
        borderRadius: 5,
        boxShadow: 'none',
        gradient: 'none',
        fontWeight: 'normal',
        fontStyle: 'normal',
        textAlign: 'center',
        lineHeight: 1.2
      })
      break
  }

  // 触发样式更新
  updateStyle()
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

// 监听页面尺寸变化
watch(() => props.pageSize, (newSize) => {
  if (newSize && newSize.width && newSize.height) {
    // 转换单位，从mm转换为px (假设 1mm ≈ 3.78px)
    const convertedSize = { ...newSize };
    if (newSize.unit === 'mm') {
      convertedSize.width = Math.round(newSize.width * 3.78);
      convertedSize.height = Math.round(newSize.height * 3.78);
    } else {
      // 如果已经是像素单位，直接使用
      convertedSize.width = newSize.width;
      convertedSize.height = newSize.height;
    }
    pageSize.value = convertedSize;
  }
}, { immediate: true });

// 开始拖动
const startDrag = (event) => {
  // 不再限制只能在drag-handle上拖拽，整个工具条头部都可以拖拽
  if (event.target.closest('.toolbar-header')) {
    isDragging.value = true;
    const toolbar = event.currentTarget;
    const rect = toolbar.getBoundingClientRect();

    // 改进拖拽偏移计算，使其更加准确
    dragOffset.x = event.clientX - rect.left;
    dragOffset.y = event.clientY - rect.top;

    // 添加事件监听器到document以获得更好的拖拽体验
    document.addEventListener('mousemove', onDrag);
    document.addEventListener('mouseup', stopDrag);

    // 防止文本选择和其他默认行为
    event.preventDefault();
    event.stopPropagation();
  }
};

// 拖动中
const onDrag = (event) => {
  if (!isDragging.value) return;

  // 优化拖拽位置计算，增加边界检查
  const containerRect = document.querySelector('.simple-card-converter').getBoundingClientRect();

  // 计算工具条的新位置
  let newX = event.clientX - dragOffset.x;
  let newY = event.clientY - dragOffset.y;

  // 添加边界检查，防止工具条被拖出可视区域
  newX = Math.max(0, Math.min(newX, containerRect.width - 200)); // 假设工具条宽度约为200px
  newY = Math.max(0, Math.min(newY, containerRect.height - 100)); // 假设工具条高度约为100px

  toolbarPosition.x = newX;
  toolbarPosition.y = newY;

  event.preventDefault();
  event.stopPropagation();
};

// 停止拖动
const stopDrag = (event) => {
  isDragging.value = false;
  document.removeEventListener('mousemove', onDrag);
  document.removeEventListener('mouseup', stopDrag);

  event.preventDefault();
  event.stopPropagation();
};

// 计算容器样式
const containerStyle = computed(() => {
  return {
    width: pageSize.value.width ? `${pageSize.value.width}px` : '800px',
    height: pageSize.value.height ? `${pageSize.value.height}px` : '600px',
    position: 'relative',
    margin: '0 auto',
    overflow: 'hidden' /* 隐藏滚动条 */
  };
});

// 计算工具条样式
const toolbarStyle = computed(() => {
  return {
    position: 'absolute',
    left: `${toolbarPosition.x}px`,
    top: `${toolbarPosition.y}px`,
    zIndex: 1000,
    cursor: isDragging.value ? 'grabbing' : 'grab'
  };
});

// 计算卡片文本样式
const cardTextStyle = computed(() => {
  // 如果cardTextManager可用，使用其getStyle方法获取样式
  if (cardTextManager.value) {
    return cardTextManager.value.getStyle()
  }

  // 否则使用原来的计算逻辑
  const style = {};

  // 字体样式
  if (converterSettings.fontSize) {
    style.fontSize = `${converterSettings.fontSize}px`;
  }

  if (converterSettings.textColor) {
    style.color = converterSettings.textColor;
  }

  if (converterSettings.fontWeight && converterSettings.fontWeight !== 'normal') {
    style.fontWeight = converterSettings.fontWeight;
  }

  if (converterSettings.fontStyle && converterSettings.fontStyle !== 'normal') {
    style.fontStyle = converterSettings.fontStyle;
  }

  // 背景样式
  if (converterSettings.showBackground) {
    style.backgroundColor = converterSettings.backgroundColor || 'transparent';
  } else {
    style.backgroundColor = 'transparent';
  }

  // 布局样式
  if (converterSettings.textAlign) {
    style.textAlign = converterSettings.textAlign;
  }

  if (converterSettings.lineHeight) {
    style.lineHeight = converterSettings.lineHeight;
  }

  // 盒模型样式
  if (converterSettings.padding !== undefined) {
    style.padding = `${converterSettings.padding}px`;
  }

  if (converterSettings.margin !== undefined) {
    style.margin = `${converterSettings.margin}px`;
  }

  // 边框样式
  if (converterSettings.borderStyle && converterSettings.borderStyle !== 'none') {
    style.borderStyle = converterSettings.borderStyle;
    style.borderWidth = `${converterSettings.borderWidth || 1}px`;
    style.borderColor = converterSettings.borderColor || '#000000';
  } else {
    style.border = 'none';
  }

  if (converterSettings.borderRadius) {
    style.borderRadius = `${converterSettings.borderRadius}px`;
  }

  // 高级样式
  if (converterSettings.opacity !== undefined && converterSettings.opacity !== 1) {
    style.opacity = converterSettings.opacity;
  }

  if (converterSettings.boxShadow) {
    style.boxShadow = converterSettings.boxShadow;
  }

  if (converterSettings.transform) {
    style.transform = converterSettings.transform;
  }

  return style;
});

// 对话框关闭处理
const onDialogClose = () => {
  showStylePanel.value = false // 关闭样式面板
}

// 切换样式面板显示
const toggleStylePanel = () => {
  showStylePanel.value = !showStylePanel.value
}

// 更新卡片样式
const updateCardStyle = (style) => {
  // 实时更新编辑中的卡片样式
  editingCard.style = { ...style }
}

</script>


<style scoped>
.setting-item {
  display: flex;
  align-items: center;
  gap: 10px;
}

.control-label {
  margin-right: 8px;
  font-size: 14px;
  color: #606266;
  white-space: nowrap;
}

.spacing-value {
  font-size: 12px;
  color: #909399;
  min-width: 40px;
}

.simple-card-converter {
  width: 100%;
  min-height: 100%;
  padding: 0px;
  background-color: #f5f5f5;
  position: relative;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.cards-display-area {
  width: 100%;
  min-height: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  align-items: center;
  overflow: hidden;
  /* 隐藏滚动条 */
  background-color: #f0f0f0;
  /* 简化的背景 */
  padding: 0px;
  /* 去掉内边距 */

  /* 使用页面纸张尺寸 */
  @media print {
    width: calc(100% - 40px);
    height: calc(100% - 40px);
    margin: 20px;
    padding: 0;
    background-color: #ffffff;
    /* 打印时使用纯白背景 */
  }
}

/* 浮动工具条样式 */
.floating-toolbar {
  position: absolute;
  top: 20px;
  left: 20px;
  background-color: #fff;
  border-radius: 8px;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.15);
  z-index: 1000;
  min-width: 320px;
  user-select: none;
  transition: box-shadow 0.3s ease;
}

.floating-toolbar:hover {
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.2);
}

.floating-toolbar:active {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.toolbar-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background-color: #409eff;
  color: white;
  border-top-left-radius: 8px;
  border-top-right-radius: 8px;
  cursor: move;
  /* 明确指示整个头部都可以拖动 */
}

.toolbar-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 500;
}

.drag-handle {
  cursor: move;
  font-size: 20px;
  line-height: 1;
  padding: 4px 8px;
  border-radius: 4px;
  transition: background-color 0.2s;
}

.drag-handle:hover {
  background-color: rgba(255, 255, 255, 0.2);
}

.toolbar-content {
  padding: 16px;
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  align-items: center;
  background-color: #fff;
  border-bottom-left-radius: 8px;
  border-bottom-right-radius: 8px;
}

.card-group {
  background-color: transparent;
  margin-bottom: v-bind('converterSettings.rowSpacing + "px"');
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  border-radius: 6px;
  overflow: hidden;
  max-width: 100%;
  /* 根据页面尺寸设置固定宽度 */
  width: v-bind('containerStyle.width');
  box-sizing: border-box;
  border: 1px solid #e0e0e0;
  margin: 0 auto v-bind('converterSettings.rowSpacing + "px"') auto;
  position: relative;
  /* 添加明确的内边距 */
  padding: 10px;
}

.card-group:last-child {
  margin-bottom: 0;
}

.card-group::before {
  display: none;
}

.card-group::after {
  display: none;
}

.cards-container {
  display: flex;
  flex-direction: column;
  padding: 0px;
  width: 100%;
  box-sizing: border-box;
  overflow: hidden;
  scroll-behavior: smooth;
  background: #fafafa;
  max-height: calc(100vh - 120px);
}

.card-row {
  display: flex;
  flex-wrap: wrap;
  margin: 0;
  gap: v-bind('converterSettings.spacing + "px"');
  /* 使用间距设置 */
}

.card-row:last-child {
  margin-bottom: 0;
}

.text-card {
  box-shadow: none !important;
  border: none !important;
  flex-shrink: 0;
  display: inline-flex;
  justify-content: center;
  align-items: center;
  padding: 0;
  color: v-bind('converterSettings.textColor');
  /* 应用字体颜色设置 */
  background-color: v-bind('converterSettings.showBackground ? converterSettings.backgroundColor : "transparent"');
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
  white-space: pre;
}

.vertical-layout .card-text {
  writing-mode: vertical-rl;
  text-orientation: mixed;
}

/* 不同行类型的样式 */
.card-row.title-row {
  /* 标题行样式 */
}

.card-row.value-row {
  /* 内容行样式 */
}

.card-row.remark-row {
  /* 备注行样式 */
}

.card-row.empty-row {
  /* 空行样式 */
}

/* 不同类型的卡片可以有不同的样式 */
.text-card.title {
  /* 标题卡片样式 */
}

.text-card.value {
  /* 内容卡片样式 */
}

.text-card.remark {
  /* 备注卡片样式 */
}

.text-card.empty {
  /* 空卡片样式 */
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

/* 无卡片占位符样式 */
.no-cards-placeholder {
  text-align: center;
  padding: 40px 20px;
  color: #909399;
}

.no-cards-placeholder p {
  margin: 10px 0;
}

.no-cards-placeholder .hint {
  font-size: 14px;
  color: #c0c4cc;
}

.dialog-actions {
  margin: 15px 0;
  text-align: center;
}

.style-panel {
  margin: 10px 0;
  border: 1px solid #ebeef5;
  border-radius: 4px;
  padding: 10px;
  max-height: 300px;
  overflow-y: auto;
}
</style>
