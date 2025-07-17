<template>
  <el-container>
    <Toolbar :onAddPage="onAddPage" :clearAllPages="clearAllPages" :rotatePage="rotateCurrentPage"
      :showRotateButton="pages.length > 0" />
    <el-aside width="220px">
      <el-menu :default-active="String(currentPageIdx)" @select="onSelectPage">
        <el-menu-item v-for="(page, idx) in pages" :key="page.id" :index="String(idx)"
          style="display:flex;align-items:center;justify-content:space-between;">
          <span v-if="!editIdxMap[idx]" @dblclick="editPageName(idx)">
            {{ page.name || 'New Page' }}
            <el-icon style="margin-left:4px;cursor:pointer;" @click.stop="editPageName(idx)">
              <edit />
            </el-icon>
          </span>
          <el-input v-else v-model="editName" size="small" @blur="savePageName(idx)" @keyup.enter="savePageName(idx)"
            style="width:120px;" autofocus />
          <el-popconfirm title="Are you sure to delete this page?" @confirm="deletePage(idx)"
            confirm-button-text="Delete" cancel-button-text="Cancel">
            <template #reference>
              <el-icon style="margin-left:8px;cursor:pointer;color:#f56c6c;">
                <delete />
              </el-icon>
            </template>
          </el-popconfirm>
          <el-icon style="margin-left:4px;cursor:pointer;" @click.stop="toggleEditPage(idx)">
            <Lock v-if="editPageIdx !== idx" />
            <svg v-else width="1em" height="1em" viewBox="0 0 1024 1024" fill="currentColor">
              <path
                d="M512 128c-106 0-192 86-192 192v96h-32c-17.7 0-32 14.3-32 32v384c0 35.3 28.7 64 64 64h384c35.3 0 64-28.7 64-64V448c0-17.7-14.3-32-32-32h-32v-96c0-106-86-192-192-192zm-128 192c0-70.7 57.3-128 128-128s128 57.3 128 128v96H384v-96zm352 128v384c0 17.7-14.3 32-32 32H320c-17.7 0-32-14.3-32-32V448h480z" />
            </svg>
          </el-icon>
          <!-- 添加旋转按钮 -->
          <el-icon style="margin-left:4px;cursor:pointer;" @click.stop="rotatePage(idx)" title="旋转页面">
            <svg width="1em" height="1em" viewBox="0 0 1024 1024" fill="currentColor">
              <path
                d="M784 464h-80c-35.3 0-64 28.7-64 64v80c0 35.3 28.7 64 64 64h80c35.3 0 64-28.7 64-64v-80c0-35.3-28.7-64-64-64zm-80 128v-80h80v80h-80zm-392-64v80c0 35.3 28.7 64 64 64h80c35.3 0 64-28.7 64-64v-80c0-35.3-28.7-64-64-64h-80c-35.3 0-64 28.7-64 64zm64-64h80v80h-80v-80zm408-320H240c-88.2 0-160 71.8-160 160v416c0 88.2 71.8 160 160 160h544c88.2 0 160-71.8 160-160V304c0-88.2-71.8-160-160-160zm96 576c0 52.9-43.1 96-96 96H240c-52.9 0-96-43.1-96-96V304c0-52.9 43.1-96 96-96h544c52.9 0 96 43.1 96 96v416z" />
              <path
                d="M756 872H268c-5.5 0-10 4.5-10 10v46c0 5.5 4.5 10 10 10h488c5.5 0 10-4.5 10-10v-46c0-5.5-4.5-10-10-10zM258 114h508c5.5 0 10-4.5 10-10V58c0-5.5-4.5-10-10-10H258c-5.5 0-10 4.5-10 10v46c0 5.5 4.5 10 10 10z" />
              <animateTransform attributeName="transform" type="rotate" from="0 512 512" to="360 512 512" dur="1s"
                begin="mouseover" fill="freeze" />
            </svg>
          </el-icon>
        </el-menu-item>
      </el-menu>

      <!-- 页面类型选择 -->
      <div class="page-type-selector">
        <el-radio-group v-model="currentPageType" size="small" @change="handlePageTypeChange">
          <el-radio-button label="form">Form</el-radio-button>
          <el-radio-button label="cards">Cards</el-radio-button>
        </el-radio-group>
      </div>
    </el-aside>
    <el-main>
      <!-- Form Page -->
      <template v-if="currentPageType === 'form'">
        <FormGrid v-show="currentPageType === 'form'" v-if="pages[currentPageIdx]" v-model="pages[currentPageIdx].forms"
          :editable="editPageIdx === currentPageIdx" :cardStyleOn="cardStyleOn"
          :pageSize="pages[currentPageIdx]?.pageSize" ref="formGridRef" />
        <FloatingBar v-show="currentPageType === 'form'" v-if="pages[currentPageIdx]"
          :clearCurrentPageForms="clearCurrentPageForms" :editable="editPageIdx === currentPageIdx"
          :onToggleCardStyle="onToggleCardStyle" @add-form="onAddForm" />
      </template>

      <!-- Card Converter Page -->
      <SimpleCardConverter v-show="currentPageType === 'cards'" ref="cardConverterRef"
        :key="`${currentPageType}-${currentPageIdx}`" />
    </el-main>
  </el-container>

  <!-- 纸张尺寸选择对话框 -->
  <el-dialog v-model="showPageSizeDialog" title="选择纸张尺寸" width="500px">
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
import SimpleCardConverter from '../components/SimpleCardConverter.vue'
import { Edit, Delete, Lock } from '@element-plus/icons-vue'
const { pages, addPage, updatePage, removePage, rotatePageOrientation } = usePages()
const currentPageIdx = ref(0)
const editIdxMap = reactive({})
const editName = ref('')
const editPageIdx = ref(-1)
const cardStyleOn = ref(true)
const formGridRef = ref(null)
const cardConverterRef = ref(null) // SimpleCardConverter组件引用
const currentPageType = ref('form') // 当前页面类型：form 或 cards

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
    if (currentPageType.value === 'cards' && cardConverterRef.value && pageSizeObj) {
      nextTick(() => {
        cardConverterRef.value.setPageSize(pageSizeObj)
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

// 添加旋转页面方法
function rotatePage(idx) {
  if (idx < 0 || idx >= pages.value.length) return

  // 获取要旋转的页面
  const page = pages.value[idx]
  if (!page) return

  // 判断是否为当前页面
  if (idx !== currentPageIdx.value) {
    // 如果不是当前页面，只交换尺寸不做视觉效果
    rotatePageOrientation(idx)
    ElMessage.success(`页面"${page.name}"已旋转`)
    return
  }

  // 交换页面尺寸的宽高
  rotatePageOrientation(idx)

  // 获取新的方向状态
  const isLandscape = page.orientation === 'landscape'

  // 获取需要旋转的内容容器 - 针对不同类型的页面获取相应的容器
  let contentContainer = null
  if (currentPageType.value === 'form') {
    // 表单页面
    contentContainer = document.querySelector('.form-grid')
  } else if (currentPageType.value === 'char') {
    // 汉字卡片页面
    contentContainer = document.querySelector('.card-manager-container')
  } else if (currentPageType.value === 'cards') {
    // Cards页面
    contentContainer = document.querySelector('.cards-container')
  } else if (currentPageType.value === 'convert') {
    // Convert to Cards页面
    contentContainer = document.querySelector('.convert-container')
  }

  // 如果找到内容容器，旋转整个容器
  if (contentContainer) {
    // 为容器添加整体旋转样式
    contentContainer.style.transform = isLandscape ? 'rotate(90deg)' : ''
    contentContainer.style.transformOrigin = 'center center'
    contentContainer.style.transition = 'transform 0.3s ease'

    // 调整容器位置，使旋转后居中
    if (isLandscape) {
      // 横向模式时添加额外空间
      contentContainer.style.margin = '80px auto'
      contentContainer.style.maxWidth = 'calc(100% - 160px)' // 确保旋转后不超出视图
    } else {
      // 纵向模式恢复正常
      contentContainer.style.margin = '0 auto'
      contentContainer.style.maxWidth = '100%'
    }

    // 为所有卡片应用适当的样式，确保在旋转后仍然可读
    const cards = contentContainer.querySelectorAll('.form-card-mini, .chinese-char-card, .card-item')
    if (cards && cards.length > 0) {
      cards.forEach(card => {
        card.style.transition = 'all 0.3s ease'
        // 可以在这里添加卡片级别的额外样式调整
      })
    }
  }

  // 显示旋转提示
  ElMessage.success(`页面"${page.name}"已旋转为${isLandscape ? '横向' : '纵向'}模式`)

  // 更新页面渲染
  nextTick(() => {
    // 更新页面内容尺寸 - 针对所有类型的页面
    if (currentPageType.value === 'form' && formGridRef.value) {
      // FormGrid会自动使用最新的页面尺寸
    } else if (currentPageType.value === 'cards' && cardConverterRef.value) {
      cardConverterRef.value.setPageSize(page.pageSize)
    }
  })
}

// 添加旋转当前页面的方法（供Toolbar使用）
function rotateCurrentPage() {
  if (pages.value.length === 0) {
    ElMessage.warning('没有可旋转的页面')
    return
  }

  rotatePage(currentPageIdx.value)
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

function handlePageTypeChange(newType) {
  console.log('切换页面类型:', newType, '当前页面类型:', currentPageType.value)
  if (newType === 'cards') {
    if (pages.value[currentPageIdx.value]?.forms?.length) {
      convertFormsToCards()
    } else {
      ElMessage.warning('当前页面没有可转换的表单内容')
      currentPageType.value = 'form'
      return
    }
  }
  currentPageType.value = newType
}

function convertFormsToCards() {
  const currentPage = pages.value[currentPageIdx.value]
  if (!currentPage?.forms?.length) {
    ElMessage.warning('当前页面没有表单内容')
    return
  }

  console.log('Converting forms to cards:', currentPage.forms)

  nextTick(() => {
    if (cardConverterRef.value) {
      cardConverterRef.value.convertFormsToCards(currentPage.forms)
      ElMessage.success(`已转换 ${currentPage.forms.length} 个表单`)
    } else {
      ElMessage.error('卡片转换组件未加载')
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


/* 旋转页面的基本样式 */
.el-main {
  overflow: auto;
  position: relative;
  min-height: 400px;
  padding: 20px;
  transition: padding 0.3s ease;
}

/* 所有可旋转容器的基本过渡效果 */
.form-grid,
.cards-container {
  transition: transform 0.3s ease, margin 0.3s ease, max-width 0.3s ease;
  transform-origin: center center;
}

/* 确保在旋转状态下内容居中 */
.el-main {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
}

/* 横向模式下容器内的卡片组调整 */
[class*="-container"][style*="rotate(90deg)"] .card-group,
[class*="-container"][style*="rotate(90deg)"] .cards-grid,
[class*="-container"][style*="rotate(90deg)"] .convert-grid {
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
}

/* 确保卡片在旋转容器中正确显示 */
[class*="-container"][style*="rotate(90deg)"] .card-item,
[class*="-container"][style*="rotate(90deg)"] .form-card-mini,
[class*="-container"][style*="rotate(90deg)"] .chinese-char-card {
  margin: 10px;
  transition: all 0.3s ease;
}
</style>