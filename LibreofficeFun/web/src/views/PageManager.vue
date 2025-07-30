<template>
  <div class="page-manager">
    <h1>页面管理器引文</h1>

    <!-- 导航区域 -->
    <div class="navigation">
      <router-link to="/" class="home-link">返回首页</router-link>
    </div>

    <!-- 页面管理功能 -->
    <el-container class="main-container">
      <Toolbar :onAddPage="onAddPage" :clearAllPages="clearAllPages" :rotatePage="rotateCurrentPage"
        :showRotateButton="pages.length > 0" />
      <el-main>
        <!-- 表单页面 -->
        <template v-if="currentPageType === 'form'">
          <FormGrid v-if="pages[currentPageIdx] && Array.isArray(pages[currentPageIdx].forms)"
            v-model="pages[currentPageIdx].forms"
            :editable="editPageIdx === currentPageIdx" 
            :cardStyleOn="cardStyleOn"
            :pageSize="pages[currentPageIdx]?.pageSize" 
            @update:modelValue="handleFormUpdate"
            ref="formGridRef" />
          <FloatingBar v-if="pages[currentPageIdx]"
            :clearCurrentPageForms="clearCurrentPageForms" :editable="editPageIdx === currentPageIdx"
            :onToggleCardStyle="onToggleCardStyle" @add-form="onAddForm" />
        </template>

        <!-- 卡片转换页面 -->
        <template v-else-if="currentPageType === 'cards'">
          <SimpleCardConverter 
            :forms="pages[currentPageIdx] ? pages[currentPageIdx].forms : []"
            :pageSize="pages[currentPageIdx]?.pageSize" 
            ref="cardConverterRef"
            :key="`${currentPageType}-${currentPageIdx}`" />
        </template>
      </el-main>
      <!-- 可拖动侧边栏 -->
      <div class="resizable-aside-container" :style="{ width: asideWidth + 'px' }">
        <div 
          class="resize-handle" 
          @mousedown="startResize"
          :class="{ 'active': isResizing }"
        ></div>
        <el-aside class="resizable-aside" :width="asideWidth + 'px'">
          <div class="aside-header">
            <h3>页面管理</h3>
            <el-button 
              class="toggle-aside-btn" 
              link 
              @click="toggleAside"
            >
              <el-icon>
                <component :is="asideCollapsed ? 'ArrowLeft' : 'ArrowRight'" />
              </el-icon>
            </el-button>
          </div>
          
          <div v-show="!asideCollapsed" class="aside-content">
            <el-menu :default-active="String(currentPageIdx)" @select="onSelectPage">
              <el-menu-item v-for="(page, idx) in pages" :key="page.id" :index="String(idx)" class="page-menu-item">
                <span v-if="!editIdxMap[idx]" @dblclick="editPageName(idx)">
                  {{ page.name || i18nTrans('pageManager.newPage') }}
                  <el-icon class="edit-icon" @click.stop="editPageName(idx)">
                    <Edit />
                  </el-icon>
                </span>
                <el-input v-else v-model="editName" size="small" @blur="savePageName(idx)" @keyup.enter="savePageName(idx)"
                  class="page-name-input" autofocus />
                <div class="page-actions">
                  <el-popconfirm :title="i18nTrans('pageManager.confirmDelete')" @confirm="deletePage(idx)"
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
                        d="M512 128c-106 0-192 86-192 192v96h-32c-17.7 0-32 14.3-32 32v384c0 35.3 28.7 64 64 64h384c35.3 0 64-28.7 64-64V448c0-17.7-14.3-32-32-32h-32v-96c0-106-86-192-192-192zm-128 192c0-70.7 57.3-128 128-128s128 57.3 128 128v96H384v-96zm352 128v-80h80v80h-80zm-392-64v80c0 35.3 28.7 64 64 64h80c35.3 0 64-28.7 64-64v-80c0-35.3-28.7-64-64-64zm64-64h80v80h-80v-80zm408-320H240c-88.2 0-160 71.8-160 160v416c0 88.2 71.8 160 160 160h544c88.2 0 160-71.8 160-160V304c0-88.2-71.8-160-160-160zm96 576c0 52.9-43.1 96-96 96H240c-52.9 0-96-43.1-96-96V304c0-52.9 43.1-96 96-96h544c52.9 0 96 43.1 96 96v416z" />
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
                <el-radio-button value="form">{{ i18nTrans('pageManager.formPage') }}</el-radio-button>
                <el-radio-button value="cards">{{ i18nTrans('pageManager.cardPage') }}</el-radio-button>
              </el-radio-group>
            </div>
          </div>
        </el-aside>
      </div>
    </el-container>

    <!-- 纸张尺寸选择对话框 -->
    <el-dialog 
      v-model="showPageSizeDialog" 
      :title="i18nTrans('pageManager.selectPageSize')" 
      width="650px"
      :before-close="handleDialogClose"
    >
      <div class="page-size-selector">
        <el-form label-position="top">
          <el-row :gutter="20">
            <el-col :span="8">
              <el-form-item :label="i18nTrans('pageManager.pageName')">
                <el-input v-model="newPageName" :placeholder="i18nTrans('pageManager.enterPageName')" autofocus></el-input>
              </el-form-item>
            </el-col>
            <el-col :span="16">
              <el-form-item :label="i18nTrans('pageManager.pageSize')">
              </el-form-item>
            </el-col>
          </el-row>

          <div class="paper-options-grid">
            <div v-for="(papers, category) in groupedPaperSizes" :key="category" class="paper-category">
              <div class="category-title">{{ category }}系列</div>
              <div class="papers-group">
                <div v-for="paper in papers" :key="paper.name" class="paper-option-compact">
                  <el-radio 
                    :value="paper.name" 
                    class="paper-radio" 
                    v-model="selectedPageSize"
                    @change="selectPaper(paper)"
                  >
                    <div 
                      class="paper-preview-compact" 
                      :style="getPaperPreviewStyle(paper)"
                      :class="{ 'selected': selectedPageSize === paper.name }"
                    >
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
            <span class="button-text">{{ i18nTrans('pageManager.cancel') }}</span>
          </el-button>
          <el-button 
            type="primary" 
            @click="debugButton('create')"
            :loading="isCreatingPage" 
            :disabled="!selectedPageSize"
            class="toolbar-button create-button"
          >
            <el-icon name="DocumentAdd"></el-icon>
            <span class="button-text">{{ i18nTrans('pageManager.create') }}</span>
          </el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted, computed, nextTick } from 'vue'
import { ElMessage, ElButton, ElIcon, ElMessageBox } from 'element-plus'
import { usePages } from '../stores/usePages'
import FormGrid from '../components/FormGrid.vue'
import Toolbar from '../components/Toolbar.vue'
import FloatingBar from '../components/FloatingBar.vue'
import SimpleCardConverter from '../components/SimpleCardConverter.vue'
import { Edit, Delete, Lock, DocumentRemove, DocumentAdd } from '@element-plus/icons-vue'
import { debounce } from 'lodash-es'
import { savePositions, clearPositions } from '../services/formPositionService'
import { getLocale, setLocale, t as i18nTrans} from '../utils/i18n'
import { onBeforeMount } from 'vue'
// 确保所有用到的翻译键都存在
const ensureTranslationsExist = () => {
  // 确保所有需要的翻译键都已定义
  const keys = [
    'pageManager.newPage',
    'pageManager.confirmDelete',
    'pageManager.confirmDeletePage',
    'pageManager.warning',
    'pageManager.confirm',
    'pageManager.cancel',
    'pageManager.create',
    'pageManager.pageCreationFailed',
    'pageManager.selectPageSize',
    'pageManager.pageName',
    'pageManager.enterPageName',
    'pageManager.pageSize',
    'pageManager.formPage',
    'pageManager.cardPage',
    'pageManager.confirmCloseDuringCreation'
  ]
}
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

// 侧边栏相关数据
const asideWidth = ref(220) // 侧边栏宽度
const isResizing = ref(false) // 是否正在调整大小
const asideCollapsed = ref(false) // 侧边栏是否折叠
const resizeStartX = ref(0) // 调整大小起始位置
const startWidth = ref(0) // 调整大小起始宽度

onBeforeMount(() => {
  try {
    console.log('[PageManager] 页面即将挂载，当前页面数量:', pages.value.length);
    
    // 确保当前页面索引有效
    if (pages.value.length > 0) {
      currentPageIdx.value = Math.min(currentPageIdx.value, pages.value.length - 1);
    } else {
      currentPageIdx.value = 0;
    }
    
    console.log('[PageManager] 初始化当前页面索引:', currentPageIdx.value);
  } catch (error) {
    console.error('[PageManager] 初始化页面数据时出错:', error);
    ElMessage.error('初始化页面数据失败');
  }
});

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

// 监听当前页面索引变化，确保页面尺寸正确更新
watch(currentPageIdx, (newIndex, oldIndex) => {
  console.log(`[PageManager] 页面索引从 ${oldIndex} 变更为 ${newIndex}`);
  
  // 确保页面数据存在
  if (newIndex >= 0 && newIndex < pages.value.length) {
    const currentPage = pages.value[newIndex];
    console.log('[PageManager] 当前页面信息:', {
      index: newIndex,
      name: currentPage.name,
      pageSize: currentPage.pageSize,
      formsCount: currentPage.forms?.length || 0
    });
  }
  
  // 强制更新以确保组件接收到最新的页面数据
  nextTick(() => {
    // 触发响应式更新
    if (pages.value.length > 0) {
      pages.value = [...pages.value];
    }
  });
}, { immediate: true });

// 纸张尺寸选择对话框相关
const showPageSizeDialog = ref(false)
const newPageName = ref(i18nTrans('pageManager.newPage'))
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
  const maxHeight = 60
  const ratio = Math.min(maxWidth / paper.width, maxHeight / paper.height)
  
  // 计算实际显示尺寸
  const displayWidth = Math.max(40, paper.width * ratio)  // 最小宽度40px
  const displayHeight = Math.max(30, paper.height * ratio) // 最小高度30px

  return {
    width: `${displayWidth}px`,
    height: `${displayHeight}px`,
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    fontSize: '10px',
    color: '#606266',
    backgroundColor: 'white',
    transition: 'all 0.2s ease',
    margin: '5px auto',  // 居中显示并增加上下间距
    boxSizing: 'border-box',
    position: 'relative',
    borderRadius: '2px',
    border: '1px solid #ebeef5'
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

// 添加当前页面尺寸计算属性
const pageSize = computed(() => {
  if (pages.value.length > 0 && pages.value[currentPageIdx.value]) {
    return pages.value[currentPageIdx.value].pageSize || { 
      width: 210, 
      height: 297, 
      unit: 'mm' 
    };
  }
  return { 
    width: 210, 
    height: 297, 
    unit: 'mm' 
  };
});

// 处理表单更新
function handleFormUpdate(updatedForms) {
  try {
    console.log('[PageManager] 接收到表单更新:', updatedForms?.length || 0, '个表单');

    // 确保当前页面存在
    if (!pages.value[currentPageIdx.value]) {
      console.warn('[PageManager] 当前页面不存在，无法更新表单');
      return;
    }

    // 直接更新当前页面的表单数组，避免使用splice等可能引起问题的方法
    const currentPage = pages.value[currentPageIdx.value];
    
    // 创建新的页面对象以确保响应式更新
    const updatedPage = {
      ...currentPage,
      forms: Array.isArray(updatedForms) ? [...updatedForms] : []
    };
    
    // 替换整个页面对象以触发响应式更新
    const newPages = [...pages.value];
    newPages[currentPageIdx.value] = updatedPage;
    pages.value = newPages;

    // 保存到本地存储
    localStorage.setItem('form-pages', JSON.stringify(pages.value));
    const storedPages = JSON.parse(localStorage.getItem('form-pages'));
    console.log('[PageManager] 本地存储更新验证:', storedPages);
    
    console.log('[PageManager] 表单更新成功，当前页面表单数量:', 
      pages.value[currentPageIdx.value]?.forms?.length || 0);
    
    // 额外验证存储的数据
    if (storedPages && storedPages[currentPageIdx.value]) {
      console.log('[PageManager] 存储中当前页面表单数量:', 
        storedPages[currentPageIdx.value]?.forms?.length || 0);
    }
  } catch (error) {
    console.error('[PageManager] 更新表单时出错:', error);
    ElMessage.error('更新表单失败');
    errorLogService.addErrorLog(error, '更新表单失败', 'error');
  }
}

// 检查并修复表单显示问题
function checkAndFixFormDisplay() {
  try {
    console.log('[PageManager] 检查表单显示状态');
    
    // 确保当前页面存在
    if (!pages.value[currentPageIdx.value]) {
      console.warn('[PageManager] 当前页面不存在，无法检查表单');
      return;
    }
    
    // 获取当前页面表单数量
    const formCount = pages.value[currentPageIdx.value].forms?.length || 0;
    console.log(`[PageManager] 当前页面有 ${formCount} 个表单`);
    
    // 如果有表单但FormGrid未显示，尝试强制更新
    if (formCount > 0 && formGridRef.value) {
      console.log('[PageManager] 尝试强制更新表单显示');
      
      // 触发响应式更新
      nextTick(() => {
        // 强制更新页面数组引用，触发响应式
        pages.value = [...pages.value];
        
        // 确保本地存储同步
        localStorage.setItem('form-pages', JSON.stringify(pages.value));
        console.log('[PageManager] 本地存储更新验证:', JSON.parse(localStorage.getItem('form-pages')));
        
        console.log('[PageManager] 表单显示修复完成');
      });
    }
  } catch (error) {
    console.error('[PageManager] 检查表单显示时出错:', error);
  }
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
      new Error(),
      `FormGrid 组件状态: ${formGridRef.value.$el ? '已挂载' : '未挂载'}`,
      'info'
    );
  }

  if (cardConverterRef.value) {
    console.log('SimpleCardConverter 组件状态:', {
      isMounted: cardConverterRef.value.$el ? '已挂载' : '未挂载',
      isVisible: cardConverterRef.value.$el?.offsetParent !== null
    });
    
    errorLogService.addErrorLog(
      new Error(),
      `SimpleCardConverter 组件状态: ${cardConverterRef.value.$el ? '已挂载' : '未挂载'}`,
      'info'
    );
  }
  
  errorLogService.addErrorLog(
    new Error(),
    `PageManager 组件完整挂载状态: FormGrid ${formGridRef.value ? '存在' : '不存在'}, SimpleCardConverter ${cardConverterRef.value ? '存在' : '不存在'}`,
    'info'
  );
});

// 在组件挂载时注册全局事件
onMounted(() => {
  // 初始化事件总线
  const { on } = useEventBus()

  // 注册添加表单事件
  on('add-form', () => {
    onAddForm()
  })

  // 注册其他事件...
  
  // 检查并修复表单显示问题
  nextTick(() => {
    // 等待组件完全加载后检查表单显示
    setTimeout(() => {
      checkAndFixFormDisplay();
    }, 500);
  });
  
  // 添加空值检查
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
      new Error(),
      `FormGrid 组件状态: ${formGridRef.value.$el ? '已挂载' : '未挂载'}`,
      'info'
    );
  }

  if (cardConverterRef.value) {
    console.log('SimpleCardConverter 组件状态:', {
      isMounted: cardConverterRef.value.$el ? '已挂载' : '未挂载',
      isVisible: cardConverterRef.value.$el?.offsetParent !== null
    });
    
    errorLogService.addErrorLog(
      new Error(),
      `SimpleCardConverter 组件状态: ${cardConverterRef.value.$el ? '已挂载' : '未挂载'}`,
      'info'
    );
  }
  
  errorLogService.addErrorLog(
    new Error(),
    `PageManager 组件完整挂载状态: FormGrid ${formGridRef.value ? '存在' : '不存在'}, SimpleCardConverter ${cardConverterRef.value ? '存在' : '不存在'}`,
    'info'
  );
});

// 优化的 onAddForm 方法
const onAddForm = () => {
  try {
    // 检查是否有页面
    if (pages.value.length === 0) {
      ElMessage.warning(i18nTrans('pageManager.noPages', {}, '请先创建页面'))
      return
    }
    
    // 检查页面是否可编辑
    if (editPageIdx.value !== currentPageIdx.value) {
      // 如果页面不可编辑，提示用户需要先解锁页面
      ElMessage.warning(i18nTrans('pageManager.unlockPageFirst', {}, '请先解锁页面再添加表单'))
      return
    }

    nextTick(() => {
      try {
        // 确保 formGridRef 存在并且已正确加载
        if (!formGridRef.value) {
          throw new Error('表单网格组件未找到')
        }
        
        if (typeof formGridRef.value.handleAddForm !== 'function') {
          throw new Error('表单网格组件方法未找到')
        }

        // 直接调用 handleAddForm 方法
        formGridRef.value.handleAddForm()
      } catch (error) {
        console.error('添加表单错误:', error)
        ElMessage.error(i18nTrans('pageManager.cannotAddForm', {}, '无法添加表单：表单编辑器未就绪'))
        errorLogService.addErrorLog(error, '添加表单失败 - 组件引用问题', 'error')
      }
    })
  } catch (error) {
    console.error('添加表单过程错误:', error)
    ElMessage.error('添加表单失败')
    errorLogService.addErrorLog(error, '添加表单失败 - 意外错误', 'error')
  }
}

// 创建新页面
const isCreatingPage = ref(false)
const selectedPaper = ref(null)

// 处理对话框关闭
const handleDialogClose = (done) => {
  if (isCreatingPage.value) {
    ElMessageBox.confirm(
      i18nTrans('pageManager.confirmCloseDuringCreation'),
      i18nTrans('pageManager.warning'),
      {
        confirmButtonText: i18nTrans('pageManager.confirm'),
        cancelButtonText: i18nTrans('pageManager.cancel'),
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

// 选择纸张并触发点击事件
const selectPaperAndClick = (paper) => {
  selectedPageSize.value = paper.name
  selectedPaper.value = paper
  
  console.log('[PageManager] 纸张选择并点击:', {
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

  try {
    // 创建页面
    const newPageNameValue = newPageName.value || i18nTrans('pageManager.newPage')
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
    
    // 保存当前页面数量作为新页面索引
    const newPageIdx = pages.value.length;
    
    // 修改addPage调用方式，确保页面数据结构完整
    const newPage = {
      id: Date.now() + Math.random(),
      name: newPageNameValue,
      forms: [], // 确保初始化为空数组
      pageSize: {
        name: pageSizeObj.name,
        width: pageSizeObj.width,
        height: pageSizeObj.height,
        unit: pageSizeObj.unit
      },
      orientation: 'portrait'
    }
    
    // 使用 usePages 提供的 addPage 方法
    addPage(newPage);
    
    // 更新当前页面索引
    currentPageIdx.value = newPageIdx;
    
    console.log('[PageManager] 当前页面索引更新为:', currentPageIdx.value, '页面总数:', pages.value.length)
    
    // 添加表单数据验证日志
    nextTick(() => {
      console.log('[PageManager] 新建页面表单数据:', 
        JSON.parse(JSON.stringify(pages.value[currentPageIdx.value]?.forms || [])))
    })
    
    // 关闭对话框
    showPageSizeDialog.value = false
    // 重置输入
    newPageName.value = i18nTrans('pageManager.newPage')

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
      pageId: pages.value[newPageIdx].id,
      formsCount: pages.value[newPageIdx].forms?.length || 0
    });
    
    // 页面创建成功提示
    ElMessage.success(i18nTrans('pageManager.pageCreated', {}, '页面创建成功'));
    
  } catch (error) {
    console.error('[PageManager] 创建页面异常:', error);
    ElMessage.error(i18nTrans('pageManager.pageCreationFailed'));
    
    errorLogService.addErrorLog(
      error,
      '创建页面失败 - 异常错误',
      'error'
    );
  } finally {
    isCreatingPage.value = false;
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
    i18nTrans('pageManager.confirmDeletePage'),
    i18nTrans('pageManager.warning'),
    {
      confirmButtonText: i18nTrans('pageManager.confirm'),
      cancelButtonText: i18nTrans('pageManager.cancel'),
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
      
      // 删除前获取页面名称
      const pageName = pages.value[idx]?.name || i18nTrans('pageManager.newPage');
      const successMessage = `页面"${pageName}"删除成功`;
      console.log('[PageManager] 页面删除成功:', {
        pageIndex: idx,
        pageName
      });
      
      ElMessage.success(successMessage);
      
      errorLogService.addErrorLog(
        new Error(successMessage),
        `页面删除成功: ${pageName} (${idx})`,
        'info'
      );
    } catch (error) {
      console.error('[PageManager] 删除页面时发生错误:', error);
      const errorMessage = '页面删除失败';
      ElMessage.error(errorMessage);
      
      errorLogService.addErrorLog(
        error,
        `页面删除失败: ${idx}`,
        'error'
      );
    }
  }).catch(() => {
    console.log('[PageManager] 页面删除操作取消:', idx);
    const cancelMessage = '已取消删除';
    ElMessage.info(cancelMessage);
    
    errorLogService.addErrorLog(
      new Error(cancelMessage),
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
  const isCurrentPage = idx === currentPageIdx.value
  
  try {
    // 交换页面尺寸的宽高
    rotatePageOrientation(idx)
    
    // 如果不是当前页面，只进行数据更新不做视觉效果
    if (!isCurrentPage) {
      ElMessage.success(`页面"${page.name || i18nTrans('pageManager.newPage')}"已旋转`)
      return
    }
    
    // 获取新的方向状态
    const isLandscape = page.orientation === 'landscape'
    
    // 更新页面渲染 - 使用响应式方式处理旋转
    nextTick(() => {
      // 根据页面类型更新视图
      if (currentPageType.value === 'form' && formGridRef.value) {
        // 使用类方式控制旋转样式，而不是直接操作DOM
        const formGridEl = formGridRef.value.$el
        if (formGridEl) {
          // 为元素添加/移除旋转类
          if (isLandscape) {
            formGridEl.classList.add('landscape-mode')
          } else {
            formGridEl.classList.remove('landscape-mode')
          }
        }
      } else if (currentPageType.value === 'cards' && cardConverterRef.value) {
        // 卡片转换器处理
        cardConverterRef.value.setPageSize({
          ...page.pageSize,
          orientation: page.orientation
        })
        
        // 同样使用类控制而非直接操作样式
        const cardsContainerEl = cardConverterRef.value.$el?.querySelector('.cards-container')
        if (cardsContainerEl) {
          if (isLandscape) {
            cardsContainerEl.classList.add('landscape-mode')
          } else {
            cardsContainerEl.classList.remove('landscape-mode')
          }
        }
      }
      
      // 显示旋转提示
      ElMessage.success(`页面"${page.name || i18nTrans('pageManager.newPage')}"已旋转为${isLandscape ? '横向' : '纵向'}模式`)
    })
  } catch (error) {
    // 错误处理
    console.error('旋转页面时出错:', error)
    ElMessage.error('旋转页面失败')
    errorLogService.addErrorLog(error, `旋转页面失败: ${idx}`, 'error')
  }
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
  console.log('[PageManager] 页面类型变更:', newType);
  currentPageType.value = newType
  
  // 如果切换到卡片页面，自动转换当前页面的表单
  if (newType === 'cards') {
    nextTick(() => {
      convertFormsToCardsAuto();
    });
  }
}

// 自动转换表单为卡片（无用户提示）
function convertFormsToCardsAuto() {
  try {
    const currentPage = pages.value[currentPageIdx.value];
    if (!currentPage) {
      console.log('[PageManager] 当前页面不存在，无法自动转换');
      return;
    }
    
    if (!currentPage.forms || !Array.isArray(currentPage.forms)) {
      console.log('[PageManager] 当前页面表单数据格式不正确，无法自动转换');
      return;
    }

    if (currentPage.forms.length === 0) {
      console.log('[PageManager] 当前页面没有表单内容，无需转换');
      return;
    }

    // 添加有效性过滤
    const validForms = currentPage.forms.filter(form =>
      (form.title && form.title.trim()) || 
      (form.value && form.value.trim()) || 
      (form.remark && form.remark.trim()) ||
      (form.media && form.media.trim())
    );

    if (validForms.length === 0) {
      console.log('[PageManager] 当前页面表单内容均为空，无需转换');
      return;
    }

    if (cardConverterRef.value) {
      try {
        console.log('[PageManager] 自动转换表单为卡片，表单数量:', validForms.length);
        // 添加错误处理
        if (typeof cardConverterRef.value.convertFormsToCards === 'function') {
          cardConverterRef.value.convertFormsToCards(currentPage.forms);
          console.log('[PageManager] 自动转换完成');
        } else {
          console.error('[PageManager] 卡片转换器缺少convertFormsToCards方法');
          ElMessage.error('卡片转换功能不可用');
        }
      } catch (error) {
        console.error('[PageManager] 调用卡片转换器时出错:', error);
        ElMessage.error('卡片转换失败: ' + (error.message || '未知错误'));
      }
    } else {
      console.warn('[PageManager] 卡片转换器引用不存在');
    }
  } catch (error) {
    console.error('[PageManager] 自动转换表单为卡片时出错:', error);
    ElMessage.error('自动转换失败: ' + (error.message || '未知错误'));
  }
}

// 转换表单为卡片
function convertFormsToCards() {
  try {
    const currentPage = pages.value[currentPageIdx.value]
    if (!currentPage) {
      ElMessage.warning('当前页面不存在')
      return
    }
    
    if (!currentPage.forms || !Array.isArray(currentPage.forms)) {
      ElMessage.warning('当前页面表单数据格式不正确')
      return
    }

    if (currentPage.forms.length === 0) {
      ElMessage.warning('当前页面没有表单内容')
      return
    }

    // 添加有效性过滤
    const validForms = currentPage.forms.filter(form =>
      (form.title && form.title.trim()) || 
      (form.value && form.value.trim()) || 
      (form.remark && form.remark.trim()) ||
      (form.media && form.media.trim())
    )

    if (validForms.length === 0) {
      ElMessage.warning('表单内容均为空，无法转换')
      return
    }

    nextTick(() => {
      if (cardConverterRef.value) {
        try {
          cardConverterRef.value.convertFormsToCards(currentPage.forms)
          ElMessage.success(`成功将 ${validForms.length} 个表单转换为卡片`)
        } catch (error) {
          console.error('[PageManager] 卡片转换过程中出错:', error)
          ElMessage.error('卡片转换失败: ' + (error.message || '未知错误'))
        }
      } else {
        ElMessage.error('卡片转换组件未加载')
        console.error('cardConverterRef 未定义')
      }
    })
  } catch (error) {
    console.error('[PageManager] 调用卡片转换功能时出错:', error)
    ElMessage.error('转换过程出错: ' + (error.message || '未知错误'))
  }
}

// 调试按钮点击
const debugButton = (buttonName) => {
  console.log(`[PageManager] 按钮点击调试 - ${buttonName} 按钮被点击`);
  console.log(`[PageManager] i18n 翻译 - cancel: ${i18nTrans('pageManager.cancel')}, create: ${i18nTrans('pageManager.create')}`);
  console.log(`[PageManager] 按钮状态 - isCreatingPage: ${isCreatingPage.value}, selectedPageSize: ${selectedPageSize.value}`);

  if (buttonName === 'cancel') {
    showPageSizeDialog.value = false;
  } else {
    createNewPage();
  }
};

// 切换侧边栏显示/隐藏
const toggleAside = () => {
  asideCollapsed.value = !asideCollapsed.value
  // 如果折叠，宽度设为较小值；否则恢复原来的宽度
  asideWidth.value = asideCollapsed.value ? 40 : 220
}

// 开始调整侧边栏大小
const startResize = (e) => {
  isResizing.value = true
  resizeStartX.value = e.clientX
  startWidth.value = asideWidth.value
  
  // 添加事件监听器
  document.addEventListener('mousemove', resize)
  document.addEventListener('mouseup', stopResize)
  
  e.preventDefault()
}

// 调整侧边栏大小
const resize = (e) => {
  if (!isResizing.value) return
  
  const deltaX = e.clientX - resizeStartX.value
  const newWidth = startWidth.value - deltaX
  
  // 限制最小和最大宽度
  asideWidth.value = Math.max(100, Math.min(500, newWidth))
}

// 停止调整侧边栏大小
const stopResize = () => {
  isResizing.value = false
  document.removeEventListener('mousemove', resize)
  document.removeEventListener('mouseup', stopResize)
}

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

/* 可拖动侧边栏样式 */
.resizable-aside-container {
  position: relative;
  height: 100%;
  transition: width 0.3s ease;
}

.resizable-aside {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.resize-handle {
  position: absolute;
  top: 0;
  left: 0;
  width: 5px;
  height: 100%;
  background-color: #dcdfe6;
  cursor: col-resize;
  z-index: 100;
  transition: background-color 0.3s;
}

.resize-handle:hover,
.resize-handle.active {
  background-color: #42b883;
}

.aside-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 15px;
  background-color: #f5f7fa;
  border-bottom: 1px solid #e4e7ed;
}

.aside-header h3 {
  margin: 0;
  font-size: 16px;
  color: #606266;
}

.aside-content {
  flex: 1;
  overflow-y: auto;
}

.toggle-aside-btn {
  padding: 5px;
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
  gap: 15px;
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
  font-size: 11px;
  color: #606266;
  background-color: white;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  transition: all 0.3s ease;
  border-radius: 4px;
  padding: 10px 6px;
  text-align: center;
  cursor: pointer;
  height: 100px;
  position: relative;
  z-index: 1;
  overflow: hidden;
}

.paper-preview-compact:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
  transform: translateY(-2px);
  z-index: 2;
}

.paper-preview-compact.selected {
  border-color: #42b883;
  box-shadow: 0 2px 10px rgba(66, 184, 131, 0.25);
  z-index: 3;
}

.paper-preview-compact::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  border: 2px solid transparent;
  border-radius: 4px;
  pointer-events: none;
  transition: border-color 0.3s ease;
}

.paper-preview-compact.selected::before {
  border-color: #42b883;
}

.paper-name {
  font-weight: 500;
  margin-bottom: 6px;
  color: #303133;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 90%;
}

.paper-dimensions {
  font-size: 10px;
  color: #909399;
  margin-top: 4px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 90%;
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

/* 页面旋转相关样式 */
.landscape-mode {
  transform: rotate(90deg);
  transform-origin: center center;
  transition: transform 0.3s ease;
  margin: 80px auto !important;
  max-width: calc(100% - 160px) !important; /* 确保旋转后不超出视图 */
}

.landscape-mode .form-card-mini,
.landscape-mode .chinese-char-card,
.landscape-mode .card-item {
  transition: all 0.3s ease;
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
    padding: 6px 4px;
  }
  
  /* 移动设备上的旋转处理 */
  .landscape-mode {
    transform: rotate(90deg) scale(0.85);
    margin: 60px auto !important;
  }
}
</style>