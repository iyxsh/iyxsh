<template>
  <div class="form-grid" ref="containerRef" :style="gridContainerStyle">
    <div v-for="form in visibleForms" :key="form.id" class="form-card-wrapper"
      :class="{ 'dragging': isDragging && draggedForm?.id === form.id }" draggable="true"
      @dragstart="handleDragStart($event, form)" @dragend="handleDragEnd($event)"
      @dragover="handleDragOver($event, form)" @drop="handleDrop($event, form)"
      @contextmenu.prevent="openMenu(form.id, $event)" @mousedown.stop="startDrag($event, form)"
      :style="getCardStyle(form)">
      <el-card v-if="!showDetail[form.id]" :class="['form-card-mini', cardStyleOn ? '' : 'no-style']"
        @dblclick="openDetail(form.id)" :body-style="getDefaultCardStyles()">
        <div ref="setContentRef" :data-id="form.id" :style="{ display: 'inline-block' }">
          <div v-if="form.showTitle !== false"
            :style="{ fontSize: (form.titleFontSize || 16) + 'px', color: form.titleColor || '#333' }">
            <b>{{ form.title }}</b>
          </div>
          <div v-if="form.showValue !== false"
            :style="{ fontSize: (form.valueFontSize || 16) + 'px', color: form.valueColor || '#333' }">
            {{ form.value }}
          </div>
          <div v-if="form.showRemark !== false && form.remark"
            :style="{ fontSize: (form.remarkFontSize || 14) + 'px', color: form.remarkColor || '#666' }">
            {{ form.remark }}
          </div>
          <Transition name="fade">
            <div v-if="menuIndex === form.id" class="card-menu">
              <div class="menu-grid">
                <el-button :disabled="!editable" @click.stop="editForm(form.id)">
                  <el-icon>
                    <EditPen />
                  </el-icon>
                  Edit
                </el-button>
                <el-button @click.stop="openDetail(form.id)">
                  <el-icon>
                    <View />
                  </el-icon>
                  Detail
                </el-button>
                <el-button type="danger" :disabled="!editable" @click.stop="confirmDelete(form.id)">
                  <el-icon>
                    <Delete />
                  </el-icon>
                  Delete
                </el-button>
              </div>
            </div>
          </Transition>
        </div>
        <div v-if="editable" class="resize-handle" @mousedown.stop="startResize($event, form.id)"></div>
      </el-card>
      <el-dialog v-model="showDetail[form.id]" title="Form Detail" width="640px" @close="closeDetail(form.id)"
        destroy-on-close>
        <FormCard :model-value="[findFormById(form.id)]" @update:model-value="onSaveDetail(form.id, $event)"
          :editable="editable" @save="onSaveDetail(form.id, $event)" @delete="onDelete(form.id)" />
      </el-dialog>
    </div>

    <!-- 添加按钮 -->
    <el-button v-if="editable" class="add-form-button" type="primary" circle @click="handleAddForm">
      <el-icon>
        <Plus />
      </el-icon>
    </el-button>
  </div>

  <!-- 添加表单的弹窗 -->
  <el-dialog v-model="showAddDialog" title="Add Form" width="640px" @close="cancelAddForm" append-to-body
    destroy-on-close>
    <FormCard v-if="addFormData" :model-value="[addFormData]" :editable="true" @update:model-value="saveAddForm" @delete="cancelAddForm" />
  </el-dialog>
</template>

<script setup>
import { ref, reactive, computed, watch, onMounted, onBeforeUnmount, nextTick } from 'vue'
import { ElMessage, ElMessageBox, ElDialog } from 'element-plus'  // 添加了缺失的 ElMessageBox 和 ElDialog 导入
import { useEventBus } from '../utils/eventBus'
import { EditPen, View, Delete, Plus } from '@element-plus/icons-vue'

const { on, off } = useEventBus()

// 添加组件加载调试信息
console.log('FormCard component initializing');

// 定义props
const props = defineProps({
  modelValue: {
    type: Array,
    required: true,
    default: () => []
  },
  editable: {
    type: Boolean,
    default: true
  },
  pageSize: {
    type: Object,
    default: () => ({
      width: 210,
      height: 297
    })
  },
  useVirtual: {
    type: Boolean,
    default: true
  },
  cardStyleOn: {
    type: Boolean,
    default: true
  }
})

// 定义emits
const emits = defineEmits([
  'save', 
  'delete', 
  'update:modelValue', 
  'form-moved', 
  'form-updated',
  'error'
])

// 表单数据与状态管理
const forms = ref([]);
const visibleForms = ref([]);

// 卡片位置与尺寸管理
const positions = reactive({});
const cardSizes = reactive({});
const minSize = { width: 180, height: 120 };
const maxSize = { width: 400, height: 300 };

// 容器引用
const containerRef = ref(null);

// 拖拽和交互状态
const isDragging = ref(false);
const draggedForm = ref(null);
const dragOverForm = ref(null);
const draggingId = ref(null);
const dragStart = ref({ x: 0, y: 0 });
const mouseStart = ref({ x: 0, y: 0 });

// 缩放相关状态
const resizingId = ref(null);
const resizeStart = ref({ width: 0, height: 0 });
const resizeMouseStart = ref({ x: 0, y: 0 });

// UI 状态管理
const showDetail = reactive({});
const menuIndex = ref(null);
const showAddDialog = ref(false);
const addFormData = ref(null);

// DOM 引用和监听器
const contentRefs = new Map();
const observedIds = new Set();
let resizeObserver = null;

// 清理任务收集
const cleanupTasks = [];

// 网格容器样式
const gridContainerStyle = computed(() => {
  if (props.pageSize) {
    const { width, height, unit } = props.pageSize;
    return {
      width: `${width}${unit}`,
      minHeight: `${height}${unit}`,
      margin: '0 auto',
      border: '1px solid #dcdfe6',
      padding: '10px',
      backgroundColor: 'white',
      boxSizing: 'border-box',
      position: 'relative',
      overflow: 'auto',
      boxShadow: '0 2px 12px 0 rgba(0, 0, 0, 0.1)'
    };
  }
  return {};
});

// 工具函数 - 卡片样式
function getDefaultCardStyles() {
  return { padding: '8px' };
}

function getCardStyle(form) {
  const pos = positions[form.id] || { x: 0, y: 0 };
  const size = cardSizes[form.id] || minSize;

  return {
    width: size.width + 'px',
    height: size.height + 'px',
    position: 'absolute',
    left: pos.x + 'px',
    top: pos.y + 'px',
    zIndex: form.zIndex || 1,
    transform: 'none',
    transition: 'transform 0.2s, box-shadow 0.2s'
  };
}

// 防抖函数
function debounce(fn, delay) {
  let timer = null;
  return function (...args) {
    if (timer) clearTimeout(timer);
    timer = setTimeout(() => {
      fn.apply(this, args);
    }, delay);
  };
}

// 初始化ResizeObserver
function setupResizeObserver() {
  if (!resizeObserver) {
    resizeObserver = new ResizeObserver(entries => {
      for (const entry of entries) {
        if (isDragging.value || resizingId.value) continue; // 拖动或缩放时不更新尺寸
        const id = entry.target.dataset.id;
        if (id) {
          const { width, height } = entry.contentRect;
          cardSizes[id] = {
            width: Math.ceil(width) + 16,
            height: Math.ceil(height) + 16
          };
        }
      }
    });
  }
}

function setContentRef(el) {
  if (el && el.dataset && el.dataset.id) {
    contentRefs.set(el.dataset.id, el);
    observeContentSize(el.dataset.id, el);
  }
}

function observeContentSize(id, el) {
  setupResizeObserver();
  if (!observedIds.has(id)) {
    resizeObserver.observe(el);
    observedIds.add(id);
  }
}

// 添加内存泄漏检查
function checkMemoryUsage() {
  if (window.performance && window.performance.memory) {
    const memoryUsage = window.performance.memory;
    console.log('FormGrid Memory Usage:', {
      usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
      totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
      jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
    });

    // 如果内存使用超过阈值，触发警告
    if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
      console.warn('FormGrid: Memory usage is high, consider optimizing');
    }
  }
}

// 格式化字节大小
function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes';
  const k = 1024;
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// 在组件卸载前移除监听
onBeforeUnmount(() => {
  // 执行清理任务
  cleanupTasks.forEach(task => task());

  // 清理资源
  cleanupResources();
});

// 资源清理
function cleanupResources() {
  // 停止ResizeObserver
  if (resizeObserver) {
    resizeObserver.disconnect();
    resizeObserver = null;
  }

  // 清理contentRefs和observedIds
  contentRefs.forEach((value, key) => contentRefs.delete(key));
  observedIds.clear();

  // 清理事件监听
  off('page-type-changed', handlePageTypeChange);
  off('form-updated', handleFormUpdated);

  // 移除滚动监听
  window.removeEventListener('scroll', updateVisibleForms);

  // 移除resize监听
  window.removeEventListener('resize', updateVisibleForms);
}

// 查找表单
function findFormById(id) {
  return forms.value.find(f => f.id === id);
}

// 保存表单详情
function onSaveDetail(id, data) {
  ElMessageBox.confirm('Please check the information carefully and click Confirm to save if there are no errors.', 'Save Confirmation', {
    confirmButtonText: 'Confirm',
    cancelButtonText: 'Cancel',
    type: 'warning',
  }).then(() => {
    const index = forms.value.findIndex(f => f.id === id);
    if (index !== -1) {
      forms.value[index] = { ...data };
      closeDetail(id);
      validatePositionById(id);
      debouncedSavePositions();
      emits('update:modelValue', forms.value);
    }
  }).catch((error) => {
    if (error !== 'cancel') {
      console.error('Save error:', error);
      ElMessage.error('Save failed');
    }
  });
}

// 确认删除
function confirmDelete(id) {
  ElMessageBox.confirm('Are you sure you want to delete this form?', 'Delete Confirmation', {
    confirmButtonText: 'Delete',
    cancelButtonText: 'Cancel',
    type: 'warning',
  }).then(() => {
    onDelete(id);
  }).catch(() => {
    // 用户取消删除，不做任何操作
  });
}

// 删除表单
function onDelete(id) {
  const index = forms.value.findIndex(f => f.id === id);
  if (index !== -1) {
    forms.value.splice(index, 1);
    delete showDetail[id];
    delete positions[id];
    delete cardSizes[id];
    menuIndex.value = null;
    debouncedSavePositions();

    // 触发表单更新事件
    emits('update:modelValue', forms.value);
    emits('form-updated', forms.value);
  }
}

// 调整表单层级
function bringCardToFront(id) {
  const maxZ = Math.max(...forms.value.map(f => f.zIndex || 1), 1);
  const card = forms.value.find(f => f.id === id);
  if (card) card.zIndex = maxZ + 1;
}

// 打开表单详情
function openDetail(id) {
  bringCardToFront(id);
  showDetail[id] = true;
  menuIndex.value = null;
}

function closeDetail(id) {
  showDetail[id] = false;
}

// 菜单相关
function openMenu(id, e) {
  bringCardToFront(id);
  menuIndex.value = id;
  document.addEventListener('click', closeMenu);
}

function closeMenu() {
  menuIndex.value = null;
  document.removeEventListener('click', closeMenu);
}

function editForm(id) {
  openDetail(id);
}

// 拖拽处理函数
function handleDragStart(event, form) {
  event.target.style.opacity = '0.5';
  isDragging.value = true;
  draggedForm.value = form;

  // 设置拖动数据
  event.dataTransfer.effectAllowed = 'move';
  event.dataTransfer.setData('text/plain', form.id);
}

function handleDragEnd(event) {
  event.target.style.opacity = '1';
  isDragging.value = false;
  draggedForm.value = null;
  dragOverForm.value = null;
}

function handleDragOver(event, form) {
  event.preventDefault();
  if (!draggedForm.value || form.id === draggedForm.value.id) {
    return;
  }
  dragOverForm.value = form;
}

function handleDrop(event, form) {
  event.preventDefault();

  if (!draggedForm.value || !dragOverForm.value) {
    return;
  }

  // 获取表单索引
  const draggedIndex = forms.value.findIndex(f => f.id === draggedForm.value.id);
  const dragOverIndex = forms.value.findIndex(f => f.id === dragOverForm.value.id);

  // 如果拖拽到相同位置，直接返回
  if (draggedIndex === dragOverIndex) {
    return;
  }

  // 重新排序表单
  const updatedForms = [...forms.value];
  const [movedForm] = updatedForms.splice(draggedIndex, 1);
  updatedForms.splice(dragOverIndex, 0, movedForm);

  // 更新表单顺序
  forms.value = updatedForms;

  // 通知父组件表单已更新
  emits('update:modelValue', forms.value);
  emits('form-updated', forms.value);
}

// 表单拖拽操作
function startDrag(e, form) {
  bringCardToFront(form.id);
  if (!props.editable || e.target.classList.contains('resize-handle')) return;

  draggingId.value = form.id;
  isDragging.value = true;
  mouseStart.value = { x: e.clientX, y: e.clientY };
  dragStart.value = { ...(positions[draggingId.value] || { x: 0, y: 0 }) };

  // 锁定宽高
  const size = cardSizes[draggingId.value] || minSize;
  cardSizes[draggingId.value] = { ...size };

  // 拖动前移除监听
  const el = contentRefs.get(draggingId.value);
  if (resizeObserver && el) resizeObserver.unobserve(el);

  document.addEventListener('mousemove', onDragging);
  document.addEventListener('mouseup', stopDrag);

  // 添加清理任务
  cleanupTasks.push(() => {
    document.removeEventListener('mousemove', onDragging);
    document.removeEventListener('mouseup', stopDrag);
  });
}

function onDragging(e) {
  if (!draggingId.value) return;

  const dx = e.clientX - mouseStart.value.x;
  const dy = e.clientY - mouseStart.value.y;
  const baseOffset = 10;
  const size = cardSizes[draggingId.value] || minSize;
  
  // 确保使用正确的页面尺寸单位进行计算
  const containerWidth = props.pageSize ? parseInt(props.pageSize.width) : 800;
  const containerHeight = props.pageSize ? parseInt(props.pageSize.height) : 600;

  let x = Math.max(baseOffset, dragStart.value.x + dx);
  let y = Math.max(baseOffset, dragStart.value.y + dy);

  // 拖动时不让卡片超出右/下边界
  x = Math.min(x, containerWidth - size.width - baseOffset);
  y = Math.min(y, containerHeight - size.height - baseOffset);

  positions[draggingId.value] = { x, y };
}

function stopDrag() {
  if (draggingId.value) {
    const el = contentRefs.get(draggingId.value);
    if (resizeObserver && el) resizeObserver.observe(el);

    validatePositionById(draggingId.value);
    debouncedSavePositions();

    // 通知位置已更新
    const form = findFormById(draggingId.value);
    if (form) {
      emits('form-moved', {
        id: draggingId.value,
        position: positions[draggingId.value]
      });
    }
  }

  draggingId.value = null;
  isDragging.value = false;
  document.removeEventListener('mousemove', onDragging);
  document.removeEventListener('mouseup', stopDrag);
}

// 调整大小相关函数
function startResize(e, id) {
  if (!props.editable) return;
  e.stopPropagation();

  resizingId.value = id;
  resizeMouseStart.value = { x: e.clientX, y: e.clientY };
  const size = cardSizes[resizingId.value] || minSize;
  resizeStart.value = { width: size.width, height: size.height };

  // 缩放前移除监听
  const el = contentRefs.get(resizingId.value);
  if (resizeObserver && el) resizeObserver.unobserve(el);

  document.addEventListener('mousemove', onResizing);
  document.addEventListener('mouseup', stopResize);

  // 添加清理任务
  cleanupTasks.push(() => {
    document.removeEventListener('mousemove', onResizing);
    document.removeEventListener('mouseup', stopResize);
  });
}

function onResizing(e) {
  if (!resizingId.value) return;

  const dx = e.clientX - resizeMouseStart.value.x;
  const dy = e.clientY - resizeMouseStart.value.y;
  
  // 确保宽高为数值类型
  const containerWidth = props.pageSize ? parseInt(props.pageSize.width) : 800;
  const containerHeight = props.pageSize ? parseInt(props.pageSize.height) : 600;

  // 计算新尺寸，同时确保在最小和最大范围内
  const newWidth = Math.min(
    Math.max(minSize.width, resizeStart.value.width + dx),
    maxSize.width
  );

  const newHeight = Math.min(
    Math.max(minSize.height, resizeStart.value.height + dy),
    maxSize.height
  );

  cardSizes[resizingId.value] = {
    width: newWidth,
    height: newHeight
  };

  // 确保调整尺寸后不会超出容器
  validatePositionById(resizingId.value);
}

function stopResize() {
  if (resizingId.value) {
    const el = contentRefs.get(resizingId.value);
    if (resizeObserver && el) resizeObserver.observe(el);
    debouncedSavePositions();
  }

  resizingId.value = null;
  document.removeEventListener('mousemove', onResizing);
  document.removeEventListener('mouseup', stopResize);
}

// 位置验证和保存
function validatePositionById(id) {
  if (!props.pageSize) return;

  // 确保转换为数字以进行计算
  const containerWidth = props.pageSize ? parseInt(props.pageSize.width) : 800;
  const containerHeight = props.pageSize ? parseInt(props.pageSize.height) : 600;
  const pos = positions[id] || { x: 0, y: 0 };
  const size = cardSizes[id] || minSize;

  // 确保位置有效 - 不超出容器边界
  if (pos.x + size.width > containerWidth) {
    pos.x = Math.max(0, containerWidth - size.width);
  }

  if (pos.y + size.height > containerHeight) {
    pos.y = Math.max(0, containerHeight - size.height);
  }

  positions[id] = pos;
}

// 位置存储相关
const debouncedSavePositions = debounce(() => {
  savePositions();
}, 300);

function savePositions() {
  localStorage.setItem('formPositions', JSON.stringify(positions));
  localStorage.setItem('formSizes', JSON.stringify(cardSizes));
}

function loadPositions() {
  const savedPositions = localStorage.getItem('formPositions');
  const savedSizes = localStorage.getItem('formSizes');

  // 先清空再赋值，保证响应式
  Object.keys(positions).forEach(k => delete positions[k]);
  Object.keys(cardSizes).forEach(k => delete cardSizes[k]);

  if (savedPositions) {
    const obj = JSON.parse(savedPositions);
    Object.keys(obj).forEach(k => positions[k] = obj[k]);
  }

  if (savedSizes) {
    const obj = JSON.parse(savedSizes);
    Object.keys(obj).forEach(k => cardSizes[k] = obj[k]);
  }

  // 在使用forEach之前，确保forms.value是一个数组
  if (Array.isArray(forms.value)) {
    forms.value.forEach(f => validatePositionById(f.id));
  } else {
    console.error('[FormCard] forms.value不是数组:', forms.value);
    // 初始化为空数组
    forms.value = [];
  }
}

// 页面类型和表单更新处理
function handlePageTypeChange(newType) {
  // 根据页面类型处理表单展示
  updateVisibleForms();
}

function handleFormUpdated(updatedForms) {
  forms.value = updatedForms;
}

// 虚拟滚动实现
function updateVisibleForms() {
  if (!props.useVirtual || !containerRef.value || !forms.value.length) {
    visibleForms.value = forms.value;
    return;
  }

  const containerRect = containerRef.value.getBoundingClientRect();
  const buffer = 300; // 预加载缓冲区

  visibleForms.value = forms.value.filter(form => {
    const pos = positions[form.id] || { x: 0, y: 0 };
    const size = cardSizes[form.id] || minSize;

    // 检查表单是否在可视区域内或缓冲区内
    return (
      pos.y < containerRect.height + buffer &&
      pos.y + size.height > -buffer &&
      pos.x < containerRect.width + buffer &&
      pos.x + size.width > -buffer
    );
  });
}

// 添加表单相关函数
function handleAddForm() {
  addFormData.value = {
    id: Date.now().toString() + Math.random().toString(36).slice(2), // 保证唯一
    title: '',
    value: '',
    remark: '',
    media: '',
    showTitle: true,
    showValue: true,
    showRemark: false,
    showMedia: false,
    titleFontSize: 16,
    valueFontSize: 16,
    remarkFontSize: 14,
    titleColor: '#333',
    valueColor: '#333',
    remarkColor: '#666'
  };

  showAddDialog.value = true;
}

async function saveAddForm(form) {
  try {
    await ElMessageBox.confirm('Are you sure you want to save this form?', 'Save Confirmation', {
      confirmButtonText: 'Save',
      cancelButtonText: 'Cancel',
      type: 'warning'
    });

    if (!form.title?.trim() && !form.value?.trim() && !form.remark?.trim()) {
      await ElMessageBox.alert('Please fill at least one field (title, value or remark)', 'Warning', {
        confirmButtonText: 'OK',
        type: 'warning'
      });
      return;
    }

    const newForm = {
      ...form,
      id: Date.now().toString() + Math.random().toString(36).slice(2),
      zIndex: 1
    };

    // 分配初始位置，避免重叠
    assignFormPosition(newForm);

    forms.value.push(newForm);
    debouncedSavePositions();

    ElMessage.success('Form saved successfully');

    // 触发表单更新事件
    emits('update:modelValue', forms.value);
    emits('form-updated', forms.value);

    showAddDialog.value = false;
    addFormData.value = null;
  } catch (error) {
    if (error?.action !== 'cancel') {
      console.error('Save error:', error);
      ElMessage.error('Save failed: ' + (error.message || 'Server error'));
    }
  }
}

function assignFormPosition(newForm) {
  // 统计当前所有已用的坐标点，避免重叠
  const usedPositions = Object.values(positions).map(pos => `${pos.x},${pos.y}`);

  // 分配初始位置
  const offsetStep = 40;
  const baseOffset = 10;
  
  // 确保容器宽度是数字
  let containerWidth = props.pageSize ? parseInt(props.pageSize.width) : 800;
  let maxPerRow = Math.floor((containerWidth - baseOffset) / (minSize.width + offsetStep));

  if (maxPerRow < 1) maxPerRow = 1;

  let initX = baseOffset;
  let initY = baseOffset;

  if (forms.value.length === 0) {
    // 第一个表单固定在左上角
    initX = baseOffset;
    initY = baseOffset;
  } else {
    // 寻找未被占用的位置
    let found = false;
    let tryCount = 0;

    while (!found && tryCount < 200) {
      let row = Math.floor(tryCount / maxPerRow);
      let col = tryCount % maxPerRow;

      initX = baseOffset + col * (minSize.width + offsetStep);
      initY = baseOffset + row * (minSize.height + offsetStep);

      const key = `${initX},${initY}`;

      if (!usedPositions.includes(key)) {
        found = true;
        break;
      }

      tryCount++;
    }
  }

  positions[newForm.id] = { x: initX, y: initY };
  cardSizes[newForm.id] = { ...minSize };
  validatePositionById(newForm.id);
}

function cancelAddForm() {
  showAddDialog.value = false;
  addFormData.value = null;
}

// 监听表单数据变化
watch(() => props.modelValue, (newValue) => {
  // 确保newValue是可迭代的对象或数组
  if (newValue && typeof newValue === 'object') {
    if (Array.isArray(newValue)) {
      // 处理数组情况，使用structuredClone确保深拷贝
      forms.value = Array.from(newValue);
    } else {
      // 处理单个对象情况
      console.warn('[FormCard] modelValue 应该是数组，但收到的是对象:', newValue);
      forms.value = [structuredClone(newValue)];
    }
    
    // 确保所有表单都有唯一的id
    forms.value = forms.value.map(form => ({
      ...form,
      id: form.id || Date.now().toString() + Math.random().toString(36).slice(2)
    }));
    
    nextTick(() => {
      updateVisibleForms();
    });
  } else {
    // 非对象/数组值，设置为空数组
    forms.value = [];
    console.warn('[FormCard] modelValue 应该是数组或对象，但收到:', newValue);
  }
}, { immediate: true, deep: true });
</script>

<style scoped>
.form-grid {
  position: relative;
  min-height: 300px;
  transition: opacity 0.3s ease;
}

.form-card-wrapper {
  position: absolute;
  transition: transform 0.2s ease, box-shadow 0.3s ease, opacity 0.3s ease;
  will-change: transform, opacity;
}

.form-card-wrapper:active {
  cursor: grabbing;
}

.form-card-wrapper.dragging {
  opacity: 0.7;
  box-shadow: 0 8px 20px rgba(0, 0, 0, 0.15);
  z-index: 100;
}

.form-card-mini {
  margin: 0;
  align-self: start;
  display: inline-block;
  vertical-align: top;
  cursor: pointer;
  background: #f8f8f8;
  transition: box-shadow 0.2s, border-color 0.2s;
  border: 1px solid transparent;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.05);
  position: absolute !important;
  transform: none !important;
  overflow: hidden;
}

.form-card-mini:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.12);
  border: 1px solid #ddd;
}

.form-card-mini.no-style {
  background: none;
  box-shadow: none;
  border: 1px solid transparent;
}

.resize-handle {
  position: absolute;
  right: 2px;
  bottom: 2px;
  width: 16px;
  height: 16px;
  background: rgba(0, 0, 0, 0.08);
  border-radius: 3px;
  cursor: se-resize;
  user-select: none;
  pointer-events: auto;
  z-index: 1000;
  transition: background-color 0.2s;
}

.resize-handle:hover {
  background: rgba(64, 158, 255, 0.7);
}

.form-card-mini .resize-handle {
  display: none;
}

.form-card-mini:hover .resize-handle {
  display: block;
}

.card-menu {
  position: absolute;
  top: 0;
  right: 0;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 4px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.15);
  z-index: 10;
  padding: 8px;
}

.menu-grid {
  display: grid;
  gap: 8px;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

.add-form-button {
  position: fixed;
  bottom: 24px;
  right: 24px;
  z-index: 1000;
  box-shadow: 0 3px 10px rgba(0, 0, 0, 0.2);
}

/* 响应式设计 */
@media (max-width: 768px) {
  .form-grid {
    width: 100%;
    padding: 8px;
  }

  .resize-handle {
    width: 20px;
    height: 20px;
  }

  .menu-grid {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .menu-grid .el-button {
    width: 100%;
    justify-content: center;
  }
}
</style>

