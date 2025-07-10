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
      
      <!-- 页面类型选择 -->
      <div class="page-type-selector">
        <el-radio-group v-model="currentPageType" size="small">
          <el-radio-button label="form">Form</el-radio-button>
          <el-radio-button label="char">Cards</el-radio-button>
        </el-radio-group>
      </div>
      
      <!-- 添加表单转汉字卡片的按钮 -->
      <div class="convert-button-container" v-if="currentPageType === 'form' && pages[currentPageIdx]?.forms?.length > 0">
        <el-button type="primary" size="small" @click="convertFormsToCharCards">
          <el-icon><Connection /></el-icon>
          Convert to Cards
        </el-button>
      </div>
    </el-aside>
    <el-main>
      <!-- Form Page -->
      <template v-if="currentPageType === 'form'">
        <FormGrid v-show="currentPageType === 'form'"
          v-if="pages[currentPageIdx]"
          v-model="pages[currentPageIdx].forms"
          :editable="editPageIdx === currentPageIdx"
          :cardStyleOn="cardStyleOn"
          :pageSize="pages[currentPageIdx]?.pageSize"
          ref="formGridRef"
        />
        <FloatingBar v-show="currentPageType === 'form'"
          v-if="pages[currentPageIdx]"
          :clearCurrentPageForms="clearCurrentPageForms"
          :editable="editPageIdx === currentPageIdx"
          :onToggleCardStyle="onToggleCardStyle"
          @add-form="onAddForm"
        />
      </template>
      
      <!-- Chinese Card Page -->
      <ChineseCardManager v-show="currentPageType === 'char'" ref="charCardRef" :key="`${currentPageType}-${currentPageIdx}`" :pageSize="pages[currentPageIdx]?.pageSize" />
    </el-main>
  </el-container>

  <!-- 纸张尺寸选择对话框 -->
  <el-dialog
    v-model="showPageSizeDialog"
    title="选择纸张尺寸"
    width="500px"
  >
    <div class="page-size-selector">
      <el-form label-position="top">
        <el-form-item label="页面名称">
          <el-input v-model="newPageName" placeholder="输入页面名称"></el-input>
        </el-form-item>
        <el-form-item label="纸张尺寸">
          <el-radio-group v-model="selectedPageSize">
            <div class="paper-options-grid">
              <div v-for="(papers, category) in groupedPaperSizes" :key="category" class="paper-category">
                <div class="category-title">{{ category }}系列</div>
                <div class="papers-group">
                  <div v-for="paper in papers" :key="paper.name" class="paper-option-compact">
                    <el-radio :label="paper.name">
                      <div class="paper-preview-compact" :style="getPaperPreviewStyle(paper)">
                        <span>{{ paper.name }}</span>
                        <span class="paper-dimensions">{{ paper.width }}×{{ paper.height }} {{ paper.unit }}</span>
                      </div>
                    </el-radio>
                  </div>
                </div>
              </div>
            </div>
          </el-radio-group>
        </el-form-item>
      </el-form>
    </div>
    <template #footer>
      <span class="dialog-footer">
        <el-button @click="showPageSizeDialog = false">取消</el-button>
        <el-button type="primary" @click="createNewPage">创建</el-button>
      </span>
    </template>
  </el-dialog>
</template>

<script setup>
import { ref, reactive, nextTick, computed } from 'vue'
import { ElMessage } from 'element-plus'
import { usePages } from '../store/usePages'
import FormGrid from '../components/FormGrid.vue'
import Toolbar from '../components/Toolbar.vue'
import FloatingBar from '../components/FloatingBar.vue'
import ChineseCardManager from '../components/chinesecardmanager.vue'
import { Edit, Delete, Lock, Connection } from '@element-plus/icons-vue'
const { pages, addPage, updatePage, removePage } = usePages()
const currentPageIdx = ref(0)
const editIdxMap = reactive({})
const editName = ref('')
const editPageIdx = ref(-1)
const cardStyleOn = ref(true)
const formGridRef = ref(null)
const charCardRef = ref(null) // 现在引用的是ChineseCardManager组件
const currentPageType = ref('form') // 当前页面类型：form 或 char

// 纸张尺寸选择对话框相关
const showPageSizeDialog = ref(false)
const newPageName = ref('新页面')
const selectedPageSize = ref('A4')

// 预定义纸张尺寸列表
const paperSizes = [
  { name: 'A3', width: 297, height: 420, unit: 'mm', category: 'A' },
  { name: 'A4', width: 210, height: 297, unit: 'mm', category: 'A' },
  { name: 'A5', width: 148, height: 210, unit: 'mm', category: 'A' },
  { name: 'B4', width: 250, height: 353, unit: 'mm', category: 'B' },
  { name: 'B5', width: 176, height: 250, unit: 'mm', category: 'B' },
  { name: 'Letter', width: 216, height: 279, unit: 'mm', category: '国际' },
  { name: 'Legal', width: 216, height: 356, unit: 'mm', category: '国际' },
  { name: 'Tabloid', width: 279, height: 432, unit: 'mm', category: '国际' },
  { name: '常见信纸', width: 148, height: 210, unit: 'mm', category: '其他' },
]

// 根据纸张尺寸生成预览样式
const getPaperPreviewStyle = (paper) => {
  // 计算比例，确保预览可以在界面上显示
  const maxWidth = 80
  const maxHeight = 100
  const ratio = Math.min(maxWidth / paper.width, maxHeight / paper.height)
  
  return {
    width: `${paper.width * ratio}px`,
    height: `${paper.height * ratio}px`,
    border: '1px solid #dcdfe6',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: '3px',
    fontSize: '10px',
    color: '#606266',
    backgroundColor: 'white',
    boxShadow: '0 1px 3px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.2s ease',
    borderRadius: '2px'
  }
}

// 添加分组的纸张列表计算属性
const groupedPaperSizes = computed(() => {
  const groups = {};
  paperSizes.forEach(paper => {
    if (!groups[paper.category]) {
      groups[paper.category] = [];
    }
    groups[paper.category].push(paper);
  });
  return groups;
});

// 创建新页面
const createNewPage = () => {
  // 获取所选纸张尺寸对象
  const pageSizeObj = paperSizes.find(p => p.name === selectedPageSize.value)
  
  // 创建页面
  addPage(newPageName.value || '新页面', pageSizeObj)
  
  // 自动将新页面设置为可编辑状态
  nextTick(() => {
    const newPageIdx = pages.value.length - 1
    currentPageIdx.value = newPageIdx
    editPageIdx.value = newPageIdx
    // 关闭对话框
    showPageSizeDialog.value = false
    // 重置输入
    newPageName.value = '新页面'
    
    // 无论选择哪种页面类型（表单或卡片），都应用选定的纸张大小
    if (currentPageType.value === 'char' && charCardRef.value && pageSizeObj) {
      nextTick(() => {
        charCardRef.value.setPageSize(pageSizeObj)
      })
    }
  })
}

// 切换卡片样式
function onToggleCardStyle() {
  cardStyleOn.value = !cardStyleOn.value
}
function onSelectPage(idx) {
  const numIdx = Number(idx)
  currentPageIdx.value = numIdx
  
  // 如果当前页面被选中并且是编辑状态，保持编辑状态
  // 否则维持原有的锁定状态
  if (editPageIdx.value === currentPageIdx.value) {
    editPageIdx.value = numIdx
  }
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
  // 显示纸张选择对话框
  showPageSizeDialog.value = true
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
function onAddForm() {
  // 检查页面是否可编辑
  if (editPageIdx.value !== currentPageIdx.value) {
    // 如果页面不可编辑，提示用户需要先解锁页面
    ElMessage.warning('Please unlock the page before adding forms')
    return
  }
  
  // 确保 formGridRef 存在并且已正确加载
  if (!formGridRef.value) {
    console.error('FormGrid reference is not available');
    ElMessage.error('Unable to add form: Form editor is not ready');
    return;
  }
  
  // 确保 handleAddForm 方法存在
  if (typeof formGridRef.value.handleAddForm !== 'function') {
    console.error('handleAddForm method not found in FormGrid');
    ElMessage.error('Unable to add form: Form editor method not available');
    return;
  }
  
  // 调用 FormGrid 的 handleAddForm 方法
  formGridRef.value.handleAddForm()
}
// 仅作为回调传递给 FormGrid，由 FormGrid 控制弹窗和添加
// 将表单内容转换为汉字卡片
function convertFormsToCharCards() {
  // 确保当前页面有表单内容
  if (!pages.value[currentPageIdx.value] || !pages.value[currentPageIdx.value].forms || pages.value[currentPageIdx.value].forms.length === 0) {
    ElMessage.warning('当前页面没有表单内容')
    return
  }

  // 切换到汉字卡片模式
  currentPageType.value = 'char'
  
  // 自动解锁页面以便编辑卡片
  editPageIdx.value = currentPageIdx.value

  // 等待组件加载完成
  nextTick(() => {
    // 确保汉字卡片管理组件已加载
    if (charCardRef.value) {
      // 先传递当前选择的纸张大小
      if (pages.value[currentPageIdx.value].pageSize) {
        charCardRef.value.setPageSize(pages.value[currentPageIdx.value].pageSize)
      }
      // 将表单内容传递给汉字卡片管理组件，该组件会自动分配表单到多个卡片组
      charCardRef.value.loadFormsToAllCards(pages.value[currentPageIdx.value].forms)
    } else {
      ElMessage.error('汉字卡片管理组件未加载')
    }
  })
}
</script>

<style scoped>
.page-type-selector {
  margin-top: 20px;
  padding: 0 10px;
  display: flex;
  justify-content: center;
}

.convert-button-container {
  margin-top: 10px;
  padding: 0 10px;
  display: flex;
  justify-content: center;
}

/* 纸张尺寸选择对话框样式 */
.page-size-selector {
  width: 100%;
}

.paper-options {
  display: flex;
  flex-wrap: wrap;
  gap: 20px;
  margin-top: 10px;
}

.paper-option {
  display: flex;
  flex-direction: column;
}

.paper-preview {
  margin: 5px 0;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  transition: all 0.3s;
}

.paper-preview:hover {
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

.paper-dimensions {
  display: block;
  font-size: 10px;
  margin-top: 5px;
}

/* 活动选择项样式 */
:deep(.el-radio__input.is-checked + .el-radio__label) .paper-preview {
  border: 2px solid #409eff !important;
  box-shadow: 0 4px 8px rgba(64, 158, 255, 0.3);
}
</style>