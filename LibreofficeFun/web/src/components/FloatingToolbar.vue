<template>
  <div class="floating-toolbar" :style="toolbarStyle" @mousedown="startDrag">
    <div class="toolbar-header">
      <h3>全局样式设置</h3>
      <div class="drag-handle">⋮⋮</div>
    </div>
    <!-- 添加card-text样式管理器 -->
    <div class="global-style-settings">
      <el-button @click="openGlobalStylePanel" size="small" type="primary">
        全局文本样式设置
      </el-button>
      <el-button @click="openCardStylePanel" size="small" type="primary">
        单卡片样式设置
      </el-button>
      <el-button @click="openCardRowStylePanel" size="small" type="primary">
        卡片行样式设置
      </el-button>
      <el-button @click="openCardGroupStylePanel" size="small" type="primary">
        卡片组样式设置
      </el-button>
    </div>
    <div class="toolbar-content">
      <!-- 卡片样式模板选择器 -->
      <div class="style-template-selector">
        <div class="selector-group">
          <span class="control-label">单卡片样式模板：</span>
          <el-select 
            v-model="selectedCardStyle"
            placeholder="选择单卡片样式模板" 
            size="small"
            @change="(val) => handleCardStyleTemplateChange('card', val)">
            <el-option 
              v-for="option in cardStyleTemplates" 
              :key="option.value" 
              :label="option.label"
              :value="option.value" />
          </el-select>
        </div>

        <div class="selector-group">
          <span class="control-label">卡片行样式模板：</span>
          <el-select 
            v-model="selectedCardRowStyle"
            placeholder="选择卡片行样式模板" 
            size="small"
            @change="(val) => handleCardStyleTemplateChange('row', val)">
            <el-option 
              v-for="option in cardRowStyleTemplates" 
              :key="option.value" 
              :label="option.label"
              :value="option.value" />
          </el-select>
        </div>

        <div class="selector-group">
          <span class="control-label">卡片组样式模板：</span>
          <el-select 
            v-model="selectedCardGroupStyle"
            placeholder="选择卡片组样式模板" 
            size="small"
            @change="(val) => handleCardStyleTemplateChange('group', val)">
            <el-option 
              v-for="option in cardGroupStyleTemplates" 
              :key="option.value" 
              :label="option.label"
              :value="option.value" />
          </el-select>
        </div>
      </div>
    </div>
  </div>

  <!-- 编辑卡片对话框 -->
  <el-dialog v-model="editDialogVisible" title="编辑卡片" width="300px" @close="onDialogClose">
    <!-- 添加空值检查和默认值 -->
    <el-input v-model="editingCard.text" maxlength="10" show-word-limit :value="editingCard.text || ''"
      @input="value => editingCard.text = value" />

    <template #footer>
      <el-button @click="toggleTextStylePanel">{{ showTextStylePanel ? '隐藏样式设置' : '文本样式设置' }}</el-button>
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

    <StyleManager 
      ref="globalStyleManager" 
      v-model="globalTextStyles"
      @change="updateGlobalTextStyle"
      @apply="applyGlobalTextStyle" />

    <template #footer>
      <el-button @click="globalStyleDialogVisible = false">取消</el-button>
      <el-button type="primary" @click="saveGlobalStyleEdit">应用到所有卡片</el-button>
    </template>
  </el-dialog>

  <!-- 卡片组样式设置对话框 -->
  <el-dialog v-model="cardGroupStyleDialogVisible" title="卡片组样式设置" width="400px" @close="onCardGroupStyleDialogClose">
    <p class="dialog-description">设置将应用于所有卡片组</p>

    <StyleManager 
      ref="cardGroupStyleManager" 
      v-model="cardGroupStyles"
      @change="updateCardGroupStyle"
      @apply="applyCardGroupStyle" />

    <template #footer>
      <el-button @click="cardGroupStyleDialogVisible = false">取消</el-button>
      <el-button type="primary" @click="saveCardGroupStyleEdit">应用到所有卡片组</el-button>
    </template>
  </el-dialog>

  <!-- 卡片行样式设置对话框 -->
  <el-dialog v-model="cardRowStyleDialogVisible" title="卡片行样式设置" width="400px" @close="onCardRowStyleDialogClose">
    <p class="dialog-description">设置将应用于所有卡片行</p>

    <StyleManager 
      ref="cardRowStyleManager" 
      v-model="cardRowStyles"
      @change="updateCardRowStyle"
      @apply="applyCardRowStyle" />

    <template #footer>
      <el-button @click="cardRowStyleDialogVisible = false">取消</el-button>
      <el-button type="primary" @click="saveCardRowStyleEdit">应用到所有卡片行</el-button>
    </template>
  </el-dialog>

  <!-- 单个卡片样式设置对话框 -->
  <el-dialog v-model="cardStyleDialogVisible" title="单个卡片样式设置" width="400px" @close="onCardStyleDialogClose">
    <p class="dialog-description">设置将应用于当前选中的卡片</p>
    <StyleManager 
      ref="cardStyleManager" 
      v-model="cardStyles"
      @change="updateCardStyle" 
      @apply="applyCardStyle" />

    <template #footer>
      <el-button @click="cardStyleDialogVisible = false">取消</el-button>
      <el-button type="primary" @click="saveCardStyleEdit">应用到选中卡片</el-button>
    </template>
  </el-dialog>

  <!-- 隐藏的StyleManager组件，用于获取默认样式值 -->
  <div style="display:none;">
    <StyleManager ref="defaultStyleManagerRef" />
  </div>
</template>

<script setup>
import { ref, reactive, computed, nextTick, onMounted } from 'vue';
import { ElMessage } from 'element-plus';
import StyleManager from './StyleManager.vue';
import {
  cardStyleTemplates, 
  cardRowStyleTemplates, 
  cardGroupStyleTemplates,
  cardStyleDefaults,
  cardRowStyleDefaults,
  cardGroupStyleDefaults
} from '@/utils/styleConfig.ts';

// 使用v-model定义样式数据
const cardGroupStyles = defineModel('cardGroupStyles', { type: Object, default: () => ({}) });
const cardRowStyles = defineModel('cardRowStyles', { type: Object, default: () => ({}) });
const cardStyles = defineModel('cardStyles', { type: Object, default: () => ({}) });
const globalTextStyles = defineModel('globalTextStyles', { type: Object, default: () => ({}) });

// 单卡片样式模板选项

// 卡片行样式模板选项

// 卡片组样式模板选项

// 导入样式模板选项

// 选定的卡片样式模板
const selectedCardStyle = defineModel('selectedCardStyle', { default: 'card-default' })
const selectedCardRowStyle = defineModel('selectedCardRowStyle', { default: 'row-default' })
const selectedCardGroupStyle = defineModel('selectedCardGroupStyle', { default: 'group-default' })


// 拖动相关数据
const dragOffset = reactive({ x: 0, y: 0 });
const isDragging = ref(false);

// 确保 toolbarPosition 被正确初始化
const toolbarPosition = reactive({ x: 20, y: 20 }); // 默认位置

// 计算工具条样式
const toolbarStyle = computed(() => {
  // 添加检查确保 toolbarPosition 存在且有 x,y 属性
  if (!toolbarPosition || 
      typeof toolbarPosition.x === 'undefined' || 
      typeof toolbarPosition.y === 'undefined') {
    // 设置默认值
    toolbarPosition.x = toolbarPosition.x || 20;
    toolbarPosition.y = toolbarPosition.y || 20;
  }
  
  return {
    position: 'absolute',
    left: `${toolbarPosition.x}px`,
    top: `${toolbarPosition.y}px`,
    zIndex: 1000,
    cursor: isDragging.value ? 'grabbing' : 'grab'
  };
});

// 定义props
const props = defineProps({
  // 新增的样式相关props
  cardGroups: {
    type: Array,
    default: () => []
  },
  defaultStyleManagerRef: {
    type: Object,
    default: () => null
  },
  // 添加页面尺寸属性
  pageSize: {
    type: Object,
    default: () => ({ width: 800, height: 600 }) // 默认使用像素单位
  }
});

// 定义emits
const emit = defineEmits([
  'cardStyleTemplateChange',
  // 新增的emits
  'save-global-style-edit',
  'save-card-group-style-edit',
  'save-card-row-style-edit',
  'save-card-style-edit',
  'edit-card',
  'edit-row-style',
  'save-row-style-edit',
  'save-card-edit', // 添加缺失的事件声明
  // 添加缺失的事件声明
  'open-global-style-panel',
  'open-card-style-panel',
  'open-card-row-style-panel',
  'open-card-group-style-panel',
  'update:cardGroups',
  'save-data-to-server' // 添加保存数据到服务器的事件
]);

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

  // 直接更新工具栏位置
  const newX = event.clientX - dragOffset.x;
  const newY = event.clientY - dragOffset.y;
  
  // 更新位置
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

// 打开全局样式设置面板
const openGlobalStylePanel = () => {
  // 深拷贝当前全局样式或使用默认样式
  const currentGlobalStyle = globalTextStyles.value || {}
  globalTextStyles.value = JSON.parse(JSON.stringify(currentGlobalStyle))
  globalStyleDialogVisible.value = true

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    if (globalStyleManager.value) {
      // 确保样式管理器有正确的初始值
      Object.assign(globalStyleManager.value.localStyle, currentGlobalStyle);
      globalStyleManager.value.$forceUpdate();
    }
  });
}

// 打开卡片组样式设置面板
const openCardGroupStylePanel = () => {
  // 深拷贝当前卡片组样式或使用默认样式
  const currentCardGroupStyle = cardGroupStyles.value || {}
  cardGroupStyles.value = JSON.parse(JSON.stringify(currentCardGroupStyle))
  cardGroupStyleDialogVisible.value = true

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    if (cardGroupStyleManager.value) {
      // 确保样式管理器有正确的初始值
      Object.assign(cardGroupStyleManager.value.localStyle, currentCardGroupStyle);
      cardGroupStyleManager.value.$forceUpdate();
    }
  });
}

// 打开卡片行样式设置面板
const openCardRowStylePanel = () => {
  // 深拷贝当前卡片行样式或使用默认样式
  const currentCardRowStyle = cardRowStyles.value || {}
  cardRowStyles.value = JSON.parse(JSON.stringify(currentCardRowStyle))
  cardRowStyleDialogVisible.value = true

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    if (cardRowStyleManager.value) {
      // 确保样式管理器有正确的初始值
      Object.assign(cardRowStyleManager.value.localStyle, currentCardRowStyle);
      cardRowStyleManager.value.$forceUpdate();
    }
  });
}

// 打开单个卡片样式设置面板
const openCardStylePanel = () => {
  // 深拷贝当前单个卡片样式或使用默认样式
  const currentCardStyle = cardStyles.value || {}
  cardStyles.value = JSON.parse(JSON.stringify(currentCardStyle))
  cardStyleDialogVisible.value = true

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    if (cardStyleManager.value) {
      // 确保样式管理器有正确的初始值
      Object.assign(cardStyleManager.value.localStyle, currentCardStyle);
      cardStyleManager.value.$forceUpdate();
    }
  });
}

// 控制各种样式对话框显示
const globalStyleDialogVisible = ref(false);
const cardGroupStyleDialogVisible = ref(false);
const cardRowStyleDialogVisible = ref(false);
const cardStyleDialogVisible = ref(false);

// 样式管理器引用
const globalStyleManager = ref(null);
const cardGroupStyleManager = ref(null);
const cardRowStyleManager = ref(null);
const cardStyleManager = ref(null);

// 对话框关闭处理
const onGlobalStyleDialogClose = () => {
  // 向父组件发出事件

};

// 卡片组样式对话框关闭处理
const onCardGroupStyleDialogClose = () => {
  // 不重置编辑状态，保留已保存的样式
};

// 卡片行样式对话框关闭处理
const onCardRowStyleDialogClose = () => {
  // 不重置编辑状态，保留已保存的样式
};

// 单个卡片样式对话框关闭处理
const onCardStyleDialogClose = () => {
  // 不重置编辑状态，保留已保存的样式
};

// 更新全局文本样式
const updateGlobalTextStyle = (style) => {
  // 实现更新全局文本样式的逻辑
  globalTextStyles.value = { ...style };
};

// 应用全局文本样式
const applyGlobalTextStyle = (style) => {
  // 实现应用全局文本样式逻辑
  globalTextStyles.value = { ...style };
};

// 保存全局样式编辑
const saveGlobalStyleEdit = () => {
  // 实现保存全局样式编辑的逻辑
  globalStyleDialogVisible.value = false;
  if (!props.cardGroups) return;
  console.log(`保存编辑的全局样式: globalTextStyles =`, globalTextStyles.value);
  
  // 使用StyleManager的toCSS方法获取实际CSS样式
  const actualStyles = globalStyleManager.value ? globalStyleManager.value.toCSS() : globalTextStyles.value;
  console.log(`实际CSS样式: actualStyles =`, actualStyles);
  
  // 更新所有卡片的文本样式
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(cardGroup => {
    if (!cardGroup.rows || cardGroup.rows.length === 0) {
      return;
    }
    cardGroup.rows.forEach(cardRow => {
      if (cardRow.cards && cardRow.cards.length > 0) {
        cardRow.cards.forEach(card => {
          // 合并全局样式和卡片原有样式
          card.textStyle = Object.assign({}, card.textStyle || {}, actualStyles);
        });
      }
    });
  });
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
  emit('save-global-style-edit', actualStyles);
  emit('save-data-to-server'); // 触发保存到服务器的事件
}

// 更新卡片组样式
const updateCardGroupStyle = (style) => {
  // 实现更新卡片组样式的逻辑
  cardGroupStyles.value = style;
};

// 应用卡片组样式
const applyCardGroupStyle = (style) => {
  // 实现应用卡片组样式的逻辑
  cardGroupStyles.value = style;
};

// 保存卡片组样式编辑
const saveCardGroupStyleEdit = () => {
  // 实现保存卡片组样式编辑的逻辑
  cardGroupStyleDialogVisible.value = false;
  if (!props.cardGroups) return;
  console.log(`保存编辑的卡片组样式: cardGroupStyles =`, cardGroupStyles.value);
  
  // 使用StyleManager的toCS方法获取实际CSS样式
  const actualStyles = cardGroupStyleManager.value ? cardGroupStyleManager.value.toCSS() : cardGroupStyles.value;
  console.log(`实际CSS样式: actualStyles =`, actualStyles);
  
  // 更新卡片组样式
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(cardGroup => {
    // 修改: 确保样式正确应用，使用Object.assign保证样式对象正确合并
    // 修改为更明确的样式赋值方式，确保样式能正确应用
    cardGroup.style = Object.assign({}, cardGroup.style || {}, actualStyles, { _css: actualStyles });
  });
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
  emit('save-card-group-style-edit', actualStyles);
  emit('save-data-to-server'); // 触发保存到服务器的事件
}


// 更新卡片行样式
const updateCardRowStyle = (style) => {
  // 实现更新卡片行样式的逻辑
  cardRowStyles.value = style;
};

// 应用卡片行样式
const applyCardRowStyle = (style) => {
  cardRowStyles.value = style;
};

// 保存卡片行样式编辑
const saveCardRowStyleEdit = () => {
  // 实现保存卡片行样式编辑的逻辑
  cardRowStyleDialogVisible.value = false;
  if (!props.cardGroups) return;
  console.log(`保存编辑的卡片行样式: cardRowStyles =`, cardRowStyles.value);
  
  // 使用StyleManager的toCSS方法获取实际CSS样式
  const actualStyles = cardRowStyleManager.value ? cardRowStyleManager.value.toCSS() : cardRowStyles.value;
  console.log(`实际CSS样式: actualStyles =`, actualStyles);
  
  // 更新卡片行样式
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(cardGroup => {
    // 修复: 检查rows是否存在（根据历史对话中提到的问题）
    if (!cardGroup.rows || cardGroup.rows.length === 0) {
      return;
    }
    cardGroup.rows.forEach(cardRow => {
      // 修改: 确保样式正确应用，使用Object.assign保证样式对象正确合并
      // 修改为更明确的样式赋值方式，确保样式能正确应用
      cardRow.style = Object.assign({}, cardRow.style || {}, actualStyles, { _css: actualStyles });
    });
  });
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
  emit('save-card-row-style-edit', actualStyles);
  emit('save-data-to-server'); // 触发保存到服务器的事件
}

// 更新卡片样式
const updateCardStyle = (style) => {
  // 实现更新卡片样式的逻辑
  cardStyles.value = style;
};

// 应用卡片样式
const applyCardStyle = (style) => {
  // 实现应用卡片样式的逻辑
  cardStyles.value = style;
};

// 保存卡片样式编辑
const saveCardStyleEdit = () => {
  // 实现保存卡片样式编辑的逻辑
  cardStyleDialogVisible.value = false;
  if (!props.cardGroups) return;
  console.log(`保存编辑的卡片样式: cardStyles =`, cardStyles.value);
  
  // 使用StyleManager的toCS方法获取实际CSS样式
  const actualStyles = cardStyleManager.value ? cardStyleManager.value.toCSS() : cardStyles.value;
  console.log(`实际CSS样式: actualStyles =`, actualStyles);
  
  // 更新卡片样式
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(cardGroup => {
    // 修复: 检查rows是否存在（根据历史对话中提到的问题）
    if (!cardGroup.rows || cardGroup.rows.length === 0) {
      return;
    }
    cardGroup.rows.forEach(cardRow => {
      // 修复: 检查cards是否存在
      if (cardRow.cards && cardRow.cards.length > 0) {
        cardRow.cards.forEach(card => {
          // 修改: 确保样式正确应用，使用Object.assign保证样式对象正确合并
          // 修改为更明确的样式赋值方式，确保样式能正确应用
          card.style = Object.assign({}, card.style || {}, actualStyles, { _css: actualStyles });
        });
      }
    });
  });
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
  emit('save-card-style-edit', actualStyles);
  emit('save-data-to-server'); // 触发保存到服务器的事件
}

// 编辑功能相关
const editDialogVisible = ref(false);
const showTextStylePanel = ref(false); // 控制文本样式面板显示
const textStyleManager = ref(null); // 文本样式管理器引用
const editingCard = reactive({
  text: '',
  groupIndex: -1,
  rowIndex: -1,
  cardIndex: -1,
  textStyle: {} // 添加文本样式设置
});

// 行样式编辑功能相关
const rowStyleDialogVisible = ref(false);
const rowStyleManager = ref(null); // 行样式管理器引用
const editingRowStyle = reactive({
  groupIndex: -1,
  rowIndex: -1,
  textStyle: {} // 行文本样式设置
});

// 获取样式管理器中的默认样式
const getStyleManagerDefaults = () => {
  try {
    // 如果样式管理器引用存在，从中获取localStyle
    if (props.defaultStyleManagerRef && props.defaultStyleManagerRef.localStyle) {
      return props.defaultStyleManagerRef.localStyle;
    }
  } catch (err) {
    console.error('无法获取StyleManager默认样式', err);
  }

  // 如果获取失败，返回一个基础默认值（与StyleManager.vue中定义的默认值保持一致）
  return {
    // 常用设置
    fontSize: 16,
    color: '#000000',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'left',
    lineHeight: 1.2,
    backgroundColor: '#ffffff',
    opacity: 1,
    textRotation: 0,
    scale: 1,

    // 高级设置
    padding: 0,
    margin: 0,
    borderStyle: 'none',
    borderWidth: 1,
    borderColor: '#000000',
    borderRadius: 0,
    width: '',
    height: '',
    maxWidth: '',
    maxHeight: '',
    minWidth: '',
    minHeight: '',
    display: '',
    position: 'static',
    left: '',
    top: '',
    right: '',
    bottom: '',
    boxShadow: '',
    transform: '',
    textDecoration: 'none',
    textTransform: 'none',
    letterSpacing: 0,
    wordSpacing: 0,
    fontFamily: '',
    fontVariant: 'normal',
    verticalAlign: 'baseline',
    overflow: 'visible',
    cursor: 'default',
    visibility: 'visible',
    zIndex: 0,
    float: 'none',
    clear: 'none',

    // Flexbox相关属性
    flexDirection: 'row',
    flexWrap: 'nowrap',
    justifyContent: 'flex-start',
    alignItems: 'stretch',

    // Grid相关属性
    gridTemplateColumns: '',
    gridTemplateRows: '',
    gridColumnGap: 0,
    gridRowGap: 0,

    // 背景相关属性
    backgroundImage: '',
    backgroundRepeat: 'repeat',
    backgroundSize: 'auto',
    backgroundPosition: '',
    backgroundAttachment: 'scroll',

    // 文本相关属性
    textIndent: 0,
    textShadow: '',
    textOverflow: '',
    wordBreak: 'normal',
    wordWrap: 'normal',

    // 列表样式属性
    listStyleType: 'none',
    listStylePosition: 'outside',
    listStyleImage: '',

    // 过渡和动画属性
    transition: '',
    animation: '',

    // 滤镜属性
    filter: '',

    // outline属性
    outline: '',

    // white-space属性
    whiteSpace: 'normal',

    // 用户选择属性
    userSelect: '',

    // 盒模型属性
    boxSizing: '',

    // 方向属性
    direction: 'ltr',
    writingMode: 'horizontal-tb'
  };
};

// 编辑卡片
const editCard = (groupIndex, rowIndex, cardIndex) => {
  // 现在需要三个索引：组索引、行索引和卡片索引
  const cardGroup = props.cardGroups[groupIndex];
  if (!cardGroup) return;

  const row = cardGroup.rows[rowIndex];
  if (!row) return;

  const card = row.cards[cardIndex];
  if (!card) return;

  editingCard.text = card.text || '';
  editingCard.groupIndex = groupIndex;
  editingCard.rowIndex = rowIndex;
  editingCard.cardIndex = cardIndex;
  // 添加卡片文本样式设置
  editingCard.textStyle = card.textStyle || {};
  // 重置文本样式面板状态
  showTextStylePanel.value = false;
  editDialogVisible.value = true;

  // 确保在对话框打开后正确设置样式
  nextTick(() => {
    // 创建一个新的对象引用来确保样式正确更新
    const newStyle = JSON.parse(JSON.stringify(editingCard.textStyle || {}));
    editingCard.textStyle = newStyle;
  });

  // 向父组件发出事件
  emit('edit-card', { groupIndex, rowIndex, cardIndex, card });
};

// 保存卡片编辑
const saveCardEdit = () => {
  const { groupIndex, rowIndex, cardIndex, text } = editingCard;
  let { textStyle } = editingCard; // 使用 let 声明，允许重新分配
  
  // 使用StyleManager的toCSS方法获取实际CSS样式
  if (textStyleManager.value && typeof textStyleManager.value.toCSS === 'function') {
    try {
      // 保存原始localStyle
      const originalLocalStyle = { ...textStyleManager.value.localStyle };
      
      // 将textStyle应用到StyleManager上
      Object.assign(textStyleManager.value.localStyle, textStyle);
      
      // 获取转换后的CSS（仅包含可见样式）
      textStyle = textStyleManager.value.toCSS();
      
      // 恢复原始样式
      Object.assign(textStyleManager.value.localStyle, originalLocalStyle);
    } catch (error) {
      console.error('Error processing card text style with StyleManager:', error);
    }
  }
  
  if (groupIndex >= 0 && rowIndex >= 0 && cardIndex >= 0 &&
    props.cardGroups[groupIndex]?.rows[rowIndex]?.cards[cardIndex]) {
    // 更新卡片文本
    const updatedCardGroups = JSON.parse(JSON.stringify(props.cardGroups));
    updatedCardGroups[groupIndex].rows[rowIndex].cards[cardIndex].text = text;
    // 更新卡片文本样式
    updatedCardGroups[groupIndex].rows[rowIndex].cards[cardIndex].textStyle = JSON.parse(JSON.stringify(textStyle || {}));

    // 更新父组件数据
    emit('update:cardGroups', updatedCardGroups);

    // 关闭对话框
    editDialogVisible.value = false;

    // 提示成功
    ElMessage.success('卡片内容已更新');

    // 向父组件发出事件
    emit('save-card-edit', { groupIndex, rowIndex, cardIndex, text, textStyle });
    emit('save-data-to-server'); // 触发保存到服务器的事件
  }
};

// 编辑行样式
const editRowStyle = (groupIndex, rowIndex) => {
  const cardGroup = props.cardGroups[groupIndex];
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

  // 向父组件发出事件
  emit('edit-row-style', { groupIndex, rowIndex });
};

// 更新行文本样式
const updateRowTextStyle = (style) => {
  editingRowStyle.textStyle = { ...style };
};

// 应用行文本样式
const applyRowTextStyle = (style) => {
  editingRowStyle.textStyle = { ...style };
};

// 保存行样式编辑
const saveRowStyleEdit = () => {
  const { groupIndex, rowIndex } = editingRowStyle;

  let { textStyle } = editingCard; // 使用 let 声明，允许重新分配
  
  // 使用StyleManager的toCSS方法获取实际CSS样式
  if (textStyleManager.value && typeof textStyleManager.value.toCSS === 'function') {
    try {
      // 保存原始localStyle
      const originalLocalStyle = { ...textStyleManager.value.localStyle };
      
      // 将textStyle应用到StyleManager上
      Object.assign(textStyleManager.value.localStyle, textStyle);
      
      // 获取转换后的CSS（仅包含可见样式）
      textStyle = textStyleManager.value.toCSS();
      
      // 恢复原始样式
      Object.assign(textStyleManager.value.localStyle, originalLocalStyle);
    } catch (error) {
      console.error('Error processing card text style with StyleManager:', error);
    }
  }

  if (groupIndex >= 0 && rowIndex >= 0 && props.cardGroups[groupIndex]?.rows[rowIndex]) {
    const row = props.cardGroups[groupIndex].rows[rowIndex];

    // 将样式保存到行的style属性中
    const updatedCardGroups = JSON.parse(JSON.stringify(props.cardGroups));
    updatedCardGroups[groupIndex].rows[rowIndex].style = JSON.parse(JSON.stringify(textStyle));

    // 将样式应用到行中的所有卡片
    if (row.cards && row.cards.length > 0) {
      row.cards.forEach(card => {
        updatedCardGroups[groupIndex].rows[rowIndex].cards.forEach(c => {
          c.textStyle = JSON.parse(JSON.stringify(textStyle)); // 深拷贝应用样式
        });
      });

      // 更新父组件数据
      emit('update:cardGroups', updatedCardGroups);

      // 关闭对话框
      rowStyleDialogVisible.value = false;

      // 提示成功
      ElMessage.success(`已将样式应用到第${rowIndex + 1}行的${row.cards.length}个卡片`);

      // 向父组件发出事件
      emit('save-row-style-edit', { groupIndex, rowIndex, textStyle });
    }
  }
};

// 切换文本样式面板显示
const toggleTextStylePanel = () => {
  showTextStylePanel.value = !showTextStylePanel.value;
  if (showTextStylePanel.value) {
    // 获取当前卡片的索引
    const { groupIndex, rowIndex, cardIndex } = editingCard;

    // 如果有有效的卡片索引
    if (groupIndex >= 0 && rowIndex >= 0 && cardIndex >= 0 &&
      props.cardGroups[groupIndex]?.rows[rowIndex]?.cards[cardIndex]) {

      // 获取当前卡片对象
      const currentCard = props.cardGroups[groupIndex].rows[rowIndex].cards[cardIndex];

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

      // 处理fontSize，确保它是数字形式而不是CSS字符串（如'21px'）
      if (styleCopy.fontSize && typeof styleCopy.fontSize === 'string' && styleCopy.fontSize.endsWith('px')) {
        styleCopy.fontSize = parseInt(styleCopy.fontSize, 10);
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
};

// 更新编辑中的卡片文本样式
const updateEditingCardTextStyle = (style) => {
  editingCard.textStyle = { ...style };
};

// 应用编辑中的卡片文本样式
const applyEditingCardTextStyle = (style) => {
  editingCard.textStyle = { ...style };
};

// 对话框关闭处理
const onDialogClose = () => {
  showTextStylePanel.value = false; // 关闭文本样式面板
};

// 行样式对话框关闭处理
const onRowDialogClose = () => {
  // 重置编辑状态
  editingRowStyle.groupIndex = -1;
  editingRowStyle.rowIndex = -1;
  editingRowStyle.textStyle = {};
};

// 处理卡片样式模板变化
const handleCardStyleTemplateChange = (type, styleValue) => {
  console.log(`Card ${type} style template changed to:`, styleValue);

  switch (type) {
    case 'card':
      handleCardStyleChange(styleValue);
      break;
    case 'row':
      handleCardRowStyleChange(styleValue);
      break;
    case 'group':
      handleCardGroupStyleChange(styleValue);
      break;
  }

  // 触发样式更新
  //updateStyle();
}

// 处理单卡片样式变化
const handleCardStyleChange = (styleValue) => {
  console.log('Card style changed to:', styleValue);

  // 使用从配置文件导入的默认值
  const cardStyle = cardStyleDefaults[styleValue] || cardStyleDefaults['card-default'];

  // 将样式应用到所有卡片
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(group => {
    group.rows.forEach(row => {
      row.cards.forEach(card => {
        card.style = { ...cardStyle };
      });
    });
  });

  // 更新卡片样式
  cardStyles.value = { ...cardStyle };
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
}

// 处理卡片行样式变化
const handleCardRowStyleChange = (styleValue) => {
  console.log('Card row style changed to:', styleValue);

  // 使用从配置文件导入的默认值
  const rowStyle = cardRowStyleDefaults[styleValue] || cardRowStyleDefaults['row-default'];

  // 应用行样式到所有行
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(group => {
    group.rows.forEach(row => {
      row.style = { ...rowStyle };
    });
  });

  // 更新卡片行样式
  cardRowStyles.value = { ...rowStyle };
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
}

// 处理卡片组样式变化
const handleCardGroupStyleChange = (styleValue) => {
  console.log('Card group style changed to:', styleValue);

  // 使用从配置文件导入的默认值
  const groupStyle = cardGroupStyleDefaults[styleValue] || cardGroupStyleDefaults['group-default'];

  // 应用组样式到所有组
  const updatedCardGroups = [...props.cardGroups];
  updatedCardGroups.forEach(group => {
    group.style = { ...groupStyle };
  });

  // 更新卡片组样式
  cardGroupStyles.value = { ...groupStyle };
  
  // 向父组件发送事件通知样式已更新
  emit('update:cardGroups', updatedCardGroups);
}

// 定义组件暴露的方法
defineExpose({
  globalStyleDialogVisible,
  cardGroupStyleDialogVisible,
  cardRowStyleDialogVisible,
  cardStyleDialogVisible,
  globalStyleManager,
  cardGroupStyleManager,
  cardRowStyleManager,
  cardStyleManager,
  editCard,     // 暴露编辑卡片方法
  editRowStyle  // 暴露编辑行样式方法
});
</script>

<style scoped>
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
  flex-direction: column;
  gap: 12px;
  background-color: #fff;
  border-bottom-left-radius: 8px;
  border-bottom-right-radius: 8px;
}

.style-template-selector {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.selector-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.selector-group .control-label {
  min-width: 100px;
  text-align: right;
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

/* 行样式对话框内容样式 */
.dialog-description {
  font-size: 14px;
  color: #606266;
  margin-bottom: 15px;
  padding-bottom: 10px;
  border-bottom: 1px dashed #ebeef5;
}
</style>