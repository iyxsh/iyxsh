<template>
  <div class="form-grid" :style="gridContainerStyle">
    <div v-for="(element, index) in forms" :key="element.id" class="form-card-wrapper"
      @contextmenu.prevent="openMenu(index, $event)" @mousedown.stop="startDrag($event, element, index)"
      :style="getCardAutoStyle(index, element)">
      <el-card v-if="showDetail[index] !== true" :class="['form-card-mini', cardStyleOn ? '' : 'no-style']"
        @dblclick="openDetail(index)" :body-style="{ padding: '8px' }" :style="{
          position: 'absolute',
          left: (positions[element.id]?.x || 0) + 'px',
          top: (positions[element.id]?.y || 0) + 'px',
          zIndex: element.zIndex,
          transform: 'none'
        }">
        <div ref="setContentRef" :data-id="element.id" :style="{ display: 'inline-block' }">
          <div v-if="element.showTitle !== false"
            :style="{ fontSize: (element.titleFontSize || 16) + 'px', color: element.titleColor || '#333' }"><b>{{
              element.title }}</b></div>
          <div v-if="element.showValue !== false"
            :style="{ fontSize: (element.valueFontSize || 16) + 'px', color: element.valueColor || '#333' }">{{
              element.value }}</div>
          <div v-if="element.showRemark !== false && element.remark"
            :style="{ fontSize: (element.remarkFontSize || 14) + 'px', color: element.remarkColor || '#666' }">{{
              element.remark }}</div>
          <div v-if="menuIndex === index" class="card-menu">
            <div class="menu-grid">
              <el-button :disabled="!editable" @click.stop="editForm(index)"><el-icon>
                  <EditPen />
                </el-icon>Edit</el-button>
              <el-button @click.stop="openDetail(index)"><el-icon>
                  <View />
                </el-icon>Detail</el-button>
              <el-button type="danger" :disabled="!editable" @click.stop="onDelete(index)"><el-icon>
                  <Delete />
                </el-icon>Delete</el-button>
            </div>
          </div>
        </div>
        <div class="resize-handle" @mousedown.stop="startResize($event, index)"></div>
      </el-card>
      <el-dialog v-else v-model="showDetail[index]" title="Form Detail" width="640px" @close="closeDetail(index)">
        <FormCard v-model="forms[index]" :editable="editable" @save="onSaveDetail(index, $event)"
          @delete="onDelete(index); closeDetail(index)" />
      </el-dialog>
    </div>
  </div>
  <!-- 将添加表单的弹窗移到最外层 -->
  <el-dialog v-model="showAddDialog" title="Add Form" width="640px" @close="cancelAddForm" append-to-body
    destroy-on-close>
    <FormCard v-if="addFormData" v-model="addFormData" :editable="true" @save="saveAddForm" @delete="cancelAddForm" />
  </el-dialog>
</template>

<script setup>
import { debounce } from 'lodash-es'
import { ref, watch, onMounted, computed, reactive, onBeforeUnmount } from 'vue'
import { ElButton, ElDialog, ElMessageBox, ElMessage } from 'element-plus'
import { EditPen, View, Delete } from '@element-plus/icons-vue'
import FormCard from './FormCard.vue'

const contentRefs = new Map()
const observedIds = new Set()

function setContentRef(el) {
  if (el && el.dataset && el.dataset.id) {
    contentRefs.set(el.dataset.id, el)
    observeContentSize(el.dataset.id, el)
  }
}

let resizeObserver = null
function observeContentSize(id, el) {
  if (!resizeObserver) {
    resizeObserver = new ResizeObserver(entries => {
      for (const entry of entries) {
        const id = entry.target.dataset.id
        if (id) {
          const { width, height } = entry.contentRect
          cardSizes[id] = {
            width: Math.ceil(width) + 16,  // 适当加点padding
            height: Math.ceil(height) + 16
          }
        }
      }
    })
  }
  if (!observedIds.has(id)) {
    resizeObserver.observe(el)
    observedIds.add(id)
  }
}

onBeforeUnmount(() => {
  if (resizeObserver) resizeObserver.disconnect()
})

// 用表单id做key，保证顺序变化后位置不丢失
const positions = reactive({}) // { [id]: { x, y } }
const cardSizes = reactive({}) // { [id]: { width, height } }
const minSize = { width: 180, height: 120 }
const maxSize = { width: 400, height: 300 }

const props = defineProps({
  modelValue: Array,
  pageSize: Object,
  addForm: Function,
  editable: Boolean,
  cardStyleOn: {
    type: Boolean,
    default: true
  }
})
const emit = defineEmits(['update:modelValue', 'position-change'])
const forms = ref(props.modelValue)
watch(() => props.modelValue, v => {
  forms.value = v
})
watch(forms, v => emit('update:modelValue', v), { deep: true })

const gridContainerStyle = computed(() => {
  if (props.pageSize) {
    const { width, height, unit } = props.pageSize
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
  return {}
})

const showDetail = ref([])
const menuIndex = ref(-1)

function onSaveDetail(index, data) {
  ElMessageBox.confirm('Please check the information carefully and click Confirm to save if there are no errors.', 'Save Confirmation', {
    confirmButtonText: 'Confirm',
    cancelButtonText: 'Cancel',
    type: 'warning',
  }).then(() => {
    forms.value[index] = { ...data }
    closeDetail(index)
    validatePositionById(forms.value[index].id)
    debouncedSavePositions()
  }).catch((error) => {
    if (error !== 'cancel') {
      console.error('保存错误:', error)
      ElMessage.error('保存失败')
    }
  })
}

function onDelete(index) {
  const id = forms.value[index].id
  forms.value.splice(index, 1)
  showDetail.value.splice(index, 1)
  delete positions[id]
  delete cardSizes[id]
  menuIndex.value = -1
  debouncedSavePositions()
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

// 拖拽相关
let draggingId = null, dragStart = { x: 0, y: 0 }, mouseStart = { x: 0, y: 0 }
let resizingId = null, resizeStart = { width: 0, height: 0 }, resizeMouseStart = { x: 0, y: 0 }

function startDrag(e, element, index) {
  if (!props.editable || e.target.classList.contains('resize-handle')) return
  draggingId = element.id
  mouseStart = { x: e.clientX, y: e.clientY }
  dragStart = { ...(positions[draggingId] || { x: 0, y: 0 }) }
  // 拖动时锁定宽高
  const size = cardSizes[draggingId] || { width: 180, height: 60 }
  cardSizes[draggingId] = { ...size }
  // 拖动前移除监听
  const el = contentRefs.get(draggingId)
  if (resizeObserver && el) resizeObserver.unobserve(el)
  document.addEventListener('mousemove', onDragging)
  document.addEventListener('mouseup', stopDrag)
}
function onDragging(e) {
  if (!draggingId) return
  const dx = e.clientX - mouseStart.x
  const dy = e.clientY - mouseStart.y
  const baseOffset = 10
  positions[draggingId] = {
    x: Math.max(baseOffset, dragStart.x + dx),
    y: Math.max(baseOffset, dragStart.y + dy)
  }
}
function stopDrag() {
  if (draggingId) {
    // 拖动结束后恢复监听
    const el = contentRefs.get(draggingId)
    if (resizeObserver && el) resizeObserver.observe(el)
    debouncedSavePositions()
  }
  draggingId = null
  document.removeEventListener('mousemove', onDragging)
  document.removeEventListener('mouseup', stopDrag)
}

function startResize(e, index) {
  if (!props.editable) return
  e.stopPropagation()
  const element = forms.value[index]
  resizingId = element.id
  resizeMouseStart = { x: e.clientX, y: e.clientY }
  const size = cardSizes[resizingId] || { width: 180, height: 60 }
  resizeStart = { width: size.width, height: size.height }
  // 缩放前移除监听
  const el = contentRefs.get(resizingId)
  if (resizeObserver && el) resizeObserver.unobserve(el)
  document.addEventListener('mousemove', onResizing)
  document.addEventListener('mouseup', stopResize)
}
function onResizing(e) {
  if (!resizingId) return
  const dx = e.clientX - resizeMouseStart.x
  const dy = e.clientY - resizeMouseStart.y
  cardSizes[resizingId] = {
    width: Math.max(60, resizeStart.width + dx),
    height: Math.max(40, resizeStart.height + dy)
  }
}
function stopResize() {
  if (resizingId) {
    // 缩放结束后恢复监听
    const el = contentRefs.get(resizingId)
    if (resizeObserver && el) resizeObserver.observe(el)
    debouncedSavePositions()
  }
  resizingId = null
  document.removeEventListener('mousemove', onResizing)
  document.removeEventListener('mouseup', stopResize)
}

// 添加防抖的存储函数
const debouncedSavePositions = debounce(() => {
  requestIdleCallback(() => {
    localStorage.setItem('formPositions', JSON.stringify(positions))
    localStorage.setItem('formSizes', JSON.stringify(cardSizes))
  })
}, 300)

function savePositions() {
  localStorage.setItem('formPositions', JSON.stringify(positions))
  localStorage.setItem('formSizes', JSON.stringify(cardSizes))
}
function loadPositions() {
  const savedPositions = localStorage.getItem('formPositions')
  const savedSizes = localStorage.getItem('formSizes')
  // 先清空再赋值，保证响应式
  Object.keys(positions).forEach(k => delete positions[k])
  Object.keys(cardSizes).forEach(k => delete cardSizes[k])
  if (savedPositions) {
    const obj = JSON.parse(savedPositions)
    Object.keys(obj).forEach(k => positions[k] = obj[k])
  }
  if (savedSizes) {
    const obj = JSON.parse(savedSizes)
    Object.keys(obj).forEach(k => cardSizes[k] = obj[k])
  }
  forms.value.forEach(f => validatePositionById(f.id))
}
function validatePositionById(id) {
  if (!props.pageSize) return
  const containerWidth = props.pageSize.width
  const containerHeight = props.pageSize.height
  const pos = positions[id] || { x: 0, y: 0 }
  const size = cardSizes[id] || minSize
  if (pos.x + size.width > containerWidth) {
    pos.x = Math.max(0, containerWidth - size.width)
  }
  if (pos.y + size.height > containerHeight) {
    pos.y = Math.max(0, containerHeight - size.height)
  }
  positions[id] = pos
  cardSizes[id] = size
}

watch(forms, (v) => {
  showDetail.value = v.map(() => false)
  v.forEach(f => validatePositionById(f.id))
  // 清理 positions 中无效的 key
  const validIds = new Set(v.map(f => f.id))
  Object.keys(positions).forEach(id => {
    if (!validIds.has(id)) {
      delete positions[id]
    }
  })
  Object.keys(cardSizes).forEach(id => {
    if (!validIds.has(id)) {
      delete cardSizes[id]
    }
  })
}, { immediate: true })

onMounted(() => {
  loadPositions()
  document.addEventListener('scroll', closeMenu)
})

const showAddDialog = ref(false)
const addFormData = ref(null)
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
  }
  showAddDialog.value = true
}
defineExpose({ handleAddForm })

async function saveAddForm(form) {
  try {
    await ElMessageBox.confirm('确认保存该表单吗？', '保存确认', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    if (!form.title?.trim() && !form.value?.trim() && !form.remark?.trim()) {
      await ElMessageBox.alert('请至少填写一个字段（标题、值或备注）', '警告', {
        confirmButtonText: '确定',
        type: 'warning'
      })
      return
    }

    const newForm = reactive({
      ...form,
      id: Date.now().toString() + Math.random().toString(36).slice(2),
      x: 0,
      y: 0,
      zIndex: 1
    })

    //forms.value.push(newForm)

    // 统计当前所有已用的坐标点，避免重叠
    const usedPositions = Object.values(positions).map(pos => `${pos.x},${pos.y}`)
    // 分配初始位置
    const offsetStep = 40
    const baseOffset = 10
    let containerWidth = props.pageSize ? props.pageSize.width : 800
    let maxPerRow = Math.floor((containerWidth - baseOffset) / (minSize.width + offsetStep))
    if (maxPerRow < 1) maxPerRow = 1

    let initX = baseOffset
    let initY = baseOffset
    if (forms.value.length === 0) {
      // 第一个表单固定在左上角
      initX = baseOffset
      initY = baseOffset
      console.log('usedPositions:', usedPositions)
      usedPositions.push(`${initX},${initY}`)
      console.log('第一个表单位置:', initX, initY)
    } else {
      // 后续表单自动分配不重叠位置
      let found = false
      let tryCount = 0
      while (!found && tryCount < 200) {
        let row = Math.floor(tryCount / maxPerRow)
        let col = tryCount % maxPerRow
        initX = baseOffset + col * (minSize.width + offsetStep)
        initY = baseOffset + row * (minSize.height + offsetStep)
        const key = `${initX},${initY}`
        if (!usedPositions.includes(key)) {
          found = true
          usedPositions.push(key)
          break
        }
        tryCount++
      }
      console.log(`分配位置: ${initX}, ${initY} (尝试次数: ${tryCount})`)
    }
    positions[newForm.id] = { x: initX, y: initY }
    cardSizes[newForm.id] = { ...minSize }
    validatePositionById(newForm.id)
    forms.value.push(newForm)
    debouncedSavePositions()
    ElMessage.success('表单保存成功')
  } catch (error) {
    if (error?.action !== 'cancel') {
      console.error('保存错误:', error)
      ElMessage.error('保存失败: ' + (error.message || '服务器错误'))
    }
  } finally {
    showAddDialog.value = false
    addFormData.value = null
  }
}
function cancelAddForm() {
  showAddDialog.value = false
  addFormData.value = null
}

function getCardAutoStyle(index, element) {
  const pos = positions[element.id] || { x: 0, y: 0 }
  const size = cardSizes[element.id] || { width: 180, height: 60 }
  return {
    width: size.width + 'px',
    height: size.height + 'px',
    position: 'absolute',
    left: pos.x + 'px',
    top: pos.y + 'px',
    zIndex: element.zIndex,
    transform: 'none'
  }
}
</script>

<style scoped>
.form-grid {
  position: relative;
  width: 100%;
  height: 100%;
  padding: 16px;
}

.form-card-wrapper {
  position: absolute;
  display: block;
  transition: transform 0.2s;
}

.card-menu {
  position: absolute;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);
  z-index: 10;
}

.menu-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 2px;
  padding: 4px;
  background: rgba(255, 255, 255, 0.95);
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
  margin: 0;
  align-self: start;
  display: inline-block;
  vertical-align: top;
  cursor: pointer;
  background: #f8f8f8;
  transition: box-shadow 0.2s, border-color 0.2s;
  border: 1px solid transparent;
  box-shadow: none;
  position: absolute !important;
  transform: none !important;
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
  background: rgba(0, 0, 0, 0.08);
  border-radius: 3px;
  cursor: se-resize;
  z-index: 2;
  user-select: none;
  pointer-events: auto;
  z-index: 1000;
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