<template>
  <draggable v-model="forms" item-key="id" class="form-grid" :animation="200">
    <template #item="{ element, index }">
      <div class="form-card-wrapper" @contextmenu.prevent="openMenu(index, $event)">
        <el-card v-if="showDetail[index] !== true" :class="['form-card-mini', cardStyleOn ? '' : 'no-style']" @dblclick="openDetail(index)"
          :style="getCardStyle(index)">
          <div v-if="element.showTitle !== false" :style="{fontSize: (element.titleFontSize || 16) + 'px', color: element.titleColor || '#333'}"><b>{{ element.title }}</b></div>
          <div v-if="element.showValue !== false" :style="{fontSize: (element.valueFontSize || 16) + 'px', color: element.valueColor || '#333'}">{{ element.value }}</div>
          <div v-if="element.showRemark !== false && element.remark" :style="{fontSize: (element.remarkFontSize || 14) + 'px', color: element.remarkColor || '#666'}">{{ element.remark }}</div>
          <div v-if="menuIndex === index" class="card-menu">
            <div class="menu-grid">
              <el-button icon="el-icon-edit" :disabled="!editable" @click.stop="editForm(index)">Edit</el-button>
              <el-button icon="el-icon-view" @click.stop="openDetail(index)">Detail</el-button>
              <el-button icon="el-icon-delete" type="danger" :disabled="!editable" @click.stop="onDelete(index)">Delete</el-button>
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
</template>

<script setup>
import { ref, watch, onMounted, computed, reactive } from 'vue'
import { ElButton, ElDialog, ElMessageBox } from 'element-plus'
import draggable from 'vuedraggable'
import FormCard from './FormCard.vue'
const props = defineProps({
  modelValue: Array,
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
</script>

<style scoped>
.form-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
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
