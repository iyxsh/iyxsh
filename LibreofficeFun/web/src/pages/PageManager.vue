<template>
  <el-container>
    <Toolbar :onAddPage="onAddPage" :clearAllPages="clearAllPages" />
    <el-aside width="220px">
      <el-menu :default-active="String(currentPageIdx)" @select="onSelectPage">
        <el-menu-item
          v-for="(page, idx) in pages"
          :key="page.id"
          :index="String(idx)"
          style="display:flex;align-items:center;justify-content:space-between;"
        >
          <span v-if="!editIdxMap[idx]" @dblclick="editPageName(idx)">
            {{ page.name || 'New Page' }}
            <el-icon style="margin-left:4px;cursor:pointer;" @click.stop="editPageName(idx)"><edit /></el-icon>
          </span>
          <el-input
            v-else
            v-model="editName"
            size="small"
            @blur="savePageName(idx)"
            @keyup.enter="savePageName(idx)"
            style="width:120px;"
            autofocus
          />
          <el-popconfirm
            title="Are you sure to delete this page?"
            @confirm="deletePage(idx)"
            confirm-button-text="Delete"
            cancel-button-text="Cancel"
          >
            <template #reference>
              <el-icon style="margin-left:8px;cursor:pointer;color:#f56c6c;"><delete /></el-icon>
            </template>
          </el-popconfirm>
          <el-icon style="margin-left:4px;cursor:pointer;" @click.stop="toggleEditPage(idx)">
            <Lock v-if="editPageIdx !== idx" />
            <svg v-else width="1em" height="1em" viewBox="0 0 1024 1024" fill="currentColor"><path d="M512 128c-106 0-192 86-192 192v96h-32c-17.7 0-32 14.3-32 32v384c0 35.3 28.7 64 64 64h384c35.3 0 64-28.7 64-64V448c0-17.7-14.3-32-32-32h-32v-96c0-106-86-192-192-192zm-128 192c0-70.7 57.3-128 128-128s128 57.3 128 128v96H384v-96zm352 128v384c0 17.7-14.3 32-32 32H320c-17.7 0-32-14.3-32-32V448h480z"/></svg>
          </el-icon>
        </el-menu-item>
      </el-menu>
    </el-aside>
    <el-main>
      <FormGrid v-if="pages[currentPageIdx]" v-model="pages[currentPageIdx].forms" :addForm="addForm" :editable="editPageIdx === currentPageIdx" :cardStyleOn="cardStyleOn" />
      <FloatingBar v-if="pages[currentPageIdx]" :clearCurrentPageForms="clearCurrentPageForms" :addForm="addForm" :editable="editPageIdx === currentPageIdx" :onToggleCardStyle="onToggleCardStyle" />
    </el-main>
  </el-container>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { usePages } from '../store/usePages'
import FormGrid from '../components/FormGrid.vue'
import Toolbar from '../components/Toolbar.vue'
import FloatingBar from '../components/FloatingBar.vue'
import { Edit, Delete, Lock } from '@element-plus/icons-vue'
const { pages, addPage, updatePage, removePage } = usePages()
const currentPageIdx = ref(0)
const editIdxMap = reactive({})
const editName = ref('')
const editPageIdx = ref(-1)
const cardStyleOn = ref(true)
function onSelectPage(idx) {
  currentPageIdx.value = Number(idx)
}
function editPageName(idx) {
  editIdxMap[idx] = true
  editName.value = pages.value[idx].name || 'new page'
}
function savePageName(idx) {
  if (editName.value.trim()) {
    updatePage(idx, { name: editName.value.trim() })
  }
  editIdxMap[idx] = false
}
function deletePage(idx) {
  removePage(idx)
  if (currentPageIdx.value >= pages.value.length) {
    currentPageIdx.value = Math.max(0, pages.value.length - 1)
  }
  if (editPageIdx.value === idx) editPageIdx.value = -1
}
function toggleEditPage(idx) {
  editPageIdx.value = editPageIdx.value === idx ? -1 : idx
}
function onAddPage() {
  addPage('new page')
}
function clearAllPages() {
  localStorage.removeItem('form-pages')
  location.reload()
}
function clearCurrentPageForms() {
  if (pages.value[currentPageIdx.value]) {
    pages.value[currentPageIdx.value].forms = []
  }
}
function addForm() {
  if (pages.value[currentPageIdx.value]) {
    pages.value[currentPageIdx.value].forms.push({ id: Date.now(), title: '', value: '', remark: '', media: '' })
  }
}
function onToggleCardStyle(val) {
  cardStyleOn.value = val
}
</script>

<style scoped>
.el-aside {
  background: #f5f7fa;
  min-height: 100vh;
}
</style>
