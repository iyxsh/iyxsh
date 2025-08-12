<template>
  <div class="page-manager">
    <!-- 显示从FileManager传入的文件信息 -->
    <div v-if="route.query?.fileName" class="file-info-banner">
      <el-alert 
        :title="`正在编辑文件: ${route.query.fileName}`" 
        type="info" 
        show-icon
        closable
        @close="handleFileInfoClose"
      />
    </div>

    <!-- 导航区域 -->
    <div class="navigation">
      <router-link to="/files" class="home-link">返回文件列表</router-link>
    </div>

    <!-- 页面管理功能 -->
    <el-container class="main-container">
      <Toolbar 
        :clearAllPages="clearAllPages" 
        :rotatePage="rotateCurrentPage"
        :showRotateButton="pages.length > 0" 
        :pages="pages"
        :currentPage="currentPageIdx"
        :cardStyleOn="cardStyleOn"
        :editable="editPageIdx === currentPageIdx"
        :newPageName="newPageName"
        @toggle-edit-mode="toggleEditMode"
        @export-data="exportData"
        @import-data="importData"
        @save-all="saveAll"
        @on-add-page="onAddPage"
        @remove-page="handleRemovePage"
        @rename-page="handleRenamePage"
        @toggle-edit-page="toggleEditPage"
        @change-page="handleToolbarChangePage"
        />
      <el-main>
        <!-- 表单页面 -->
        <template v-if="currentPageType === 'form'">
          <FormGrid v-if="pages[currentPageIdx]" v-model="pages[currentPageIdx].forms" 
            :editable="editPageIdx === currentPageIdx" :cardStyleOn="cardStyleOn"
            :pageSize="pages[currentPageIdx]?.pageSize" @update:modelValue="handleFormUpdate" 
            @containerResize="handleContainerResize" ref="formGridRef" 
            :key="`formgrid-${currentPageIdx}`" />
        </template>

        <!-- 卡片转换页面 -->
        <template v-else-if="currentPageType === 'cards'">
          <SimpleCardConverter :forms="pages[currentPageIdx] ? pages[currentPageIdx].forms : []"
            :pageSize="pages[currentPageIdx]?.pageSize" ref="cardConverterRef"
            :key="`${currentPageType}-${currentPageIdx}`" @edit-card="handleEditCard"
            @edit-row-style="handleEditRowStyle" />
        </template>
        
        <!-- 浮动工具条 -->
        <FloatingBar 
          :editable="editPageIdx === currentPageIdx"
          :pages="pages"
          :current-page-idx="currentPageIdx"
          :current-page-type="currentPageType"
          @add-form="addFormToCurrentPage"
          @toggle-card-style="onToggleCardStyle"
          @prev-page="prevPage"
          @next-page="nextPage"
          @change-page-type="handlePageTypeChange"
        />
      </el-main>
    </el-container>

    <!-- 纸张尺寸选择对话框 -->
    <el-dialog v-model="showPageSizeDialog" :title="i18nTrans('pageManager.selectPageSize')" width="650px"
      :before-close="handleDialogClose">
      <div class="page-size-selector">
        <el-form label-position="top">
          <el-row :gutter="20">
            <el-col :span="8">
              <el-form-item :label="i18nTrans('pageManager.pageName')">
                <el-input v-model="newPageName" :placeholder="i18nTrans('pageManager.enterPageName')"
                  autofocus></el-input>
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
                  <el-radio :value="paper.name" class="paper-radio" v-model="selectedPageSize"
                    @change="selectPaper(paper)">
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
          <el-button @click="debugButton('cancel')" class="toolbar-button cancel-button">
            <el-icon name="DocumentRemove"></el-icon>
            <span class="button-text">{{ i18nTrans('pageManager.cancel') }}</span>
          </el-button>
          <el-button type="primary" @click="debugButton('create')" :loading="isCreatingPage"
            :disabled="!selectedPageSize" class="toolbar-button create-button">
            <el-icon name="DocumentAdd"></el-icon>
            <span class="button-text">{{ i18nTrans('pageManager.create') }}</span>
          </el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted, computed, nextTick, onUnmounted, getCurrentInstance } from 'vue'
import { ElMessage, ElButton, ElIcon, ElMessageBox } from 'element-plus'
import { usePages } from '../stores/usePages'
import FormGrid from '../components/FormGrid.vue'
import Toolbar from '../components/Toolbar.vue'
import FloatingBar from '../components/FloatingBar.vue'
import SimpleCardConverter from '../components/SimpleCardConverter.vue'
import { debounce } from 'lodash-es'
import { savePositions, clearPositions } from '../services/formPositionService'
import { getLocale, setLocale, t as i18nTrans } from '../utils/i18n'
import { onBeforeMount, onBeforeUnmount } from 'vue'
import { useRoute, useRouter } from 'vue-router'
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
const router = useRouter()

// 获取路由信息
const route = useRoute()

// 默认页面尺寸
const defaultPageSize = {
  width: 210,
  height: 297,
  unit: 'mm'
}

// 初始化页面数据
const initializePages = () => {
  // 检查路由中是否有文件数据
  if (route.query?.fileData) {
    try {
      const fileData = JSON.parse(route.query.fileData)
      // 使用传入的文件数据初始化页面
      pages.value = Array.isArray(fileData) ? fileData : [fileData]
      currentPageIdx.value = Math.min(currentPageIdx.value, pages.value.length - 1)

      console.log('[PageManager] 从文件管理器加载数据:', fileData)
      
      // 检查是否有文件名
      if (route.query.fileName) {
        console.log('[PageManager] 加载文件:', route.query.fileName)
      }
    } catch (e) {
      console.error('[PageManager] 解析文件数据失败:', e)
      // 如果解析失败，使用默认初始化
      if (pages.value.length === 0) {
        pages.value = [{ forms: [], pageSize: defaultPageSize }]
        currentPageIdx.value = 0
      }
    }
  } else {
    // 默认初始化
    if (pages.value.length === 0) {
      pages.value = [{ forms: [], pageSize: defaultPageSize }]
      currentPageIdx.value = 0
    }
  }
}

// 添加 getApiService 方法
const getApiService = () => {
  const instance = getCurrentInstance();
  return instance?.appContext.config.globalProperties.$apiService;
};

onBeforeMount(async () => {
  try {
    console.log('[PageManager] 页面即将挂载，当前页面数量:', pages.value.length);

    // 确保当前页面索引有效
    if (pages.value.length > 0) {
      currentPageIdx.value = Math.min(currentPageIdx.value, pages.value.length - 1);
    } else {
      currentPageIdx.value = 0;
    }

    // 检查路由中是否有文件名参数
    if (route.query?.fileName) {
      console.log('[PageManager] 加载文件:', route.query.fileName);

      // 获取 ApiServiceManager 实例
      const apiService = getApiService();
      if (!apiService) {
        console.warn('[PageManager] ApiServiceManager 实例不可用');
        ElMessage.warning('API服务不可用，请稍后重试');
        return;
      }

      // 调用 getSheetData 获取工作表数据
      const sheetDataRequest = {
        filename: route.query.fileName,
        sheetname: 'Sheet1' // 默认工作表名称，可根据需要调整
      };

      try {
        const response = await apiService.getSheetData(sheetDataRequest);

        // 将返回的工作表数据转换为多条表单数据
        const formDataArray = convertSheetDataToForms(response.data);

        // 清理并写入表单缓存
        updateFormCache(formDataArray);

        console.log('[PageManager] 工作表数据已成功加载并缓存');
      } catch (error) {
        console.error('[PageManager] 获取工作表数据失败:', error);
        ElMessage.error('加载工作表数据失败');
      }
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

    // 检查表单是否真的发生了变化
    const currentPage = pages.value[currentPageIdx.value];
    const existingForms = currentPage.forms || [];

    // 使用JSON.stringify进行深度比较
    if (JSON.stringify(existingForms) === JSON.stringify(updatedForms)) {
      console.log('[PageManager] 表单未发生变化，跳过更新');
      return;
    }

    // 创建更新后的页面对象
    const updatedPage = {
      ...currentPage,
      forms: Array.isArray(updatedForms) ? [...updatedForms] : []
    };

    // 创建新的pages数组以确保响应式更新
    const newPages = [...pages.value];
    newPages[currentPageIdx.value] = updatedPage;
    
    // 使用Vue的响应式系统进行更新
    pages.value = newPages;

    // 保存到本地存储
    localStorage.setItem('form-pages', JSON.stringify(pages.value));
    
    // 验证存储的数据
    const storedPages = JSON.parse(localStorage.getItem('form-pages'));
    console.log('[PageManager] 本地存储更新验证:', storedPages);

    console.log('[PageManager] 表单更新成功，当前页面表单数量:',
      pages.value[currentPageIdx.value]?.forms?.length || 0);

    // 额外验证存储的数据
    if (storedPages && storedPages[currentPageIdx.value]) {
      console.log('[PageManager] 存储中当前页面表单数量:',
        storedPages[currentPageIdx.value]?.forms?.length || 0);
    }
    
    // 触发nextTick确保DOM更新
    nextTick(() => {
      console.log('[PageManager] 页面数据更新完成，当前表单数量:', 
        pages.value[currentPageIdx.value]?.forms?.length || 0);
    });
  } catch (error) {
    console.error('[PageManager] 更新表单时出错:', error);
    ElMessage.error('更新表单失败');
    errorLogService.addErrorLog(error, '更新表单失败', 'error');
  }
}

// 页面导航方法
function prevPage() {
  if (currentPageIdx.value > 0) {
    currentPageIdx.value--
  } else {
    ElMessage.warning(i18nTrans('pageManager.firstPage', {}, '已经是第一页'))
  }
}

function nextPage() {
  if (currentPageIdx.value < pages.value.length - 1) {
    currentPageIdx.value++
  } else {
    ElMessage.warning(i18nTrans('pageManager.lastPage', {}, '已经是最后一页'))
  }
}

// 新增方法：添加表单到当前页面
function addFormToCurrentPage() {
  console.log('[PageManager] 添加表单到当前页面');

  // 确保当前页面存在
  if (!pages.value[currentPageIdx.value]) {
    console.warn('[PageManager] 当前页面不存在，无法添加表单');
    return;
  }

  // 获取当前页面
  const currentPage = pages.value[currentPageIdx.value];

  // 确保表单数组存在
  if (!currentPage.forms) {
    currentPage.forms = [];
  }

  // 添加新表单
  const newForm = {
    id: Date.now() + Math.random(),
    type: 'text',
    title: '',
    value: '',
    remark: '',
    fontSize: 14,
    fontColor: '#000000',
    backgroundColor: '#ffffff',
    borderColor: '#000000'
  };

  currentPage.forms.push(newForm);

  console.log('[PageManager] 已添加新表单，当前页面表单数量:', currentPage.forms.length);
}

// 新增方法：添加页面
function onAddPage() {
  console.log('[PageManager] 接收到添加页面请求');
  showPageSizeSelector();
}

// 新增方法：显示页面尺寸选择器
function showPageSizeSelector() {
  // 重置新建页面名称
  newPageName.value = i18nTrans('pageManager.newPage')
  
  // 显示对话框
  showPageSizeDialog.value = true
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

// 在组件挂载时注册全局事件
onMounted(() => {
  // 初始化事件总线
  const { on } = useEventBus()

  // 注册添加表单事件
  on('add-form', () => {
    onAddForm()
  })
  
  // 添加页面相关事件监听
  on('add-page', () => {
    onAddPage()
  })
  
  // 添加删除页面事件监听
  on('delete-page', (idx) => {
    deletePage(idx)
  })
  
  // 添加切换页面事件监听
  on('change-page', (idx) => {
    currentPageIdx.value = idx
  })

  // 注册其他事件...
  
  // 检查并修复表单显示问题
  nextTick(() => {
    // 等待组件完全加载后检查表单显示
    setTimeout(() => {
      checkAndFixFormDisplay();
    }, 500);
  });
  
  // 添加窗口大小变化监听器，用于更新el-main尺寸
  window.addEventListener('resize', handleWindowResize);
  
  // 添加mainResize事件监听器
  window.addEventListener('mainResize', handleMainResize);
  
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

// 组件卸载时移除事件监听器
onUnmounted(() => {
  window.removeEventListener('resize', handleWindowResize);
  window.removeEventListener('mainResize', handleMainResize);
});

// 处理窗口大小变化事件
const handleWindowResize = debounce(() => {
  console.log('[PageManager] 窗口大小发生变化');
  
  // 更新当前页面的pageSize以匹配el-main的新尺寸
  updateCurrentPagePageSize();
}, 300);

// 处理mainResize事件
const handleMainResize = debounce((event) => {
  console.log('[PageManager] 接收到mainResize事件');
  
  // 更新当前页面的pageSize以匹配el-main的新尺寸
  updateCurrentPagePageSize();
}, 300);

// 处理FormGrid容器尺寸变化事件
const handleContainerResize = debounce((size) => {
  console.log('[PageManager] FormGrid容器尺寸发生变化:', size);
  
  // 更新当前页面的pageSize以匹配容器的新尺寸
  updateCurrentPagePageSize();
}, 300);

// 更新当前页面的pageSize
const updateCurrentPagePageSize = () => {
  try {
    // 确保有页面存在
    if (pages.value.length === 0 || currentPageIdx.value < 0 || currentPageIdx.value >= pages.value.length) {
      return;
    }
    
    // 获取el-main元素的尺寸
    let mainWidth = 210; // 默认宽度 210mm
    let mainHeight = 297; // 默认高度 297mm
    
    // 尝试获取实际的el-main尺寸
    const mainElement = document.querySelector('.main-container .el-main');
    if (mainElement) {
      // 获取元素的像素尺寸
      const rect = mainElement.getBoundingClientRect();
      const widthPx = rect.width;
      const heightPx = rect.height;
      
      // 将像素转换为毫米 (假设 1mm = 3.779527559055px)
      mainWidth = Math.round(widthPx / 3.779527559055);
      mainHeight = Math.round(heightPx / 3.779527559055);
      
      console.log('[PageManager] el-main新尺寸 (px):', widthPx, 'x', heightPx);
      console.log('[PageManager] 转换为毫米:', mainWidth, 'x', mainHeight);
    }
    
    // 更新当前页面的pageSize
    const currentPage = pages.value[currentPageIdx.value];
    if (currentPage && currentPage.pageSize) {
      // 检查尺寸是否发生变化
      if (currentPage.pageSize.width !== mainWidth || currentPage.pageSize.height !== mainHeight) {
        console.log('[PageManager] 页面尺寸发生变化，更新pageSize');
        
        // 创建更新后的页面对象
        const updatedPage = {
          ...currentPage,
          pageSize: {
            name: '设备页',
            width: mainWidth,
            height: mainHeight,
            unit: 'mm'
          }
        };
        
        // 更新页面
        const newPages = [...pages.value];
        newPages[currentPageIdx.value] = updatedPage;
        pages.value = newPages;
        
        // 保存到本地存储
        localStorage.setItem('form-pages', JSON.stringify(pages.value));
        
        console.log('[PageManager] 页面尺寸更新完成');
      }
    }
  } catch (error) {
    console.error('[PageManager] 更新页面尺寸时出错:', error);
    errorLogService.addErrorLog(error, '更新页面尺寸失败', 'error');
  }
};

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
const createNewPage = async () => {
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
    let pageSizeObj = paperSizes.find(p => p.name === selectedPageSize.value)

    // 如果未选择纸张尺寸，使用el-main的尺寸作为默认值
    if (!pageSizeObj) {
      console.warn('[PageManager] 未选择纸张尺寸，使用el-main尺寸作为默认值');

      // 获取el-main元素的尺寸
      let mainWidth = 210; // 默认宽度 210mm
      let mainHeight = 297; // 默认高度 297mm

      // 尝试获取实际的el-main尺寸
      const mainElement = document.querySelector('.main-container .el-main');
      if (mainElement) {
        // 获取元素的像素尺寸
        const rect = mainElement.getBoundingClientRect();
        const widthPx = rect.width;
        const heightPx = rect.height;

        // 将像素转换为毫米 (假设 1mm = 3.779527559055px)
        mainWidth = Math.round(widthPx / 3.779527559055);
        mainHeight = Math.round(heightPx / 3.779527559055);

        console.log('[PageManager] el-main尺寸 (px):', widthPx, 'x', heightPx);
        console.log('[PageManager] 转换为毫米:', mainWidth, 'x', mainHeight);
        // 创建基于el-main尺寸的自定义纸张尺寸对象
        pageSizeObj = {
          name: '设备页',
          width: widthPx,
          height: heightPx,
          unit: 'px'
        };
      }
      else {
        // 创建基于el-main尺寸的自定义纸张尺寸对象
        pageSizeObj = {
          name: '设备页',
          width: mainWidth,
          height: mainHeight,
          unit: 'mm'
        };
      }
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
    console.log('[PageManager] 页面创建完成，当前页面索引:', currentPageIdx.value);

    // 重置表单
    resetPageForm();

    // 显示成功消息
    ElMessage.success(i18nTrans('pageManager.pageCreated'));

    // 关闭对话框
    showPageSizeDialog.value = false;
  } catch (error) {
    console.error('[PageManager] 创建页面失败:', error);
    ElMessage.error('创建页面失败: ' + (error.message || '未知错误'));

    // 记录错误日志
    errorLogService.addErrorLog(
      error,
      '创建页面失败',
      'error'
    );
  } finally {
    isCreatingPage.value = false;
  }
}

// 重置页面表单
const resetPageForm = () => {
  newPageName.value = i18nTrans('pageManager.newPage')
  selectedPageSize.value = 'A4'
  selectedPaper.value = null
}

// 处理Toolbar的页面切换事件
function handleToolbarChangePage(pageIndex) {
  console.log('[PageManager] 接收到Toolbar的页面切换事件:', pageIndex);
  
  // 添加参数验证
  if (typeof pageIndex !== 'number' || pageIndex < 0 || pageIndex >= pages.value.length) {
    console.warn('[PageManager] 接收到无效的页面索引:', pageIndex);
    return;
  }
  
  currentPageIdx.value = pageIndex;
}

// 处理页面删除事件
function handleRemovePage(pageIndex) {
  console.log('[PageManager] 处理页面删除事件:', pageIndex);
  
  // 添加参数验证
  if (typeof pageIndex !== 'number' || pageIndex < 0 || pageIndex >= pages.value.length) {
    console.warn('[PageManager] 接收到无效的页面索引:', pageIndex);
    ElMessage.error('无效的页面索引');
    return;
  }
  
  // 检查是否只剩一个页面
  if (pages.value.length <= 1) {
    ElMessage.warning('至少需要保留一个页面');
    return;
  }
  
  // 获取要删除的页面名称
  const pageName = pages.value[pageIndex].name || `页面 ${pageIndex + 1}`;
  
  // 确认删除
  ElMessageBox.confirm(
    `确定要删除页面"${pageName}"吗？此操作无法撤销。`,
    '删除确认',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    }
  ).then(() => {
    // 执行删除操作
    removePage(pageIndex);
    
    // 如果删除的是当前页面或之前的页面，需要调整当前页面索引
    if (pageIndex < currentPageIdx.value) {
      // 删除了当前页面之前的页面，当前页面索引需要减1
      currentPageIdx.value = Math.max(0, currentPageIdx.value - 1);
    } else if (pageIndex === currentPageIdx.value) {
      // 删除了当前页面，切换到前一个页面（如果存在）或后一个页面
      if (currentPageIdx.value > 0) {
        currentPageIdx.value = currentPageIdx.value - 1;
      } else if (pages.value.length > 0) {
        // 当前页面是第一页，切换到新的第一页
        currentPageIdx.value = 0;
      } else {
        // 没有页面了
        currentPageIdx.value = -1;
      }
    }
    // 如果删除的是后面的页面，则不需要调整当前页面索引
    
    // 显示删除成功提示
    ElMessage.success(`页面"${pageName}"已删除`);
    
    console.log('[PageManager] 页面删除成功，当前页面索引:', currentPageIdx.value, '剩余页面数:', pages.value.length);
  }).catch(() => {
    // 用户取消删除
    console.log('[PageManager] 用户取消删除页面');
  });
}

// 处理页面重命名事件
function handleRenamePage(pageIndex, newName) {
  console.log('[PageManager] 处理页面重命名事件:', pageIndex, newName);
  
  // 添加参数验证
  if (typeof pageIndex !== 'number' || pageIndex < 0 || pageIndex >= pages.value.length) {
    console.warn('[PageManager] 接收到无效的页面索引:', pageIndex);
    return;
  }
  
  if (!newName || typeof newName !== 'string') {
    console.warn('[PageManager] 接收到无效的页面名称:', newName);
    return;
  }
  
  // 更新页面名称
  pages.value[pageIndex].name = newName;
  
  console.log('[PageManager] 页面名称已更新:', pageIndex, newName);
}

// 切换页面编辑状态
function toggleEditPage(idx) {
  editPageIdx.value = editPageIdx.value === idx ? -1 : idx
}

// 创建页面并使用默认尺寸
async function createNewPageWithDefaults() {
  console.log('[PageManager] 开始创建新页面，使用默认尺寸');

  isCreatingPage.value = true;

  try {
    // 创建页面
    const newPageNameValue = newPageName.value || i18nTrans('pageManager.newPage');
    console.log('[PageManager] 创建页面，名称:', newPageNameValue, '尺寸:', '设备页');

    // 获取el-main元素的尺寸作为默认值
    let mainWidth = 210; // 默认宽度 210mm
    let mainHeight = 297; // 默认高度 297mm

    let pageSizeObj;
    // 尝试获取实际的el-main尺寸
    const mainElement = document.querySelector('.main-container .el-main');
    if (mainElement) {
      // 获取元素的像素尺寸
      const rect = mainElement.getBoundingClientRect();
      const widthPx = rect.width;
      const heightPx = rect.height;

      // 将像素转换为毫米 (假设 1mm = 3.779527559055px)
      mainWidth = Math.round(widthPx / 3.779527559055);
      mainHeight = Math.round(heightPx / 3.779527559055);

      console.log('[PageManager] el-main尺寸 (px):', widthPx, 'x', heightPx);
      console.log('[PageManager] 转换为毫米:', mainWidth, 'x', mainHeight);
      // 创建基于el-main尺寸的自定义纸张尺寸对象
        pageSizeObj = {
        name: '设备页',
        width: widthPx,
        height: heightPx,
        unit: 'px'
      };
    }
    else {
      // 创建基于el-main尺寸的自定义纸张尺寸对象
      pageSizeObj = {
        name: '设备页',
        width: mainWidth,
        height: mainHeight,
        unit: 'mm'
      };
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

// 处理编辑卡片事件
const handleEditCard = (editData) => {
  console.log('[PageManager] 接收到编辑卡片事件:', editData);
  if (cardConverterRef.value) {
    // 调用FloatingToolbar组件中的editCard方法
    if (cardConverterRef.value.$refs && cardConverterRef.value.$refs.floatingToolbarRef) {
      cardConverterRef.value.$refs.floatingToolbarRef.editCard(
        editData.groupIndex,
        editData.rowIndex,
        editData.cardIndex
      );
    }
  }
};

// 处理编辑行样式事件
const handleEditRowStyle = (editData) => {
  console.log('[PageManager] 接收到编辑行样式事件:', editData);
  if (cardConverterRef.value) {
    // 调用FloatingToolbar组件中的editRowStyle方法
    if (cardConverterRef.value.$refs && cardConverterRef.value.$refs.floatingToolbarRef) {
      cardConverterRef.value.$refs.floatingToolbarRef.editRowStyle(
        editData.groupIndex,
        editData.rowIndex
      );
    }
  }
};

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

// 处理文件信息关闭事件
const handleFileInfoClose = () => {
  // 清除路由中的fileName参数
  const query = { ...route.query };
  delete query.fileName;
  delete query.fileData;
  
  // 这里我们不进行路由跳转，只是清除显示
  // 因为实际应用中用户可能希望继续编辑
  console.log('[PageManager] 文件信息横幅已关闭');
};

// 添加辅助方法：将工作表数据转换为表单格式
function convertSheetDataToForms(sheetData) {
  return sheetData.map(row => ({
    id: Date.now() + Math.random(),
    type: 'text',
    title: row.title || '',
    value: row.value || '',
    remark: row.remark || '',
    fontSize: 14,
    fontColor: '#000000',
    backgroundColor: '#ffffff',
    borderColor: '#000000'
  }));
}

// 添加辅助方法：更新表单缓存
function updateFormCache(formDataArray) {
  // 假设使用 store 来管理表单缓存
  const currentPage = pages.value[currentPageIdx.value];
  if (currentPage) {
    currentPage.forms = [...formDataArray];
    pages.value = [...pages.value]; // 触发响应式更新

    // 保存到本地存储或其它缓存机制
    localStorage.setItem('form-cache', JSON.stringify(pages.value));
  }
}

</script>

<style scoped>
.page-manager {
  padding: 0px;
  height: calc(100vh); /* 考虑顶部导航和其他元素的高度 */
  display: flex;
  flex-direction: column;
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
  border: none;
  overflow: hidden;
  flex: 1; /* 占据剩余空间 */
  display: flex;
  flex-direction: column;
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

.page-type-selector {
  margin-top: 20px;
  padding: 0 10px;
  display: flex;
  justify-content: center;
}

/* 可拖动侧边栏样式 */

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
  max-width: calc(100% - 160px) !important;
  /* 确保旋转后不超出视图 */
}

.landscape-mode .form-card-mini,
.landscape-mode .chinese-char-card,
.landscape-mode .card-item {
  transition: all 0.3s ease;
}

.el-main {
  padding: 0;
  border: none;
  flex: 1; /* 占据剩余空间 */
  overflow: auto; /* 允许滚动 */
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