<template>
  <div class="page-manager">
    <h1>页面管理器</h1>

    <!-- 导航区域 -->
    <div class="navigation">
      <router-link to="/" class="home-link">返回首页</router-link>
    </div>

    <!-- 页面管理功能 -->
    <el-container class="main-container">
      <Toolbar :onAddPage="onAddPage" :clearAllPages="clearAllPages" :rotatePage="rotateCurrentPage"
        :showRotateButton="pages.length > 0" />
      <el-aside width="220px">
        <el-menu :default-active="String(currentPageIdx)" @select="onSelectPage">
          <el-menu-item v-for="(page, idx) in pages" :key="page.id" :index="String(idx)" class="page-menu-item">
            <span v-if="!editIdxMap[idx]" @dblclick="editPageName(idx)">
              {{ page.name || $t('pageManager.newPage') }}
              <el-icon class="edit-icon" @click.stop="editPageName(idx)">
                <Edit />
              </el-icon>
            </span>
            <el-input v-else v-model="editName" size="small" @blur="savePageName(idx)" @keyup.enter="savePageName(idx)"
              class="page-name-input" autofocus />
            <div class="page-actions">
              <el-popconfirm :title="$t('pageManager.confirmDelete')" @confirm="deletePage(idx)"
                confirm-button-text="删除" cancel-button-text="取消">
                <template #reference>
                  <el-icon class="delete-icon">
                    <Delete />
                  </el-icon>
                </template>
              </el-popconfirm>
              <el-icon class="lock-icon" @click.stop="toggleEditPage(idx)">
                <Lock v-if="editPageIdx !== idx" />
                <svg v-else width="1em" height="1em" viewBox="0 0 1024 1024" fill="currentColor">
                  <path
                    d="M512 128c-106 0-192 86-192 192v96h-32c-17.7 0-32 14.3-32 32v384c0 35.3 28.7 64 64 64h384c35.3 0 64-28.7 64-64V448c0-17.7-14.3-32-32-32h-32v-96c0-106-86-192-192-192zm-128 192c0-70.7 57.3-128 128-128s128 57.3 128 128v96H384v-96zm352 128v384c0 17.7-14.3 32-32 32H320c-17.7 0-32-14.3-32-32V448h480z" />
                </svg>
              </el-icon>
              <!-- 旋转按钮 -->
              <el-icon class="rotate-icon" @click.stop="rotatePage(idx)" title="旋转页面">
                <svg width="1em" height="1em" viewBox="0 0 1024 1024" fill="currentColor">
                  <path
                    d="M784 464h-80c-35.3 0-64 28.7-64 64v80c0 35.3 28.7 64 64 64h80c35.3 0 64-28.7 64-64v-80c0-35.3-28.7-64-64-64zm-80 128v-80h80v80h-80zm-392-64v80c0 35.3 28.7 64 64 64h80c35.3 0 64-28.7 64-64v-80c0-35.3-28.7-64-64-64zm64-64h80v80h-80v-80zm408-320H240c-88.2 0-160 71.8-160 160v416c0 88.2 71.8 160 160 160h544c88.2 0 160-71.8 160-160V304c0-88.2-71.8-160-160-160zm96 576c0 52.9-43.1 96-96 96H240c-52.9 0-96-43.1-96-96V304c0-52.9 43.1-96 96-96h544c52.9 0 96 43.1 96 96v416z" />
                  <path
                    d="M756 872H268c-5.5 0-10 4.5-10 10v46c0 5.5 4.5 10 10 10h488c5.5 0 10-4.5 10-10v-46c0-5.5-4.5-10-10-10zM258 114h508c5.5 0 10-4.5 10-10V58c0-5.5-4.5-10-10-10H258c-5.5 0-10 4.5-10 10v46c0 5.5 4.5 10 10 10z" />
                  <animateTransform attributeName="transform" type="rotate" from="0 512 512" to="360 512 512" dur="1s"
                    begin="mouseover" fill="freeze" />
                </svg>
              </el-icon>
            </div>
          </el-menu-item>
        </el-menu>

        <!-- 页面类型选择 -->
        <div class="page-type-selector">
          <el-radio-group v-model="currentPageType" size="small" @change="handlePageTypeChange">
            <el-radio-button label="form">{{ $t('pageManager.formPage') }}</el-radio-button>
            <el-radio-button label="cards">{{ $t('pageManager.cardPage') }}</el-radio-button>
          </el-radio-group>
        </div>
      </el-aside>
      <el-main>
        <!-- 表单页面 -->
        <template v-if="currentPageType === 'form'">
          <FormGrid v-show="currentPageType === 'form'" v-if="pages[currentPageIdx]"
            v-model="pages[currentPageIdx].forms" @update:modelValue="handleFormUpdate"
            :editable="editPageIdx === currentPageIdx" :cardStyleOn="cardStyleOn"
            :pageSize="pages[currentPageIdx]?.pageSize" ref="formGridRef" />
          <FloatingBar v-show="currentPageType === 'form'" v-if="pages[currentPageIdx]"
            :clearCurrentPageForms="clearCurrentPageForms" :editable="editPageIdx === currentPageIdx"
            :onToggleCardStyle="onToggleCardStyle" @add-form="onAddForm" />
        </template>

        <!-- 卡片转换页面 -->
        <SimpleCardConverter v-show="currentPageType === 'cards'" ref="cardConverterRef"
          :key="`${currentPageType}-${currentPageIdx}`" />
      </el-main>
    </el-container>

    <!-- 纸张尺寸选择对话框 -->
    <el-dialog 
      v-model="showPageSizeDialog" 
      :title="$t('pageManager.selectPageSize')" 
      width="650px"
      :before-close="handleDialogClose"
    >
      <div class="page-size-selector">
        <el-form label-position="top">
          <el-row :gutter="20">
            <el-col :span="8">
              <el-form-item :label="$t('pageManager.pageName')">
                <el-input v-model="newPageName" :placeholder="$t('pageManager.enterPageName')" autofocus></el-input>
              </el-form-item>
            </el-col>
            <el-col :span="16">
              <el-form-item :label="$t('pageManager.pageSize')">
              </el-form-item>
            </el-col>
          </el-row>

          <div class="paper-options-grid">
            <div v-for="(papers, category) in groupedPaperSizes" :key="category" class="paper-category">
              <div class="category-title">{{ category }}系列</div>
              <div class="papers-group">
                <div v-for="paper in papers" :key="paper.name" class="paper-option-compact" @click="selectPaper(paper)">
                  <el-radio :label="paper.name" class="paper-radio">
                    <div class="paper-preview-compact" :style="getPaperPreviewStyle(paper)"
                      :class="{ 'selected': selectedPageSize === paper.name }">
                      <span class="paper-name">{{ paper.name }}</span>
                      <span class="paper-dimensions">
                        {{ paper.width }}×{{ paper.height }} {{ paper.unit }}
                      </span>
                    </div>
                  </el-radio>
                </div>
              </div>
            </div>
          </div>
        </el-form>
      </div>
      <template #footer>
        <span class="dialog-footer">
          <el-button 
            @click="debugButton('cancel')"
            class="toolbar-button cancel-button"
          >
            <el-icon name="DocumentRemove"></el-icon>
            <span class="button-text">{{ $t('pageManager.cancel') }}</span>
          </el-button>
          <el-button 
            type="primary" 
            @click="debugButton('create')"
            :loading="isCreatingPage" 
            :disabled="!selectedPageSize"
            class="toolbar-button create-button"
          >
            <el-icon name="DocumentAdd"></el-icon>
            <span class="button-text">{{ $t('pageManager.create') }}</span>
          </el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted } from 'vue'
import { ElMessage, ElButton, ElIcon, ElMessageBox } from 'element-plus'
import { usePages } from '../stores/usePages'
import FormGrid from '../components/FormGrid.vue'
import Toolbar from '../components/Toolbar.vue'
import FloatingBar from '../components/FloatingBar.vue'
import SimpleCardConverter from '../components/SimpleCardConverter.vue'
import { Edit, Delete, Lock, DocumentRemove, DocumentAdd } from '@element-plus/icons-vue'
import { debounce } from 'lodash-es'
import { savePositions, clearPositions } from '../services/formPositionService'
import { getLocale, setLocale, t as $t } from '../utils/i18n'
import { useEventBus } from '../utils/eventBus'
import errorLogService from '@/services/errorLogService'

// 创建响应式对象
const { pages, addPage, updatePage, removePage, rotatePageOrientation } = usePages()
const currentPageIdx = ref(0)
const editIdxMap = reactive({})
const editName = ref('')
const editPageIdx = ref(-1)
const cardStyleOn = ref(true)
const formGridRef = ref(null)
const cardConverterRef = ref(null) // SimpleCardConverter组件引用
const currentPageType = ref('form') // 当前页面类型：form 或 cards

// i18n相关
const locale = ref(getLocale())

// 监听语言变化
watch(
  () => {
    // 通过window.$app访问全局属性，并添加空值检查
    return window.$app?.config?.globalProperties?.$locale
  },
  (newLocale) => {
    if (newLocale) {
      locale.value = newLocale
      // 可以添加更新UI的逻辑
    }
  }
)

// 纸张尺寸选择对话框相关
const showPageSizeDialog = ref(false)
const newPageName = ref($t('pageManager.newPage'))
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

// 处理表单更新
function handleFormUpdate(updatedForms) {
  localStorage.setItem('form-pages', JSON.stringify(pages.value))
}

// 在组件挂载时检查引用
onMounted(() => {
  console.log('PageManager 组件已挂载');
  console.log('FormGrid 引用:', formGridRef.value ? '存在' : '不存在');
  console.log('SimpleCardConverter 引用:', cardConverterRef.value ? '存在' : '不存在');

  // 添加空值检查
  if (formGridRef.value) {
    console.log('FormGrid 组件状态:', {
      isMounted: formGridRef.value.$el ? '已挂载' : '未挂载',
      isVisible: formGridRef.value.$el?.offsetParent !== null
    });
    
    errorLogService.addErrorLog(
      null,
      `FormGrid 组件状态: ${formGridRef.value.$el ? '已挂载' : '未挂载'}`,
      'debug'
    );
  }

  if (cardConverterRef.value) {
    console.log('SimpleCardConverter 组件状态:', {
      isMounted: cardConverterRef.value.$el ? '已挂载' : '未挂载',
      isVisible: cardConverterRef.value.$el?.offsetParent !== null
    });
    
    errorLogService.addErrorLog(
      null,
      `SimpleCardConverter 组件状态: ${cardConverterRef.value.$el ? '已挂载' : '未挂载'}`,
      'debug'
    );
  }
  
  errorLogService.addErrorLog(
    null,
    `PageManager 组件完整挂载状态: FormGrid ${formGridRef.value ? '存在' : '不存在'}, SimpleCardConverter ${cardConverterRef.value ? '存在' : '不存在'}`,
    'debug'
  );
});

// 修改 onAddForm 方法
const onAddForm = () => {
  // 检查页面是否可编辑
  if (editPageIdx.value !== currentPageIdx.value) {
    // 如果页面不可编辑，提示用户需要先解锁页面
    ElMessage.warning('请先解锁页面再添加表单')
    return
  }

  nextTick(() => {
    // 确保 formGridRef 存在并且已正确加载
    if (!formGridRef.value?.handleAddForm) {
      console.error('FormGrid reference is not available')
      ElMessage.error('无法添加表单：表单编辑器未就绪')
      return
    }

    // 直接调用 handleAddForm 方法
    formGridRef.value.handleAddForm()
  })
}

// 在组件挂载时注册全局事件
onMounted(() => {
  // 初始化事件总线
  const { on } = useEventBus()

  // 注册添加表单事件
  on('add-form', () => {
    onAddForm()
  })

  // 注册其他事件...
})

// 创建新页面
const isCreatingPage = ref(false)
const selectedPaper = ref(null)

// 选择纸张
const selectPaper = (paper) => {
  selectedPageSize.value = paper.name
  selectedPaper.value = paper
  
  console.log('[PageManager] 纸张选择:', {
    paperName: paper.name,
    paperWidth: paper.width,
    paperHeight: paper.height
  });
}

// 创建页面方法优化
const createNewPage = () => {
  console.log('[PageManager] 开始创建新页面，参数:', {
    pageName: newPageName.value,
    pageSize: selectedPageSize.value,
    pageType: currentPageType.value
  });
  
  if (!selectedPageSize.value) {
    const warningMessage = '未选择纸张尺寸';
    console.warn('[PageManager] 创建页面警告:', warningMessage);
    ElMessage.warning(warningMessage);
    
    errorLogService.addErrorLog(
      new Error(warningMessage),
      '创建页面失败 - 未选择纸张尺寸',
      'warning'
    );
    
    return
  }

  isCreatingPage.value = true

// 处理对话框关闭
const handleDialogClose = (done) => {
  if (isCreatingPage.value) {
    ElMessageBox.confirm(
      $t('pageManager.confirmCloseDuringCreation'),
      $t('pageManager.warning'),
      {
        confirmButtonText: $t('pageManager.confirm'),
        cancelButtonText: $t('pageManager.cancel'),
        type: 'warning'
      }
    ).then(() => {
      done()
    }).catch(() => {
      // 取消关闭
    })
  } else {
    done()
  }
}

  try {
    // 创建页面
    const newPageNameValue = newPageName.value || $t('pageManager.newPage')
    console.log('[PageManager] 创建页面，名称:', newPageNameValue, '尺寸:', selectedPageSize.value);
    
    // 获取所选纸张尺寸对象
    const pageSizeObj = paperSizes.find(p => p.name === selectedPageSize.value)
    
    // 添加详细的输入验证
    if (!pageSizeObj) {
      const errorMessage = `未找到纸张尺寸对象: ${selectedPageSize.value}`;
      console.error('[PageManager] 创建页面错误:', errorMessage);
      ElMessage.error('无法创建页面：纸张尺寸无效')
      
      errorLogService.addErrorLog(
        new Error(errorMessage),
        `创建页面失败 - 未找到纸张尺寸: ${selectedPageSize.value}`,
        'error'
      );
      
      isCreatingPage.value = false
      return
    }
    
    addPage(newPageNameValue, pageSizeObj)

    // 自动将新页面设置为可编辑状态
    nextTick(() => {
      const newPageIdx = pages.value.length - 1
      console.log('[PageManager] 页面创建成功，索引:', newPageIdx);
      
      currentPageIdx.value = newPageIdx
      editPageIdx.value = newPageIdx
      // 关闭对话框
      showPageSizeDialog.value = false
      // 重置输入
      newPageName.value = $t('pageManager.newPage')

      // 添加空值检查
      if (currentPageType.value === 'cards' && cardConverterRef.value && pageSizeObj && pages.value[newPageIdx]) {
        console.log('[PageManager] 设置卡片页面尺寸:', {
          ...pageSizeObj,
          orientation: pages.value[newPageIdx].orientation
        });
        
        cardConverterRef.value.setPageSize({
          ...pageSizeObj,
          orientation: pages.value[newPageIdx].orientation
        })
      }
      
      // 记录成功创建日志
      console.log('[PageManager] 页面创建成功:', {
        pageName: newPageNameValue,
        pageSize: selectedPageSize.value,
        pageIdx: newPageIdx
      });
      
      errorLogService.addErrorLog(
        null,
        `页面创建成功: ${newPageNameValue} (${selectedPageSize.value})`,
        'info'
      );
    })
  } catch (error) {
    console.error('创建页面时出错:', error)
    console.error('错误详细信息:', {
      errorType: typeof error,
      errorMessage: error.message,
      errorStack: error.stack,
      pagesState: JSON.stringify(pages.value, null, 2),
      currentPageIdx: currentPageIdx.value,
      selectedPageSize: selectedPageSize.value,
      pageSizeObj: pageSizeObj
    });
    
    ElMessage.error({
      message: $t('pageManager.pageCreationFailed'),
      duration: 5000
    })
    
    // 记录错误日志
    errorLogService.addErrorLog(
      error,
      `创建页面失败: ${newPageName.value || '未知名称'} (${selectedPageSize.value || '无尺寸'})`,
      'error'
    );
  } finally {
    isCreatingPage.value = false
  }
}

// 切换卡片样式
function onToggleCardStyle() {
  cardStyleOn.value = !cardStyleOn.value
}

// 选择页面
function onSelectPage(idx) {
  const numIdx = Number(idx)
  console.log('[PageManager] 开始选择页面，索引:', numIdx);
  
  if (numIdx < 0 || numIdx >= pages.value.length) {
    const warningMessage = `无效的页面索引: ${numIdx}`;
    console.warn('[PageManager] 选择页面警告:', warningMessage);
    ElMessage.warning(warningMessage);
    
    errorLogService.addErrorLog(
      new Error(warningMessage),
      `页面选择失败 - 无效索引: ${numIdx}`,
      'warning'
    );
    
    return;
  }

  try {
    currentPageIdx.value = numIdx
    
    // 如果当前页面被选中并且是编辑状态，保持编辑状态
    // 否则维持原有的锁定状态
    if (editPageIdx.value === currentPageIdx.value) {
      editPageIdx.value = numIdx
    }
    
    console.log('[PageManager] 页面选择成功:', {
      pageIndex: numIdx,
      pageName: pages.value[numIdx]?.name
    });
    
    errorLogService.addErrorLog(
      null,
      `页面选择成功: ${pages.value[numIdx]?.name} (${numIdx})`,
      'info'
    );
  } catch (error) {
    console.error('[PageManager] 选择页面时发生错误:', error);
    ElMessage.error('页面选择失败');
    
    errorLogService.addErrorLog(
      error,
      `页面选择失败: ${numIdx}`,
      'error'
    );
  }
}

// 编辑页面名称
function editPageName(idx) {
  editIdxMap[idx] = true
  editName.value = pages.value[idx].name || '新页面'
}

// 保存页面名称
function savePageName(idx) {
  if (editName.value.trim()) {
    updatePage(idx, { name: editName.value.trim() })
  }
  editIdxMap[idx] = false
}

// 删除页面
function deletePage(idx) {
  console.log('[PageManager] 开始删除页面，索引:', idx);
  
  if (idx < 0 || idx >= pages.value.length) {
    const warningMessage = `无效的页面索引: ${idx}`;
    console.warn('[PageManager] 删除页面警告:', warningMessage);
    ElMessage.warning(warningMessage);
    
    errorLogService.addErrorLog(
      new Error(warningMessage),
      `页面删除失败 - 无效索引: ${idx}`,
      'warning'
    );
    
    return;
  }
  
  // 显示确认对话框
  ElMessageBox.confirm(
    $t('pageManager.confirmDeletePage'),
    $t('pageManager.warning'),
    {
      confirmButtonText: $t('pageManager.confirm'),
      cancelButtonText: $t('pageManager.cancel'),
      type: 'warning'
    }
  ).then(() => {
    try {
      console.log('[PageManager] 用户确认删除页面:', {
        pageIndex: idx,
        pageName: pages.value[idx]?.name
      });
      
      removePage(idx)
      
      if (currentPageIdx.value >= pages.value.length) {
        currentPageIdx.value = Math.max(0, pages.value.length - 1)
      }
      if (editPageIdx.value === idx) editPageIdx.value = -1
      
      console.log('[PageManager] 页面删除成功:', {
        pageIndex: idx,
        pageName: pages.value[idx]?.name
      });
      
      errorLogService.addErrorLog(
        null,
        `页面删除成功: ${pages.value[idx]?.name} (${idx})`,
        'info'
      );
    } catch (error) {
      console.error('[PageManager] 删除页面时发生错误:', error);
      ElMessage.error('页面删除失败');
      
      errorLogService.addErrorLog(
        error,
        `页面删除失败: ${idx}`,
        'error'
      );
    }
  }).catch(() => {
    console.log('[PageManager] 页面删除操作取消:', idx);
    ElMessage.info('已取消删除');
    
    errorLogService.addErrorLog(
      null,
      `页面删除取消: ${idx}`,
      'info'
    );
  });
}

// 切换页面编辑状态
function toggleEditPage(idx) {
  editPageIdx.value = editPageIdx.value === idx ? -1 : idx
}

// 旋转页面
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
  } else if (currentPageType.value === 'cards') {
    // Cards页面
    contentContainer = document.querySelector('.cards-container')
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
      cardConverterRef.value.setPageSize({
        ...page.pageSize,
        orientation: page.orientation // 添加方向参数
      })
    }
  })
}

// 旋转当前页面的方法（供Toolbar使用）
function rotateCurrentPage() {
  if (pages.value.length === 0) {
    ElMessage.warning('没有可旋转的页面')
    return
  }

  rotatePage(currentPageIdx.value)
}

// 添加页面
function onAddPage() {
  // 显示纸张选择对话框
  showPageSizeDialog.value = true
}

// 清除所有页面
function clearAllPages() {
  localStorage.removeItem('form-pages')
  clearPositions()
  location.reload()
}

// 清除当前页面的表单
function clearCurrentPageForms() {
  if (pages.value[currentPageIdx.value]) {
    pages.value[currentPageIdx.value].forms = []
  }
}

// 处理页面类型变更
function handlePageTypeChange(newType) {
  currentPageType.value = newType
}

// 转换表单为卡片
function convertFormsToCards() {
  const currentPage = pages.value[currentPageIdx.value]
  if (!currentPage?.forms?.length) {
    ElMessage.warning('当前页面没有表单内容')
    return
  }

  // 添加有效性过滤
  const validForms = currentPage.forms.filter(form =>
    form.title || form.value || form.remark
  )

  if (validForms.length === 0) {
    ElMessage.warning('表单内容均为空，无法转换')
    return
  }

  nextTick(() => {
    if (cardConverterRef.value) {
      cardConverterRef.value.convertFormsToCards(currentPage.forms)
      ElMessage.success(`已转换 ${currentPage.forms.length} 个表单`)
    } else {
      ElMessage.error('卡片转换组件未加载')
      console.error('cardConverterRef 未定义')
    }
  })
}

// 调试按钮点击
const debugButton = (buttonName) => {
  console.log(`[PageManager] 按钮点击调试 - ${buttonName} 按钮被点击`);
  console.log(`[PageManager] i18n 翻译 - cancel: ${$t('pageManager.cancel')}, create: ${$t('pageManager.create')}`);
  console.log(`[PageManager] 按钮状态 - isCreatingPage: ${isCreatingPage.value}, selectedPageSize: ${selectedPageSize.value}`);

  if (buttonName === 'cancel') {
    showPageSizeDialog.value = false;
  } else {
    createNewPage();
  }
};
</script>

<style scoped>
.page-manager {
  padding: 20px;
}

.page-manager h1 {
  color: #2c3e50;
  margin-bottom: 20px;
  font-size: 24px;
}

.navigation {
  margin: 10px 0 20px;
}

.home-link {
  text-decoration: none;
  color: #42b883;
  display: inline-flex;
  align-items: center;
  font-size: 14px;
}

.home-link:hover {
  text-decoration: underline;
}

.main-container {
  margin-top: 20px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  overflow: hidden;
}

.page-menu-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: auto;
  padding: 10px 20px;
}

.page-name-input {
  width: 120px;
}

.page-actions {
  display: flex;
  align-items: center;
}

.edit-icon,
.delete-icon,
.lock-icon,
.rotate-icon {
  cursor: pointer;
  margin-left: 4px;
  font-size: 16px;
}

.delete-icon {
  color: #f56c6c;
}

.page-type-selector {
  margin-top: 20px;
  padding: 0 10px;
  display: flex;
  justify-content: center;
}

/* 纸张尺寸选择对话框样式 */
.page-size-selector {
  width: 100%;
  padding: 0 20px;
}

.paper-options-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 25px;
  margin-top: 10px;
}

.paper-category {
  margin-bottom: 25px;
}

.category-title {
  font-size: 16px;
  font-weight: 600;
  margin-bottom: 12px;
  color: #303133;
  border-left: 4px solid #42b883;
  padding-left: 8px;
  background-color: #f5f7fa;
  border-radius: 2px;
  padding: 6px 8px;
}

.papers-group {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
  gap: 12px;
}

.paper-option-compact {
  margin-bottom: 8px;
  transition: transform 0.2s ease;
}

.paper-option-compact:hover {
  transform: translateY(-2px);
}

.paper-radio {
  width: 100%;
  margin: 0;
}

.paper-preview-compact {
  border: 1px solid #dcdfe6;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  margin-top: 3px;
  font-size: 11px;
  color: #606266;
  background-color: white;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  transition: all 0.3s ease;
  border-radius: 4px;
  padding: 10px 6px;
  text-align: center;
  cursor: pointer;
  height: 110px;
}

.paper-preview-compact:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
  transform: translateY(-2px);
}

.paper-preview-compact.selected {
  border-color: #42b883;
  box-shadow: 0 2px 10px rgba(66, 184, 131, 0.25);
}

.paper-name {
  font-weight: 500;
  margin-bottom: 6px;
  color: #303133;
}

.paper-dimensions {
  font-size: 10px;
  color: #909399;
  margin-top: 4px;
}

/* 对话框按钮样式 */
.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  margin-top: 20px;
}

.dialog-footer .el-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  min-width: 120px;
  padding: 12px 16px;
  font-weight: 500;
  color: #303133;
  background-color: #f0f2f5;
  transition: all 0.2s ease;
}

.dialog-footer .el-button:hover {
  background-color: #e4e7ed;
}

.dialog-footer .el-button:active {
  background-color: #dcdfe6;
}

.dialog-footer .el-button .el-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-size: 16px;
  margin-right: 4px;
}

/* 响应式调整 */
@media (max-width: 768px) {
  .dialog-footer {
    flex-direction: column;
    align-items: flex-end;
  }

  .dialog-footer .el-button {
    width: 100%;
    justify-content: center;
    text-align: center;
  }

  .dialog-footer .el-button .el-icon {
    margin-right: 0;
    margin-bottom: 4px;
  }
}

/* 响应式设计优化 */
@media (max-width: 768px) {
  .paper-options-grid {
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
  }

  .papers-group {
    grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
  }

  .paper-preview-compact {
    height: 90px;
    padding: 8px 4px;
  }
}
</style>