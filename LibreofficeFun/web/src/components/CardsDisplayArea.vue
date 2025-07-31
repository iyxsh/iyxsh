<template>
  <div class="cards-display-area">
    <div class="card-group" v-for="(group, groupIndex) in cardGroups" :key="group.id || groupIndex"
      :style="getCardGroupStyle(groupIndex)">
      <div v-for="(row, rowIndex) in group.rows" :key="'row-' + (group.id || groupIndex) + '-' + rowIndex"
        class="card-row" :class="row.type" @click.self="editRowStyle(groupIndex, rowIndex)"
        :data-row-id="`${groupIndex}-${rowIndex}`" :style="getRowStyle(groupIndex, rowIndex)">
        <div v-for="(card, cardIndex) in row.cards"
          :key="'card-' + (group.id || groupIndex) + '-' + rowIndex + '-' + cardIndex"
          @click="editCard(groupIndex, rowIndex, cardIndex)" :class="card.type"
          :data-card-id="`${groupIndex}-${rowIndex}-${cardIndex}`" class="text-card-container"
          :style="getCardContainerStyle(card)">
          <span class="card-text" :style="getCardTextStyle(card)">{{ card?.text || '示例' }}</span>
        </div>
      </div>
    </div>
    <!-- 当没有卡片时显示提示 -->
    <div v-if="!cardGroups || cardGroups.length === 0" class="no-cards-placeholder">
      <p>暂无卡片内容</p>
      <p class="hint">请先在表单页面添加表单，然后转换为卡片</p>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, nextTick } from 'vue';
import { ElMessage } from 'element-plus';

// 使用v-model定义数据
const cardGroups = defineModel('cardGroups', { type: Array, default: () => [] });
const cardGroupStyles = defineModel('cardGroupStyles', { type: Object, default: () => ({}) });
const cardRowStyles = defineModel('cardRowStyles', { type: Object, default: () => ({}) });
const cardStyles = defineModel('cardStyles', { type: Object, default: () => ({}) });
const globalTextStyles = defineModel('globalTextStyles', { type: Object, default: () => ({}) });

// 定义要向父组件发出的事件
const emit = defineEmits([
  'edit-card',
  'edit-row-style',
  'save-card-edit',
  'save-row-style-edit',
  'update:cardGroups'
]);

// 定义props
const props = defineProps({
  defaultStyleManagerRef: {
    type: Object,
    default: () => null
  },
  pageSize: {
    type: Object,
    default: () => ({ width: 800, height: 600 })
  }
});

// 定义组件暴露的方法
defineExpose({
  // 移除了与样式面板相关的暴露方法，因为这些功能已移到浮动工具条组件中
});

// 导入默认样式配置
import { defaultTextStyles } from '../utils/styleConfig.ts';

// 获取卡片组样式
const getCardGroupStyle = (groupIndex) => {
  // 首先尝试获取特定卡片组的样式（如果支持）
  const cardGroup = cardGroups.value[groupIndex];
  if (cardGroup && cardGroup.style) {
    // 如果卡片组有特定样式，返回该样式
    if (cardGroup.style._css) {
      return Object.assign({}, cardGroup.style._css);
    } else {
      return Object.assign({}, cardGroup.style);
    }
  }

  // 默认返回全局卡片组样式
  const style = {};

  // 如果有已处理的CSS样式，优先使用
  if (cardGroupStyles.value && cardGroupStyles.value._css) {
    Object.assign(style, cardGroupStyles.value._css);
  } else if (cardGroupStyles.value) {
    Object.assign(style, cardGroupStyles.value);
  }

  return style;
};

// 获取卡片行样式
const getRowStyle = (groupIndex, rowIndex) => {
  // 首先尝试获取特定行的样式
  const cardGroup = cardGroups.value[groupIndex];
  if (cardGroup && cardGroup.rows && cardGroup.rows[rowIndex]) {
    const row = cardGroup.rows[rowIndex];
    // 如果行有特定样式，返回该样式
    if (row.style) {
      // 如果有已处理的CSS样式，优先使用
      if (row.style._css) {
        return Object.assign({}, row.style._css);
      } else if (row.style) {
        return Object.assign({}, row.style);
      }
    }
  }

  // 默认返回全局行样式
  const style = {}; // 使用卡片组样式
  // 如果有已处理的CSS样式，优先使用
  if (cardRowStyles.value && cardRowStyles.value._css) {
    Object.assign(style, cardRowStyles.value._css);
  } else if (cardRowStyles.value) {
    Object.assign(style, cardRowStyles.value);
  }

  return style;
};

// 获取单个卡片容器样式
const getCardContainerStyle = (card) => {
  const style = {};

  // 如果卡片有特定的容器样式，优先使用
  if (card && card.style) {
    // 如果有已处理的CSS样式，优先使用
    if (card.style._css) {
      return Object.assign({}, card.style._css);
    } else {
      return Object.assign({}, card.style);
    }
  }

  // 如果有全局单卡片样式设置，作为默认值使用
  if (cardStyles.value) {
    // 如果有已处理的CSS样式，优先使用
    if (cardStyles.value._css) {
      Object.assign(style, cardStyles.value._css);
    } else {
      Object.assign(style, cardStyles.value);
    }
  }

  return style;
};

// 编辑卡片
const editCard = (groupIndex, rowIndex, cardIndex) => {
  // 现在需要三个索引：组索引、行索引和卡片索引
  const cardGroup = cardGroups.value[groupIndex];
  if (!cardGroup) return;

  const row = cardGroup.rows[rowIndex];
  if (!row) return;

  const card = row.cards[cardIndex];
  if (!card) return;

  // 向父组件发出事件
  emit('edit-card', { groupIndex, rowIndex, cardIndex, card });
};

// 编辑行样式
const editRowStyle = (groupIndex, rowIndex) => {
  const cardGroup = cardGroups.value[groupIndex];
  if (!cardGroup) return;

  const row = cardGroup.rows[rowIndex];
  if (!row) return;

  // 向父组件发出事件
  emit('edit-row-style', { groupIndex, rowIndex });
};

// 获取卡片文本样式
const getCardTextStyle = (card) => {
  // 添加空值检查
  if (!card) {
    return {};
  }

  const style = {};
  console.log('card.textStyle', card.textStyle);
  // 如果卡片有特定的容器样式，优先使用
  if (card && card.textStyle) {
    // 如果有已处理的CSS样式，优先使用
    if (card.textStyle._css) {
      return Object.assign({}, card.textStyle._css);
    } else {
      return Object.assign({}, card.textStyle);
    }
  }

  // 如果有全局单卡片样式设置，作为默认值使用
  if (globalTextStyles.value) {
    // 如果有已处理的CSS样式，优先使用
    if (globalTextStyles.value._css) {
      Object.assign(style, globalTextStyles.value._css);
    } else {
      Object.assign(style, globalTextStyles.value);
    }
  }
  console.log('style', style);
  return style;
/*
console.log('card.textStyle',card.textStyle);
  // 获取卡片文本样式
  const textStyle = card.textStyle || {};

  // 使用StyleManager的toCSS方法来转换样式
  // 创建一个临时的StyleManager实例来处理样式转换
  const tempStyleManager = props.defaultStyleManagerRef;
  if (tempStyleManager && tempStyleManager.localStyle && typeof tempStyleManager.toCSS === 'function') {
    try {
      // 保存原始localStyle
      const originalLocalStyle = { ...tempStyleManager.localStyle };

      // 将textStyle应用到临时的StyleManager上
      if (originalLocalStyle && textStyle) {
        Object.assign(tempStyleManager.localStyle, textStyle);

        // 获取转换后的CSS（仅包含可见样式）
        const css = tempStyleManager.toCSS();

        // 恢复原始样式
        Object.assign(tempStyleManager.localStyle, originalLocalStyle);

        // 特殊处理transform属性，因为卡片可能有独立的旋转和缩放设置
        // 只有当textRotation或scale在样式中定义时才应用变换
        if (('textRotation' in textStyle && textStyle.textRotation !== 0) ||
          ('scale' in textStyle && textStyle.scale !== 1)) {
          const rotation = textStyle.textRotation || 0;
          const scale = textStyle.scale !== undefined ? textStyle.scale : 1;
          css.transform = `rotate(${rotation}deg) scale(${scale})`;
        }

        // 为支持transform，需要设置display为inline-block
        if (css.transform ||
          ('textRotation' in textStyle && textStyle.textRotation !== 0) ||
          ('scale' in textStyle && textStyle.scale !== 1)) {
          css.display = 'inline-block';
          // 添加will-change属性优化变换性能
          css.willChange = 'transform';
        }
console.log('styleManagerToCSS', css);
        return css;
      }
    } catch (error) {
      console.error('Error processing card text style with StyleManager:', error);
      // 如果出错，继续使用回退方案
    }
  }

  // 如果无法使用StyleManager，则回退到原来的手动处理方式
  // 获取文本样式默认值
  const defaultStyle = getTextStyleDefaults();

  // 构建样式对象
  const style = {};

  // 字体样式
  style.fontSize = textStyle.fontSize !== undefined ?
    `${textStyle.fontSize}px` : `${defaultStyle.fontSize}px`;

  style.color = textStyle.color || defaultStyle.color;

  if (textStyle.fontWeight && textStyle.fontWeight !== 'normal') {
    style.fontWeight = textStyle.fontWeight;
  } else if (defaultStyle.fontWeight !== 'normal') {
    style.fontWeight = defaultStyle.fontWeight;
  }

  if (textStyle.fontStyle && textStyle.fontStyle !== 'normal') {
    style.fontStyle = textStyle.fontStyle;
  } else if (defaultStyle.fontStyle !== 'normal') {
    style.fontStyle = defaultStyle.fontStyle;
  }

  // 背景样式
  style.backgroundColor = textStyle.backgroundColor || defaultStyle.backgroundColor;

  // 布局样式
  style.textAlign = textStyle.textAlign || defaultStyle.textAlign;

  style.lineHeight = textStyle.lineHeight !== undefined ?
    textStyle.lineHeight : defaultStyle.lineHeight;

  // 盒模型样式
  style.padding = textStyle.padding !== undefined ?
    `${textStyle.padding}px` : `${defaultStyle.padding}px`;

  style.margin = textStyle.margin !== undefined ?
    `${textStyle.margin}px` : `${defaultStyle.margin}px`;

  // 边框样式
  if (textStyle.borderStyle && textStyle.borderStyle !== 'none') {
    style.borderStyle = textStyle.borderStyle;
    style.borderWidth = `${textStyle.borderWidth || defaultStyle.borderWidth}px`;
    style.borderColor = textStyle.borderColor || defaultStyle.borderColor;
  } else if (defaultStyle.borderStyle !== 'none') {
    style.borderStyle = defaultStyle.borderStyle;
    style.borderWidth = `${defaultStyle.borderWidth}px`;
    style.borderColor = defaultStyle.borderColor;
  } else {
    style.border = 'none';
  }

  style.borderRadius = textStyle.borderRadius !== undefined ?
    `${textStyle.borderRadius}px` : `${defaultStyle.borderRadius}px`;

  // 高级样式
  if (textStyle.opacity !== undefined && textStyle.opacity !== 1) {
    style.opacity = textStyle.opacity;
  } else if (defaultStyle.opacity !== 1) {
    style.opacity = defaultStyle.opacity;
  }

  style.boxShadow = textStyle.boxShadow || defaultStyle.boxShadow;

  // 处理transform属性，优先使用textRotation
  if (('textRotation' in textStyle && textStyle.textRotation !== 0) ||
    ('scale' in textStyle && textStyle.scale !== 1)) {
    // 如果有textRotation属性，将其转换为transform
    const rotation = textStyle.textRotation || 0;
    const scale = textStyle.scale !== undefined ? textStyle.scale : 1;
    style.transform = `rotate(${rotation}deg) scale(${scale})`;
  } else if (textStyle.transform) {
    // 如果直接提供了transform属性，使用它
    style.transform = textStyle.transform;
  } else if (defaultStyle.transform) {
    // 使用默认的transform
    style.transform = defaultStyle.transform;
  }

  // 为支持transform，需要设置display为inline-block
  if (textStyle.transform || defaultStyle.transform ||
    ('textRotation' in textStyle && textStyle.textRotation !== 0) ||
    ('scale' in textStyle && textStyle.scale !== 1)) {
    style.display = 'inline-block';
    // 添加will-change属性优化性能
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
      `${defaultStyle.letterSpacing}px` : undefined);

  style.wordSpacing = textStyle.wordSpacing !== undefined ?
    `${textStyle.wordSpacing}px` : (defaultStyle.wordSpacing ?
      `${defaultStyle.wordSpacing}px` : undefined);
console.log('style', style);
  return style;
  */
};

// 获取文本样式默认值
const getTextStyleDefaults = () => {
  try {
    // 如果样式管理器引用存在，从中获取文本样式默认值
    if (props.defaultStyleManagerRef && props.defaultStyleManagerRef.localTextStyle) {
      return { ...props.defaultStyleManagerRef.localTextStyle };
    }
  } catch (err) {
    console.error('无法获取StyleManager文本样式默认值', err);
  }

  // 如果获取失败，回退到基础默认值
  return { ...defaultTextStyles };
};
</script>

<style scoped>
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