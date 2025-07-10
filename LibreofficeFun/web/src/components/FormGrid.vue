<template>
  <draggable v-model="forms" item-key="id" class="form-grid" :style="gridContainerStyle" :animation="200">
    <template #item="{ element, index }">
      <div class="form-card-wrapper" @contextmenu.prevent="openMenu(index, $event)">
        <el-card v-if="showDetail[index] !== true" :class="['form-card-mini', cardStyleOn ? '' : 'no-style']" @dblclick="openDetail(index)"
          :style="getCardStyle(index)">
          <div v-if="element.showTitle !== false" :style="{fontSize: (element.titleFontSize || 16) + 'px', color: element.titleColor || '#333'}"><b>{{ element.title }}</b></div>
          <div v-if="element.showValue !== false" :style="{fontSize: (element.valueFontSize || 16) + 'px', color: element.valueColor || '#333'}">{{ element.value }}</div>
          <div v-if="element.showRemark !== false && element.remark" :style="{fontSize: (element.remarkFontSize || 14) + 'px', color: element.remarkColor || '#666'}">{{ element.remark }}</div>
          <div v-if="menuIndex === index" class="card-menu">
            <div class="menu-grid">
              <el-button :disabled="!editable" @click.stop="editForm(index)"><el-icon><EditPen /></el-icon>Edit</el-button>
              <el-button @click.stop="openDetail(index)"><el-icon><View /></el-icon>Detail</el-button>
              <el-button type="danger" :disabled="!editable" @click.stop="onDelete(index)"><el-icon><Delete /></el-icon>Delete</el-button>
            </div>
          </div>
          <div class="resize-handle" @mousedown.stop="startResize($event, index)"></div>
        </el-card>
        <el-dialog
          v-else
          v-model="showDetail[index]"
          title="Form Detail"
          width="640px"
          @close="closeDetail(index)"
        >
          <FormCard
            v-model="forms[index]"
            :editable="editable"
            @save="onSaveDetail(index, $event)"
            @delete="onDelete(index); closeDetail(index)"
          />
        </el-dialog>
      </div>
    </template>
  </draggable>
  <!-- 将添加表单的弹窗移到最外层 -->
  <el-dialog v-model="showAddDialog" title="Add Form" width="640px" @close="cancelAddForm" append-to-body destroy-on-close>
    <FormCard v-if="addFormData" v-model="addFormData" :editable="true" @save="saveAddForm" @delete="cancelAddForm" />
  </el-dialog>
</template>

<script setup>
import { ref, watch, onMounted, computed, reactive, nextTick } from 'vue'
import { ElButton, ElDialog, ElMessageBox } from 'element-plus'
import { EditPen, View, Delete } from '@element-plus/icons-vue'
import draggable from 'vuedraggable'
import FormCard from './FormCard.vue'
const props = defineProps({
  modelValue: Array,
  pageSize: Object, // 页面尺寸
  addForm: Function,
  editable: Boolean,
  cardStyleOn: {
    type: Boolean,
    default: true
  }
})
const emit = defineEmits(['update:modelValue'])
const forms = computed({
  get: () => props.modelValue,
  set: v => emit('update:modelValue', v)
})

// 根据页面尺寸计算表单容器样式
const gridContainerStyle = computed(() => {
  // 如果提供了页面尺寸，就应用页面尺寸
  if (props.pageSize) {
    const { width, height, unit } = props.pageSize
    
    // 将尺寸应用于网格容器，添加边距给内容
    return {
      width: `${width}${unit}`,
      minHeight: `${height}${unit}`,
      margin: '0 auto',
      border: '1px dashed #ccc',
      padding: '10px',
      backgroundColor: 'white',
      boxSizing: 'border-box',
      position: 'relative',
      overflow: 'auto'
    }
  }
  
  // 默认样式
  return {}
})
const showDetail = ref([])
const menuIndex = ref(-1)
const cardSizes = reactive({})
watch(forms, v => emit('update:modelValue', v), { deep: true })
function onSaveDetail(index, data) {
  ElMessageBox.confirm('Please check the information carefully and click Confirm to save if there are no errors.', 'Save Confirmation', {
    confirmButtonText: 'Confirm',
    cancelButtonText: 'Cancel',
    type: 'warning',
  }).then(() => {
    forms.value[index] = { ...data }
    closeDetail(index)
  })
  // 取消时不保存
}
function onDelete(index) {
  forms.value.splice(index, 1)
  showDetail.value.splice(index, 1)
  menuIndex.value = -1
}
function openDetail(index) {
  showDetail.value[index] = true
  menuIndex.value = -1
}
function closeDetail(index) {
  showDetail.value[index] = false
}
function openMenu(index, e) {
  menuIndex.value = index
  document.addEventListener('click', closeMenu)
}
function closeMenu() {
  menuIndex.value = -1
  document.removeEventListener('click', closeMenu)
}
function editForm(index) {
  openDetail(index)
}
function getCardStyle(index) {
  const size = cardSizes[index] || { width: 340, height: 'auto' }
  return { width: size.width + 'px', height: size.height }
}
let resizing = false, resizeIndex = null, startX = 0, startY = 0, startW = 0, startH = 0
function startResize(e, index) {
  if (!resizing) {
    resizing = true
    resizeIndex = index
    startX = e.clientX
    startY = e.clientY
    const el = e.target.closest('.el-card')
    startW = el.offsetWidth
    startH = el.offsetHeight
    document.addEventListener('mousemove', onResizing)
  } else {
    stopResize()
  }
}
function onResizing(e) {
  if (!resizing) return
  const dx = e.clientX - startX
  const dy = e.clientY - startY
  cardSizes[resizeIndex] = {
    width: Math.max(180, startW + dx),
    height: Math.max(120, startH + dy) + 'px'
  }
}
function stopResize() {
  resizing = false
  resizeIndex = null
  document.removeEventListener('mousemove', onResizing)
}
document.addEventListener('mouseup', () => { if (resizing) stopResize() })
// 初始化 showDetail
watch(forms, (v) => {
  showDetail.value = v.map(() => false)
}, { immediate: true })
onMounted(() => {
  document.addEventListener('scroll', closeMenu)
})
function getDisplayChars(element) {
  const chars = []
  if (element.showTitle !== false && element.title) {
    for (const c of element.title) {
      chars.push({ char: c, type: 'title' })
    }
  }
  if (element.showValue !== false && element.value) {
    for (const c of element.value) {
      chars.push({ char: c, type: 'value' })
    }
  }
  if (element.showRemark !== false && element.remark) {
    for (const c of element.remark) {
      chars.push({ char: c, type: 'remark' })
    }
  }
  return chars
}
function getCharStyle(element, type) {
  if (type === 'title') {
    return { fontSize: (element.titleFontSize || 16) + 'px', color: element.titleColor || '#333', textAlign: 'center' }
  }
  if (type === 'value') {
    return { fontSize: (element.valueFontSize || 16) + 'px', color: element.valueColor || '#333', textAlign: 'center' }
  }
  if (type === 'remark') {
    return { fontSize: (element.remarkFontSize || 14) + 'px', color: element.remarkColor || '#666', textAlign: 'center' }
  }
  return {}
}
const showAddDialog = ref(false)
const addFormData = ref(null)
function handleAddForm() {
  console.log('FormGrid.handleAddForm called');
  addFormData.value = {
    id: Date.now(),
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
  }
  // 确保使用nextTick来等待DOM更新
  showAddDialog.value = true;
  console.log('showAddDialog set to:', showAddDialog.value);
  
  // 使用Vue的nextTick确保DOM已更新
  nextTick(() => {
    console.log('showAddDialog in nextTick:', showAddDialog.value);
  });
}
defineExpose({ handleAddForm })
function saveAddForm(form) {
  // 不允许空卡片，必须至少填写 title、value 或 remark
  if (!(form.title && form.title.trim()) && !(form.value && form.value.trim()) && !(form.remark && form.remark.trim())) {
    ElMessageBox.alert('Please fill in at least one field (Title, Value or Remark) before saving.', 'Warning', {
      confirmButtonText: 'OK',
      type: 'warning',
    })
    return
  }
  forms.value.push({ ...form })
  showAddDialog.value = false
  addFormData.value = null
}
function cancelAddForm() {
  showAddDialog.value = false
  addFormData.value = null
}
</script>

<style scoped>
.form-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  background-color: #fff;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  transition: all 0.3s ease;
  margin: 0 auto;
}
.form-card-wrapper {
  position: relative;
  display: inline-block;
}
.card-menu {
  position: absolute;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);
  z-index: 10;
}
.menu-grid {
  display: grid;
  grid-template-columns: repeat(3, 40px);
  grid-template-rows: repeat(3, 40px);
  gap: 2px;
  background: rgba(255,255,255,0.95);
  border-radius: 8px;
  box-shadow: 0 2px 8px #ccc;
  padding: 4px;
  justify-items: center;
  align-items: center;
}
.menu-grid .el-button {
  width: 36px;
  height: 36px;
  padding: 0;
  font-size: 18px;
}
.form-card-mini {
  margin: 8px;
  width: 340px;
  display: inline-block;
  vertical-align: top;
  cursor: pointer;
  background: #f8f8f8;
  transition: box-shadow 0.2s, border-color 0.2s;
  border: 1px solid transparent;
  box-shadow: none;
  position: relative;
  resize: none;
}
.form-card-mini:hover {
  box-shadow: 0 2px 12px #aaa;
  border: 1px solid #bbb;
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
  background: rgba(0,0,0,0.08);
  border-radius: 3px;
  cursor: se-resize;
  z-index: 2;
  user-select: none;
}
.resize-handle:hover {
  background: #409eff;
}
.form-card-mini .resize-handle {
  display: none;
}
.form-card-mini:hover .resize-handle {
  display: block;
}
</style>
