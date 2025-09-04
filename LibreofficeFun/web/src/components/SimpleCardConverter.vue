<template>
  <div class="simple-card-converter" :style="containerStyle">
    <!-- 浮动工具条 -->
    <FloatingToolbar 
      ref="floatingToolbarRef"
      v-model:cardGroups="cardGroups" 
      v-model:cardGroupStyles="cardGroupStyles"
      v-model:cardRowStyles="cardRowStyles" 
      v-model:cardStyles="cardStyles" 
      v-model:globalTextStyles="globalTextStyles"
      :defaultStyleManagerRef="defaultStyleManagerRef" 
      @save-data-to-server="saveCardDataToServer" />
    <CardsDisplayArea 
      v-model:cardGroups="cardGroups" 
      v-model:cardGroupStyles="cardGroupStyles"
      v-model:cardRowStyles="cardRowStyles" 
      v-model:cardStyles="cardStyles" 
      v-model:globalTextStyles="globalTextStyles"
      :defaultStyleManagerRef="defaultStyleManagerRef" 
      :page-size="{ width: 800, height: 600 }" 
      @edit-card="componentEmit('edit-card', $event)"
      @edit-row-style="componentEmit('edit-row-style', $event)" />
  </div>
</template>
<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, watch, nextTick, computed } from 'vue';
import { useEventBus } from '@/utils/eventBus';
import { ElMessage } from 'element-plus';
import CardsDisplayArea from './CardsDisplayArea.vue'; // 导入CardsDisplayArea组件
import FloatingToolbar from './FloatingToolbar.vue'; // 导入FloatingToolbar组件
import ApiService, { callApi } from '../services/ApiService';
// 修复导入语句，明确指定.ts扩展名

// 创建事件总线实例
const { on, off, emit: eventBusEmit } = useEventBus();

// 添加组件加载调试信息
console.log('SimpleCardConverter component initializing');

// 添加页面尺寸相关数据
const pageSize = ref({ width: 800, height: 600 }); // 默认页面尺寸，像素单位

const floatingToolbarRef = ref(null); // 添加浮动工具栏引用定义

const defaultStyleManagerRef = ref({});

// 单个卡片样式编辑功能相关
const editingCardStyle = reactive({
  groupIndex: -1,
  rowIndex: -1,
  cardIndex: -1,
  textStyle: {} // 单个卡片文本样式设置
})

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

// 使用新定义的接口类型定义响应式数据 注意此处CardGroupStyles等在 Vue 文件中可以通过 defineModel 的类型推导自动获取，不需要显式导入这些类型。显示导入会报错
// 从styleConfig.ts导入CardGroups类型
// 使用导入的CardGroups类型重新定义cardGroups

// 使用 defineModel 替代 ref，以保持响应式集成和 TypeScript 支持
const cardGroups = defineModel('cardGroups', { required: true, type: Array, default: () => [] });
const cardGroupStyles = defineModel('cardGroupStyles', { required: true, type: Object, default: () => ({}) });
const cardRowStyles = defineModel('cardRowStyles', { required: true, type: Object, default: () => ({}) });
const cardStyles = defineModel('cardStyles', { required: true, type: Object, default: () => ({}) });
const globalTextStyles = defineModel('globalTextStyles', { required: true, type: Object, default: () => ({}) });


// 定义组件需要触发的事件
const componentEmit = defineEmits([
  'edit-card',
  'edit-row-style'
]);

// 在组件挂载时注册事件
onMounted(() => {
  // 监听页面尺寸变化事件
  on('page-size-changed', handlePageSizeChange)

  // 如果有传入的表单数据，自动进行转换
  if (props.forms && props.forms.length > 0) {
    console.log('[SimpleCardConverter] 检测到传入的表单数据，自动进行转换');
    convertFormsToCards(props.forms);
  }

  // 从后端加载数据
  //loadCardDataFromServer();
})

// 从后端加载卡片数据
const loadCardDataFromServer = async () => {
  try {
    const cardGroupsData = await callApi(ApiService.CardGroups);
    if (cardGroupsData && cardGroupsData.length > 0) {
      cardGroups.value = cardGroupsData;
    }

    const styleConfig = await callApi(ApiService.StyleConfig);
    if (styleConfig) {
      if (styleConfig.cardGroupStyles) {
        cardGroupStyles.value = styleConfig.cardGroupStyles;
      }
      if (styleConfig.cardRowStyles) {
        cardRowStyles.value = styleConfig.cardRowStyles;
      }
      if (styleConfig.cardStyles) {
        cardStyles.value = styleConfig.cardStyles;
      }
      if (styleConfig.globalTextStyles) {
        globalTextStyles.value = styleConfig.globalTextStyles;
      }
    }

    ElMessage.success('数据加载成功');
  } catch (error) {
    console.error('加载数据失败:', error);
    ElMessage.error('数据加载失败: ' + error.message);
  }
};

// 保存卡片数据到后端
const saveCardDataToServer = async () => {
  try {
    await callApi(ApiService.saveCardGroups, cardGroups.value);

    const styleConfig = {
      cardGroupStyles: cardGroupStyles.value,
      cardRowStyles: cardRowStyles.value,
      cardStyles: cardStyles.value,
      globalTextStyles: globalTextStyles.value
    };
    await callApi(ApiService.saveStyleConfig, styleConfig);

    ElMessage.success('数据保存成功');
  } catch (error) {
    console.error('保存数据失败:', error);
    ElMessage.error('数据保存失败: ' + error.message);
  }
};

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

// 添加页面尺寸处理方法
const setPageSize = (size) => {
  if (size) {
    // 通知其他组件页面尺寸已更改
    eventBusEmit('page-size-changed', size)
  }
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

// 添加 handlePageSizeChange 方法
const handlePageSizeChange = (size) => {
  if (size && size.width && size.height) {
    // 根据页面尺寸调整字体大小
    const baseSize = Math.min(size.width, size.height)
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
        // 使用StyleManager的默认样式
        textStyle: {},
        style: {} // 使用单卡片样式
      }))
      rows.push({
        type: 'title-row',
        cards: titleCards,
        style: {} // 使用单行样式
      });
    }

    // 处理表单内容（如果有且不为空）
    if (form.value && form.value.trim()) {
      // 为内容中的每个字符创建单独的卡片，并将它们放在同一行
      const valueChars = form.value.trim().split('')
      const valueCards = valueChars.map(char => ({
        text: char,
        type: 'value',
        // 使用StyleManager的默认样式
        textStyle: {},
        style: {} // 使用单卡片样式
      }))
      rows.push({
        type: 'value-row',
        cards: valueCards,
        style: {} // 使用单行样式
      });
    }

    // 处理表单备注（如果有且不为空）
    if (form.remark && form.remark.trim()) {
      // 为备注中的每个字符创建单独的卡片，并将它们放在同一行
      const remarkChars = form.remark.trim().split('')
      const remarkCards = remarkChars.map(char => ({
        text: char,
        type: 'remark',
        // 使用StyleManager的默认样式
        textStyle: {},
        style: {} // 使用单卡片样式
      }))
      rows.push({
        type: 'remark-row',
        cards: remarkCards,
        style: {} // 使用单行样式
      });
    }

    // 如果没有内容，创建一个默认行
    if (rows.length === 0) {
      rows.push({
        type: 'empty-row',
        cards: [{
          text: '',
          type: 'empty',
          // 使用StyleManager的默认样式
          textStyle: {},
          style: {} // 使用单卡片样式
        }]
      });
    }

    return {
      id: form.id || `group-${Date.now()}-${index}`,
      title: form.title || `表单 ${index + 1}`,
      rows,
      style: {} // 使用卡片组样式
    }
  })

  cardGroups.value = newGroups
  nextTick(() => {
    updateStyle()
    ElMessage.success(`成功将 ${validForms.length} 个表单转换为卡片`)

    // 保存到后端
    saveCardDataToServer();
  })
}

// 添加资源清理
function cleanupResources() {
  // 清理内存检查
  cleanupTasks.forEach(task => task())
}

// defineExpose is a compiler macro and no longer needs to be imported

// 暴露方法
defineExpose({
  convertFormsToCards,
  setPageSize,
  saveCardDataToServer,
  loadCardDataFromServer,
  floatingToolbarRef // 暴露浮动工具栏引用
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

.card-group {
  /* 移除固定宽度，让容器根据内容自适应 */
  width: fit-content;
  min-width: 100%;
  /* 添加自适应高度 */
  height: fit-content;
  /* 确保内容不会被裁剪 */
  overflow: visible;
  /* 确保容器能够适应子元素的变换 */
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

.card-row {
  display: flex;
  /* 允许换行以适应大尺寸变换后的文本 */
  flex-wrap: wrap;
  /* 设置容器尺寸跟随内容变化 */
  width: 100%;
  height: fit-content;
  margin: 0;
  gap: 2px;
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
  /* 移除固定最小尺寸，让容器完全根据内容自适应 */
  min-width: fit-content;
  min-height: fit-content;
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
  /* 确保文本在容器中居中 */
  text-align: center;
  /* 添加一些内边距使文本不会贴边 */
  padding: 0px;
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
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.global-style-settings .el-button {
  flex: 1;
  min-width: 120px;
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
</style>