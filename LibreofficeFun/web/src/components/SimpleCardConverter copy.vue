<template>
  <div class="simple-card-converter" :style="containerStyle">
    <!-- 浮动工具条 -->
    <div class="floating-toolbar" :style="toolbarStyle" @mousedown="startDrag">
      <div class="toolbar-header">
        <h3>表单转卡片</h3>
        <div class="drag-handle">⋮⋮</div>
      </div>
      <!-- 添加空值检查 -->
      <el-switch v-if="converterSettings" v-model="converterSettings.showBorder" active-text="显示边框" inactive-text="隐藏边框"
        @change="updateStyle" />
      <!-- 添加空值检查 -->
      <el-switch v-if="converterSettings" v-model="converterSettings.showBackground" active-text="显示背景"
        inactive-text="隐藏背景" @change="updateStyle" />

      <!-- 添加card-text样式管理器 -->
      <StyleSettings v-if="converterSettings" v-model="converterSettings" :predefine-colors="predefineColors"
        @update:model-value="updateStyle" />

      <!-- 添加card-text样式管理器 -->
      <div class="global-style-settings">
        <el-button @click="openGlobalStylePanel" size="small" type="primary">
          统一样式设置
        </el-button>
      </div>
      <div class="toolbar-content">

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
        <div class="cards-container" :class="{ 'vertical-layout': converterSettings.layout === 'vertical' }">
          <div v-for="(row, rowIndex) in group.rows" :key="'row-' + (group.id || groupIndex) + '-' + rowIndex"
            class="card-row" :class="row.type" @click.self="editRowStyle(groupIndex, rowIndex)"
            :data-row-id="`${groupIndex}-${rowIndex}`">
            <div v-for="(card, cardIndex) in row.cards"
              :key="'card-' + (group.id || groupIndex) + '-' + rowIndex + '-' + cardIndex"
              @click="editCard(groupIndex, rowIndex, cardIndex)" :class="card.type"
              :data-card-id="`${groupIndex}-${rowIndex}-${cardIndex}`" class="text-card-container">
              <span class="card-text" :style="getCardTextStyle(card)">{{ card?.text || '示例' }}</span>
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
        <el-button @click="toggleTextStylePanel">{{ showTextStylePanel ? '隐藏样式' : '文本样式设置' }}</el-button>
        <!-- 文本样式设置面板 -->
        <div v-show="showTextStylePanel" class="text-style-panel">
          <StyleManager ref="textStyleManager" v-model="editingCard.textStyle" @change="updateEditingCardTextStyle"
            @apply="applyEditingCardTextStyle" />
        </div>
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveCardEdit">确定</el-button>
      </template>
    </el-dialog>

    <!-- 行样式编辑对话框 -->
    <el-dialog v-model="rowStyleDialogVisible" title="行文本样式设置" width="350px" @close="onRowDialogClose">
      <p class="dialog-description">设置将应用于当前行的所有卡片文本</p>

      <StyleManager ref="rowStyleManager" v-model="editingRowStyle.textStyle" @change="updateRowTextStyle"
        @apply="applyRowTextStyle" />

      <template #footer>
        <el-button @click="rowStyleDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveRowStyleEdit">应用到整行</el-button>
      </template>
    </el-dialog>

    <!-- 统一样式设置对话框 -->
    <el-dialog v-model="globalStyleDialogVisible" title="统一样式设置" width="400px" @close="onGlobalStyleDialogClose">
      <p class="dialog-description">设置将应用于所有卡片文本</p>

      <StyleManager ref="globalStyleManager" v-model="globalTextStyles" @change="updateGlobalTextStyle"
        @apply="applyGlobalTextStyle" />

      <template #footer>
        <el-button @click="globalStyleDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveGlobalStyleEdit">应用到所有卡片</el-button>
      </template>
    </el-dialog>
  </div>

  <!-- 隐藏的StyleManager组件，用于获取默认样式值 -->
  <div style="display:none;">
    <StyleManager ref="defaultStyleManagerRef" />
  </div>
</template>
<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, watch, nextTick, computed } from 'vue';
import StyleSettings from './StyleSettings.vue';
import { useEventBus } from '@/utils/eventBus';
import { ElMessage } from 'element-plus';
import StyleManager from './StyleManager.vue'; // 导入StyleManager组件

// 创建事件总线实例
const { on, off, emit } = useEventBus();

// 添加组件加载调试信息
console.log('SimpleCardConverter component initializing');

// 拖动相关数据
const isDragging = ref(false);
const dragOffset = reactive({ x: 0, y: 0 });
const toolbarPosition = reactive({ x: 20, y: 20 }); // 默认位置

// 添加页面尺寸相关数据
const pageSize = ref({ width: 800, height: 600 }); // 默认页面尺寸，像素单位

// 全局文本样式设置
const globalTextStyles = ref({})

// 控制全局样式面板显示
const showGlobalStylePanel = ref(false)

// 控制全局样式对话框显示
const globalStyleDialogVisible = ref(false)

// 全局样式管理器引用
const globalStyleManager = ref(null)

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
  rowSpacing: 10, // 添加行距属性，默认值为10px
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
const showTextStylePanel = ref(false) // 控制文本样式面板显示
const textStyleManager = ref(null) // 文本样式管理器引用
const editingCard = reactive({
  text: '',
  groupIndex: -1,
  rowIndex: -1,
  cardIndex: -1,
  textStyle: {} // 添加文本样式设置
})

// 行样式编辑功能相关
const rowStyleDialogVisible = ref(false)
const rowStyleManager = ref(null) // 行样式管理器引用
const editingRowStyle = reactive({
  groupIndex: -1,
  rowIndex: -1,
  textStyle: {} // 行文本样式设置
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
    border: converterSettings.showBorder ? '0px solid #ddd' : 'none',
    borderRadius: '0px',
    padding: '0px',
    backgroundColor: '#fff',
    // 添加自适应宽度，使其根据内容调整
    width: 'fit-content',
    minWidth: '100%', // 至少填满容器宽度
    // 添加自适应高度
    height: 'fit-content',
    // 确保内容不会被裁剪
    overflow: 'visible'
  }
}

// 获取卡片行样式
const getRowStyle = () => {
  return {
    marginBottom: `${rowSpacing.value}px`
  }
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
  // 添加卡片文本样式设置
  editingCard.textStyle = card.textStyle || {}
  // 重置文本样式面板状态
  showTextStylePanel.value = false
  editDialogVisible.value = true

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    // 创建一个新的对象引用来确保样式正确更新
    const newStyle = JSON.parse(JSON.stringify(editingCard.textStyle || {}))
    editingCard.textStyle = newStyle
  })
}

// 保存卡片编辑
const saveCardEdit = () => {
  const { groupIndex, rowIndex, cardIndex, text, textStyle } = editingCard
  if (groupIndex >= 0 && rowIndex >= 0 && cardIndex >= 0 &&
    cardGroups.value[groupIndex]?.rows[rowIndex]?.cards[cardIndex]) {
    // 更新卡片文本
    cardGroups.value[groupIndex].rows[rowIndex].cards[cardIndex].text = text
    // 更新卡片文本样式
    cardGroups.value[groupIndex].rows[rowIndex].cards[cardIndex].textStyle = { ...textStyle }
    editDialogVisible.value = false
    showTextStylePanel.value = false // 关闭文本样式面板
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
  // 获取全局样式
  const globalStyle = globalTextStyles.value || {};

  const style = {};

  // 字体样式（优先使用全局样式）
  if (globalStyle.fontSize) {
    style.fontSize = `${globalStyle.fontSize}px`;
  } else if (converterSettings.fontSize) {
    style.fontSize = `${converterSettings.fontSize}px`;
  }

  if (globalStyle.color) {
    style.color = globalStyle.color;
  } else if (converterSettings.textColor) {
    style.color = converterSettings.textColor;
  }

  if (globalStyle.fontWeight && globalStyle.fontWeight !== 'normal') {
    style.fontWeight = globalStyle.fontWeight;
  } else if (converterSettings.fontWeight && converterSettings.fontWeight !== 'normal') {
    style.fontWeight = converterSettings.fontWeight;
  }

  if (globalStyle.fontStyle && globalStyle.fontStyle !== 'normal') {
    style.fontStyle = globalStyle.fontStyle;
  } else if (converterSettings.fontStyle && converterSettings.fontStyle !== 'normal') {
    style.fontStyle = converterSettings.fontStyle;
  }

  // 背景样式（优先使用全局样式）
  if (globalStyle.showBackground !== undefined) {
    style.backgroundColor = globalStyle.showBackground ?
      (globalStyle.backgroundColor || 'transparent') : 'transparent';
  } else if (converterSettings.showBackground !== undefined) {
    style.backgroundColor = converterSettings.showBackground ?
      (converterSettings.backgroundColor || 'transparent') : 'transparent';
  } else {
    style.backgroundColor = 'transparent';
  }

  // 布局样式（优先使用全局样式）
  if (globalStyle.textAlign) {
    style.textAlign = globalStyle.textAlign;
  } else if (converterSettings.textAlign) {
    style.textAlign = converterSettings.textAlign;
  }

  if (globalStyle.lineHeight) {
    style.lineHeight = globalStyle.lineHeight;
  } else if (converterSettings.lineHeight) {
    style.lineHeight = converterSettings.lineHeight;
  }

  // 盒模型样式（优先使用全局样式）
  if (globalStyle.padding !== undefined) {
    style.padding = `${globalStyle.padding}px`;
  } else if (converterSettings.padding !== undefined) {
    style.padding = `${converterSettings.padding}px`;
  }

  if (globalStyle.margin !== undefined) {
    style.margin = `${globalStyle.margin}px`;
  } else if (converterSettings.margin !== undefined) {
    style.margin = `${converterSettings.margin}px`;
  }

  // 边框样式（优先使用全局样式）
  if (globalStyle.borderStyle && globalStyle.borderStyle !== 'none') {
    style.borderStyle = globalStyle.borderStyle;
    style.borderWidth = `${globalStyle.borderWidth || 1}px`;
    style.borderColor = globalStyle.borderColor || '#000000';
  } else if (converterSettings.borderStyle && converterSettings.borderStyle !== 'none') {
    style.borderStyle = converterSettings.borderStyle;
    style.borderWidth = `${converterSettings.borderWidth || 1}px`;
    style.borderColor = converterSettings.borderColor || '#000000';
  } else {
    style.border = 'none';
  }

  if (globalStyle.borderRadius) {
    style.borderRadius = `${globalStyle.borderRadius}px`;
  } else if (converterSettings.borderRadius) {
    style.borderRadius = `${converterSettings.borderRadius}px`;
  }

  // 高级样式（优先使用全局样式）
  if (globalStyle.opacity !== undefined && globalStyle.opacity !== 1) {
    style.opacity = globalStyle.opacity;
  } else if (converterSettings.opacity !== undefined && converterSettings.opacity !== 1) {
    style.opacity = converterSettings.opacity;
  }

  style.boxShadow = textStyle.boxShadow || defaultStyle.boxShadow

  // 处理transform属性，优先使用textRotation
  if (textStyle.textRotation !== undefined && textStyle.textRotation !== 0) {
    // 如果有textRotation属性，将其转换为transform
    const scale = textStyle.scale || 1;
    style.transform = `rotate(${textStyle.textRotation}deg) scale(${scale})`;
  } else if (textStyle.transform) {
    // 如果直接提供了transform属性，使用它
    style.transform = textStyle.transform;
  } else if (defaultStyle.transform) {
    // 使用默认的transform
    style.transform = defaultStyle.transform;
  }

  // 为支持transform，需要设置display为inline-block
  if (textStyle.transform || defaultStyle.transform || 
      (textStyle.textRotation !== undefined && textStyle.textRotation !== 0) ||
      textStyle.scale !== undefined) {
    style.display = 'inline-block';
    // 添加will-change属性优化变换性能
    style.willChange = 'transform';
  }

  // 文字特有样式
  if (textStyle.textDecoration && textStyle.textDecoration !== 'none') {
    style.textDecoration = textStyle.textDecoration;
  } else if (defaultStyle.textDecoration !== 'none') {
    style.textDecoration = defaultStyle.textDecoration;
  }

  if (textStyle.textTransform && textStyle.textTransform !== 'none') {
    style.textTransform = textStyle.textTransform;
  } else if (defaultStyle.textTransform !== 'none') {
    style.textTransform = defaultStyle.textTransform;
  }

  style.letterSpacing = textStyle.letterSpacing !== undefined ?
    `${textStyle.letterSpacing}px` : (defaultStyle.letterSpacing ?
      `${defaultStyle.letterSpacing}px` : undefined)

  style.wordSpacing = textStyle.wordSpacing !== undefined ?
    `${textStyle.wordSpacing}px` : (defaultStyle.wordSpacing ?
      `${defaultStyle.wordSpacing}px` : undefined)

  return style;
});

// 对话框关闭处理
const onDialogClose = () => {
  showTextStylePanel.value = false // 关闭文本样式面板
}

// 行样式对话框关闭处理
const onRowDialogClose = () => {
  // 重置编辑状态
  editingRowStyle.groupIndex = -1
  editingRowStyle.rowIndex = -1
  editingRowStyle.textStyle = {}
}

// 切换文本样式面板显示
const toggleTextStylePanel = () => {
  showTextStylePanel.value = !showTextStylePanel.value
  if (showTextStylePanel.value) {
    // 获取当前卡片的索引
    const { groupIndex, rowIndex, cardIndex } = editingCard;

    // 如果有有效的卡片索引
    if (groupIndex >= 0 && rowIndex >= 0 && cardIndex >= 0 &&
      cardGroups.value[groupIndex]?.rows[rowIndex]?.cards[cardIndex]) {

      // 获取当前卡片对象
      const currentCard = cardGroups.value[groupIndex].rows[rowIndex].cards[cardIndex];

      // 获取当前卡片已保存的样式
      const savedStyle = currentCard.textStyle || {};

      // 深拷贝保存的样式
      const styleCopy = JSON.parse(JSON.stringify(savedStyle));

      // 处理fontWeight，确保它不是数值形式
      if (styleCopy.fontWeight === '400') {
        styleCopy.fontWeight = 'normal';
      } else if (styleCopy.fontWeight === '700') {
        styleCopy.fontWeight = 'bold';
      }

      // 处理行高，确保它是合理值
      if (styleCopy.lineHeight > 3 || !styleCopy.lineHeight) {
        styleCopy.lineHeight = 1.2; // 设置为合理的默认行高
      }

      // 设置编辑中的卡片样式
      editingCard.textStyle = styleCopy;
    } else {
      // 如果没有有效的卡片，使用默认样式
      const defaultStyle = getStyleManagerDefaults();
      editingCard.textStyle = JSON.parse(JSON.stringify(defaultStyle));
    }

    // 立即触发更新以显示在UI上
    nextTick(() => {
      if (textStyleManager.value) {
        textStyleManager.value.$forceUpdate();
      }
    });
  }
}

// 强制刷新文本样式
const refreshTextStyle = () => {
  if (textStyleManager.value && showTextStylePanel.value) {
    // 获取当前样式
    const currentStyle = { ...editingCard.textStyle }
    // 重新赋值以触发更新
    editingCard.textStyle = { ...currentStyle }
  }
}

// 更新编辑中的卡片文本样式
const updateEditingCardTextStyle = (style) => {
  editingCard.textStyle = { ...style }
}

// 应用编辑中的卡片文本样式
const applyEditingCardTextStyle = (style) => {
  editingCard.textStyle = { ...style }
}

// 编辑行样式
const editRowStyle = (groupIndex, rowIndex) => {
  const cardGroup = cardGroups.value[groupIndex];
  if (!cardGroup) return;

  const row = cardGroup.rows[rowIndex];
  if (!row) return;

  // 设置当前编辑的行索引
  editingRowStyle.groupIndex = groupIndex;
  editingRowStyle.rowIndex = rowIndex;

  // 尝试获取行中第一个卡片的样式作为初始值
  if (row.cards && row.cards.length > 0) {
    const firstCard = row.cards[0];
    editingRowStyle.textStyle = JSON.parse(JSON.stringify(firstCard.textStyle || {}));
  } else {
    editingRowStyle.textStyle = {}; // 如果没有卡片，使用空对象
  }

  // 显示行样式对话框
  rowStyleDialogVisible.value = true;

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    if (rowStyleManager.value) {
      rowStyleManager.value.$forceUpdate();
    }
  });
}

// 更新行文本样式
const updateRowTextStyle = (style) => {
  editingRowStyle.textStyle = { ...style };
}

// 应用行文本样式
const applyRowTextStyle = (style) => {
  editingRowStyle.textStyle = { ...style };
}

// 保存行样式编辑
const saveRowStyleEdit = () => {
  const { groupIndex, rowIndex, textStyle } = editingRowStyle;

  if (groupIndex >= 0 && rowIndex >= 0 && cardGroups.value[groupIndex]?.rows[rowIndex]) {
    const row = cardGroups.value[groupIndex].rows[rowIndex];

    // 将样式应用到行中的所有卡片
    if (row.cards && row.cards.length > 0) {
      row.cards.forEach(card => {
        card.textStyle = JSON.parse(JSON.stringify(textStyle)); // 深拷贝应用样式
      });

      // 关闭对话框
      rowStyleDialogVisible.value = false;

      // 提示成功
      ElMessage.success(`已将样式应用到第${rowIndex + 1}行的${row.cards.length}个卡片`);

      // 触发更新
      updateStyle();
    }
  }
}

// 创建一个全局默认样式管理器引用
// 这个StyleManager组件不会被渲染，仅用于获取默认样式值
const defaultStyleManagerRef = ref(null);

// 获取样式管理器中的默认样式
const getStyleManagerDefaults = () => {
  try {
    // 如果样式管理器引用存在，从中获取localStyle
    if (defaultStyleManagerRef.value && defaultStyleManagerRef.value.localStyle) {
      return defaultStyleManagerRef.value.localStyle;
    }
  } catch (err) {
    console.error('无法获取StyleManager默认样式', err);
  }

  // 如果获取失败，返回一个基础默认值
  return {
    fontSize: 16,
    color: '#000000',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'left',
    lineHeight: 1.2,
    backgroundColor: '#ffffff',
    opacity: 1,
    padding: 0,
    margin: 0,
    borderStyle: 'none',
    borderWidth: 1,
    borderColor: '#000000',
    borderRadius: 0,
    textRotation: 0,
    scale: 1
  };
};

// 获取卡片文本样式
const getCardTextStyle = (card) => {
  // 添加空值检查
  if (!card) {
    return {}
  }

  // 获取卡片文本样式
  const textStyle = card.textStyle || {}

  // 获取StyleManager的默认样式
  // 从defaultStyleManagerRef中获取localStyle作为默认样式
  const defaultStyle = getStyleManagerDefaults();

  // 构建样式对象
  const style = {}

  // 字体样式
  style.fontSize = textStyle.fontSize !== undefined ?
    `${textStyle.fontSize}px` : `${defaultStyle.fontSize}px`

  style.color = textStyle.color || defaultStyle.color

  if (textStyle.fontWeight && textStyle.fontWeight !== 'normal') {
    style.fontWeight = textStyle.fontWeight
  } else if (defaultStyle.fontWeight !== 'normal') {
    style.fontWeight = defaultStyle.fontWeight
  }

  if (textStyle.fontStyle && textStyle.fontStyle !== 'normal') {
    style.fontStyle = textStyle.fontStyle
  } else if (defaultStyle.fontStyle !== 'normal') {
    style.fontStyle = defaultStyle.fontStyle
  }

  // 背景样式
  style.backgroundColor = textStyle.backgroundColor || defaultStyle.backgroundColor

  // 布局样式
  style.textAlign = textStyle.textAlign || defaultStyle.textAlign

  style.lineHeight = textStyle.lineHeight !== undefined ?
    textStyle.lineHeight : defaultStyle.lineHeight

  // 盒模型样式
  style.padding = textStyle.padding !== undefined ?
    `${textStyle.padding}px` : `${defaultStyle.padding}px`

  style.margin = textStyle.margin !== undefined ?
    `${textStyle.margin}px` : `${defaultStyle.margin}px`

  // 边框样式
  if (textStyle.borderStyle && textStyle.borderStyle !== 'none') {
    style.borderStyle = textStyle.borderStyle
    style.borderWidth = `${textStyle.borderWidth || defaultStyle.borderWidth}px`
    style.borderColor = textStyle.borderColor || defaultStyle.borderColor
  } else if (defaultStyle.borderStyle !== 'none') {
    style.borderStyle = defaultStyle.borderStyle
    style.borderWidth = `${defaultStyle.borderWidth}px`
    style.borderColor = defaultStyle.borderColor
  } else {
    style.border = 'none'
  }

  style.borderRadius = textStyle.borderRadius !== undefined ?
    `${textStyle.borderRadius}px` : `${defaultStyle.borderRadius}px`

  // 高级样式
  if (textStyle.opacity !== undefined && textStyle.opacity !== 1) {
    style.opacity = textStyle.opacity
  } else if (defaultStyle.opacity !== 1) {
    style.opacity = defaultStyle.opacity
  }

  style.boxShadow = textStyle.boxShadow || defaultStyle.boxShadow

  style.transform = textStyle.transform || defaultStyle.transform

  // 为支持transform，需要设置display为inline-block
  if (textStyle.transform || defaultStyle.transform) {
    style.display = 'inline-block'
    // 添加will-change属性优化变换性能
    style.willChange = 'transform'
  }

  // 文字特有样式
  if (textStyle.textDecoration && textStyle.textDecoration !== 'none') {
    style.textDecoration = textStyle.textDecoration
  } else if (defaultStyle.textDecoration !== 'none') {
    style.textDecoration = defaultStyle.textDecoration
  }

  if (textStyle.textTransform && textStyle.textTransform !== 'none') {
    style.textTransform = textStyle.textTransform
  } else if (defaultStyle.textTransform !== 'none') {
    style.textTransform = defaultStyle.textTransform
  }

  style.letterSpacing = textStyle.letterSpacing !== undefined ?
    `${textStyle.letterSpacing}px` : (defaultStyle.letterSpacing ?
      `${defaultStyle.letterSpacing}px` : undefined)

  style.wordSpacing = textStyle.wordSpacing !== undefined ?
    `${textStyle.wordSpacing}px` : (defaultStyle.wordSpacing ?
      `${defaultStyle.wordSpacing}px` : undefined)

  return style
}

// 切换全局样式面板显示
const toggleGlobalStylePanel = () => {
  showGlobalStylePanel.value = !showGlobalStylePanel.value
}

// 打开全局样式设置面板
const openGlobalStylePanel = () => {
  // 深拷贝当前全局样式或使用默认样式
  const currentGlobalStyle = globalTextStyles.value || {}
  globalTextStyles.value = JSON.parse(JSON.stringify(currentGlobalStyle))
  globalStyleDialogVisible.value = true

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    if (globalStyleManager.value) {
      globalStyleManager.value.$forceUpdate();
    }
  });
}

// 更新全局文本样式
const updateGlobalTextStyle = (style) => {
  globalTextStyles.value = { ...style };
}

// 应用全局文本样式
const applyGlobalTextStyle = (style) => {
  globalTextStyles.value = { ...style };
}

// 保存全局样式编辑
const saveGlobalStyleEdit = () => {
  // 创建一个样式副本
  const globalStyleCopy = JSON.parse(JSON.stringify(globalTextStyles.value));
  
  // 如果有textRotation属性，构造transform属性
  if (globalStyleCopy.textRotation !== undefined && globalStyleCopy.textRotation !== 0) {
    const scale = globalStyleCopy.scale || 1;
    globalStyleCopy.transform = `rotate(${globalStyleCopy.textRotation}deg) scale(${scale})`;
  }

  // 将样式应用到所有卡片
  cardGroups.value.forEach(group => {
    group.rows.forEach(row => {
      if (row.cards && row.cards.length > 0) {
        row.cards.forEach(card => {
          card.textStyle = JSON.parse(JSON.stringify(globalStyleCopy));
        });
      }
    });
  });

  // 关闭对话框
  globalStyleDialogVisible.value = false;

  // 提示成功
  ElMessage.success('已将样式应用到所有卡片');

  // 触发更新
  updateStyle();
}

// 全局样式对话框关闭处理
const onGlobalStyleDialogClose = () => {
  // 重置编辑状态
  globalTextStyles.value = {}
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
  background: transparent;
  position: relative;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.cards-display-area {
  padding: 0px;
  width: 100%;
  box-sizing: border-box;
  overflow: hidden;
  scroll-behavior: smooth;
  background: transparent;
  max-height: calc(100vh - 120px);
  /* 添加自适应布局 */
  display: flex;
  flex-direction: column;
  align-items: center;
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
  /* 移除固定的宽度绑定，使用更灵活的自适应方式 */
  width: fit-content;
  min-width: 100%;
  margin: 0 auto v-bind('converterSettings.rowSpacing + "px"') auto;
  position: relative;
  padding: 0px;
  /* 添加自适应高度 */
  height: fit-content;
  /* 确保内容不会被裁剪 */
  overflow: visible;
  box-sizing: border-box;
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
  /* 使用更灵活的宽度 */
  width: 100%;
  min-width: fit-content;
  box-sizing: border-box;
  /* 允许内容溢出以适应变换后的元素 */
  overflow: visible;
  scroll-behavior: smooth;
  background: transparent;
  max-height: calc(100vh - 120px);
}

.card-row {
  display: flex;
  /* 允许换行以适应大尺寸变换后的文本 */
  flex-wrap: wrap;
  /* 设置容器尺寸跟随内容变化 */
  width: 100%;
  height: fit-content;
  margin: 0;
  gap: v-bind('converterSettings.spacing + "px"');
  /* 使用间距设置 */
  /* 添加以下样式以适应变换后的文本 */
  align-items: flex-start;
  padding: 0px;
  /* 确保行容器不会裁剪内容 */
  overflow: visible;
  /* 移除固定最小高度，让容器完全根据内容自适应 */
  min-height: unset;
  /* 确保行内元素正确对齐 */
  align-content: flex-start;
  /* 根据子元素实际尺寸进行布局 */
  align-self: flex-start;
  /* 为变换后的元素预留空间 */
  align-items: stretch;
}

.text-card-container {
  /* 为旋转和缩放后的文本预留足够空间 */
  display: flex;
  align-items: center;
  justify-content: center;
  /* 设置容器尺寸跟随内容变化 */
  width: fit-content;
  height: fit-content;
  /* 添加相对定位，确保变换基于容器中心 */
  position: relative;
  /* 减小内边距，更精确地适应文本 */
  padding: 0px;
  /* 确保容器本身不会阻挡变换效果 */
  overflow: visible;
  /* 设置容器的变换原点，使其在布局中更居中 */
  transform-origin: center center;
  /* 允许容器根据内容调整 */
  flex: 0 0 auto;
  /* 添加以下样式以更好地适应变换 */
  box-sizing: content-box;
  /* 确保容器在布局中正确对齐 */
  align-self: flex-start;
  /* 为变换元素创建新的层叠上下文 */
  isolation: isolate;
  /* 确保容器为内容提供足够的空间 */
  min-width: 2em;
  min-height: 2em;
}

.card-text {
  line-height: 1;
  user-select: none;
  transform-origin: center center;
  white-space: pre;
  /* 添加以下样式以适应变换后的文本 */
  display: inline-block;
  /* 确保文本本身不会被裁剪 */
  overflow: visible;
  /* 添加z-index确保文本在顶层显示 */
  z-index: 1;
  /* 确保文本不会换行 */
  white-space: nowrap;
  /* 为变换添加过渡效果 */
  transition: transform 0.3s ease;
  /* 添加will-change属性优化性能 */
  will-change: transform;
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

.global-style-settings {
  margin: 10px 0;
}

.global-style-panel {
  margin-top: 10px;
  border: 1px solid #ebeef5;
  border-radius: 4px;
  padding: 10px;
  max-height: 300px;
  overflow-y: auto;
}

/* 文本样式面板样式 */
.text-style-panel {
  margin-top: 10px;
  border: 1px solid #ebeef5;
  border-radius: 4px;
  padding: 10px;
  max-height: 300px;
  overflow-y: auto;
}

/* 行样式交互样式 */
.card-row {
  cursor: pointer;
  position: relative;
}

.card-row:hover::before {
  content: "";
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(64, 158, 255, 0.1);
  border: 1px dashed #409eff;
  pointer-events: none;
  z-index: 0;
}

/* 行样式对话框内容样式 */
.dialog-description {
  font-size: 14px;
  color: #606266;
  margin-bottom: 15px;
  padding-bottom: 10px;
  border-bottom: 1px dashed #ebeef5;
}

/* 移除已删除的样式 */
/*
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
*/
</style>