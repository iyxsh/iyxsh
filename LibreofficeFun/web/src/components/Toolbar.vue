<template>
  <div class="toolbar-container" :class="{ 'toolbar-dragging': isDragging, 'toolbar-docked': isDocked, 'toolbar-minimized': isMinimized, 'toolbar-floating': !isDocked }" ref="toolbarContainer" :style="toolbarPosition">
    <!-- 拖动手柄 -->
    <div class="toolbar-drag-handle" @mousedown="startDrag">
      <el-icon><Rank /></el-icon>
    </div>

    <!-- 工具栏操作按钮 -->
    <div class="toolbar-actions">
      <el-button circle size="small" @click="toggleMinimize" class="toolbar-action-button">
        <el-icon>
          <component :is="isMinimized ? 'Plus' : 'Minus'"></component>
        </el-icon>
      </el-button>
      <el-button circle size="small" @click="toggleDock" class="toolbar-action-button">
        <el-icon>
          <component :is="isDocked ? 'TakeawayBox' : 'Position'"></component>
        </el-icon>
      </el-button>
    </div>

    <!-- 主工具栏内容 -->
    <div class="toolbar" v-show="!isMinimized">
      <div class="toolbar-section toolbar-left">
        <el-tooltip content="添加新页面" placement="bottom" :disabled="!isEditable">
          <el-button @click="addPage" :disabled="!isEditable" type="success" plain>
            <el-icon><DocumentAdd /></el-icon>
            <span class="button-text">添加页面</span>
          </el-button>
        </el-tooltip>
        
        <el-tooltip content="删除当前页面" placement="bottom" :disabled="!isEditable">
          <el-button @click="removeCurrentPage" :disabled="!isEditable || (props.pages && props.pages.length <= 1)" type="danger" plain>
            <el-icon><Delete /></el-icon>
            <span class="button-text">删除页面</span>
          </el-button>
        </el-tooltip>

        <el-tooltip content="清除所有页面" placement="bottom" :disabled="!isEditable">
          <el-button @click="clearAllPages" :disabled="!isEditable" type="danger" plain>
            <el-icon><Delete /></el-icon>
            <span class="button-text">清除全部</span>
          </el-button>
        </el-tooltip>

        <el-tooltip content="旋转页面" placement="bottom" :disabled="!isEditable">
          <el-button @click="rotatePage" :disabled="!isEditable || !showRotateButton" type="warning" plain>
            <el-icon><Refresh /></el-icon>
            <span class="button-text">旋转页面</span>
          </el-button>
        </el-tooltip>
      </div>

      <div class="toolbar-section toolbar-center">
        <el-tooltip 
          effect="dark" 
          :content="isEditable ? '锁定页面' : '解锁页面'" 
          placement="bottom"
        >
          <el-button 
            type="primary" 
            size="small" 
            @click="toggleEditPage"
          >
            <el-icon>
              <Lock v-if="!isEditable" />
              <Unlock v-else />
            </el-icon>
            <span class="button-text">{{ isEditable ? '锁定' : '解锁' }}</span>
          </el-button>
        </el-tooltip>

        <el-tooltip content="选择要编辑的页面" placement="bottom">
          <el-select v-model="selectedPage" placeholder="选择页面" @change="changePage" size="small">
            <el-option
              v-for="(page, index) in pages"
              :key="index"
              :label="page.name || `页面 ${index + 1}`"
              :value="index"
            >
              <div class="page-option" @click.stop>
                <span 
                  v-if="!editingPageName || editingPageIndex !== index" 
                  @dblclick.stop="startEditPageName(index)"
                  class="page-name"
                >
                  {{ page.name || `页面 ${index + 1}` }}
                </span>
                <el-input
                  v-else
                  v-model="newPageName"
                  size="small"
                  ref="pageNameInput"
                  @blur="savePageName(index)"
                  @keyup.enter="savePageName(index)"
                  @keyup.esc="cancelEditPageName"
                />
              </div>
            </el-option>
          </el-select>
        </el-tooltip>

        <el-tooltip content="切换编辑/预览模式" placement="bottom">
          <el-switch
            :model-value="isEditable"
            @update:model-value="toggleEditMode"
            active-text="编辑"
            inactive-text="预览"
            inline-prompt
          />
        </el-tooltip>
      </div>

      <div class="toolbar-section toolbar-right">
        <el-button-group>
          <el-tooltip content="导出数据" placement="bottom">
            <el-button @click="exportData" type="info" plain>
              <el-icon><Upload /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="导入数据" placement="bottom">
            <el-button @click="importData" type="info" plain>
              <el-icon><Download /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="保存全部" placement="bottom">
            <el-button @click="saveAll" type="primary">
              <el-icon><Save /></el-icon>
              <span class="button-text">保存</span>
            </el-button>
          </el-tooltip>
        </el-button-group>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, computed, nextTick, watch, getCurrentInstance } from 'vue'
import { ElMessage, ElMessageBox, ElDialog, ElButton, ElTooltip, ElInput } from 'element-plus'
import { useEventBus } from '../utils/eventBus'
import { DocumentAdd, Delete, Upload, Download, Position, Rank, Plus, Minus, Refresh, Lock, Unlock } from '@element-plus/icons-vue'
import { Share as Save } from '@element-plus/icons-vue'
import { CollectionTag as TakeawayBox } from '@element-plus/icons-vue'
import errorLogService from '../services/errorLogService'

// 添加组件加载调试信息
console.log('Toolbar component initializing');

// 定义props
const props = defineProps({
  pages: Array,
  currentPage: Number,
  editable: Boolean,
  onAddPage: {
    type: Function,
    required: true
  },
  clearAllPages: Function,
  rotatePage: Function,
  showRotateButton: Boolean,
  onRemovePage: Function, // 添加删除页面的回调函数
  newPageName: String // 添加新页面名称prop
})

// 创建计算属性或本地状态以避免直接修改props
const isEditable = ref(props.editable)

// 定义emits
const emit = defineEmits(['toggle-edit-mode', 'change-page', 'export-data', 'import-data', 'save-all', 'on-add-page', 'remove-page', 'rename-page', 'toggle-edit-page'])

// 使用事件总线
const { on, off } = useEventBus()

// 本地状态
const selectedPage = ref(props.currentPage)

// 页面名称编辑状态
const editingPageName = ref(false)
const editingPageIndex = ref(-1)
const newPageName = ref('')
const pageNameInput = ref(null)

// 监听props.currentPage变化
watch(() => props.currentPage, (newVal) => {
  console.log('[Toolbar] currentPage prop changed:', newVal);
  if (newVal !== undefined && newVal !== null) {
    selectedPage.value = newVal;
  }
}, { immediate: true });

// 监听props.pages变化
watch(() => props.pages, (newPages) => {
  console.log('[Toolbar] pages prop changed:', newPages?.length);
  // 如果当前选中的页面超出范围，则重置为0
  if (newPages && selectedPage.value >= newPages.length) {
    selectedPage.value = Math.max(0, newPages.length - 1);
  }
}, { deep: true });

// 拖拽相关状态
const isDragging = ref(false)
const isDocked = ref(false) // 默认为浮动状态
const isMinimized = ref(false)
const toolbarContainer = ref(null)
const dragOffset = reactive({ x: 0, y: 0 })
const position = reactive({ x: 20, y: 20 })

// 计算工具栏位置样式
const toolbarPosition = computed(() => {
  if (isDocked.value) return {}
  return {
    left: `${position.x}px`,
    top: `${position.y}px`,
    position: 'fixed',
    zIndex: 1000,
    margin: 0, // 覆盖默认margin
    width: 'auto' // 让宽度自适应内容
  }
})

// 错误处理函数
const handleError = (error, context = '未知上下文') => {
  console.error(`[Toolbar Error] ${context}`, error);
  ElMessage.error({
    message: `操作失败: ${context}`,
    duration: 5000,
    type: 'error'
  });
  
  // 记录错误日志
  errorLogService.addErrorLog(error, context, 'error');
}

// 处理页面切换
const changePage = (pageIndex) => {
  try {
    console.log('[Toolbar] 页面切换事件触发:', pageIndex);
    
    // 确保pageIndex是数字类型
    const pageNum = Number(pageIndex);
    
    if (isNaN(pageNum) || pageNum < 0 || (props.pages && pageNum >= props.pages.length)) {
      throw new Error(`无效的页面索引: ${pageNum}`);
    }
    
    selectedPage.value = pageNum;
    emit('change-page', pageNum);
  } catch (error) {
    handleError(error, '页面切换失败');
  }
}

// 添加页面方法
const addPage = async () => {
  try {
    // 使用emit触发添加页面事件，确保事件名称与PageManager.vue中监听的事件一致
    emit('on-add-page')
    
    // 调用 API 服务添加工作表
    try {
      // 使用全局属性方式访问API服务
      const apiService = window.$apiService
      if (apiService && typeof apiService.addWorksheet === 'function') {
        // 获取当前文件名和新建页面名称
        const newPageNameValue = props.newPageName || `Sheet${(props.pages ? props.pages.length : 0) + 1}`
        const fileName = window.location.search.includes('fileName=') ? 
          decodeURIComponent(window.location.search.split('fileName=')[1].split('&')[0] || window.location.search.split('fileName=')[1]) : 
          '未命名文件'
        
        // 传递正确的参数格式
        const worksheetData = {
          filename: fileName,
          sheetname: newPageNameValue
        }
        
        // 调用API服务的addWorksheet方法
        const response = await apiService.addWorksheet(worksheetData)
        if(response.errorCode !== 1000 || response.errorMessage !== "Success") {
          console.error('[Toolbar] 添加工作表失败:', response);
          ElMessage.error('添加工作表失败: ' + (response.errorMessage || '未知错误'));
          return;
        }

        console.log('[Toolbar] 成功调用 addWorksheet API')
      } else {
        console.warn('[Toolbar] ApiService不可用或缺少addWorksheet方法')
      }
    } catch (apiError) {
      console.error('[Toolbar] 调用 addWorksheet API 失败:', apiError)
      // 不阻塞 UI 操作，仅记录错误
    }
  } catch (error) {
    handleError(error, '添加页面失败')
  }
}

// 清空所有页面
const clearAllPages = () => {
  try {
    if (!isEditable.value) {
      showUnlockMessage()
      return
    }
    
    ElMessageBox.confirm(
      '确定要清除所有页面吗？此操作无法撤销。',
      '清除确认',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    ).then(() => {
      // 调用传入的clearAllPages函数
      if (props.clearAllPages && typeof props.clearAllPages === 'function') {
        props.clearAllPages()
      } else {
        console.warn('[Toolbar] clearAllPages 方法未定义或不是函数')
        ElMessage.warning('清空所有页面功能不可用')
      }
    }).catch(() => {
      // 用户取消操作
    })
  } catch (error) {
    handleError(error, '清空所有页面失败')
  }
}

// 切换编辑模式
const toggleEditMode = (value) => {
  try {
    isEditable.value = value
    emit('toggle-edit-mode', value)
  } catch (error) {
    handleError(error, '切换编辑模式失败')
  }
}

// 切换页面编辑状态
const toggleEditPage = () => {
  try {
    // 切换编辑状态
    const newEditableState = !isEditable.value;
    isEditable.value = newEditableState;
    emit('toggle-edit-page', props.currentPage);
    emit('toggle-edit-mode', newEditableState);
  } catch (error) {
    handleError(error, '切换页面编辑状态失败');
  }
}

// 删除当前页面
const removeCurrentPage = async () => {
  try {
    if (!isEditable.value) {
      showUnlockMessage();
      return;
    }
    
    // 检查是否只剩一个页面
    if (props.pages && props.pages.length <= 1) {
      ElMessage.warning('至少需要保留一个页面');
      return;
    }
    
    // 获取当前选中页面的索引
    const currentPageIndex = selectedPage.value;
    
    // 获取要删除的页面名称
    const pageName = props.pages && props.pages[currentPageIndex] 
      ? (props.pages[currentPageIndex].name || `Sheet${currentPageIndex + 1}`)
      : `Sheet${currentPageIndex + 1}`;
    
    // 确认删除
    await ElMessageBox.confirm(
      `确定要删除页面"${pageName}"吗？此操作无法撤销。`,
      '删除确认',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    );

    // 调用API服务删除工作表
    try {
      const apiService = window.$apiService;
      if (apiService && typeof apiService.removeWorksheet === 'function') {
        // 获取文件名
        const fileName = window.location.search.includes('fileName=') ? 
          decodeURIComponent(window.location.search.split('fileName=')[1].split('&')[0] || window.location.search.split('fileName=')[1]) : 
          '未命名文件';
        
        // 调用API服务的removeWorksheet方法
        const response = await apiService.removeWorksheet({
          filename: fileName,
          sheetname: pageName
        });
        if(response.errorCode !== 1000 || response.errorMessage !== "Success") {
          console.error('[Toolbar] 删除工作表失败:', response);
          ElMessage.error('删除工作表失败: ' + (response.errorMessage || '未知错误'));
          return;
        }

        console.log('[Toolbar] 成功调用 removeWorksheet API');
      } else {
        console.warn('[Toolbar] ApiService不可用或缺少removeWorksheet方法');
      }
    } catch (apiError) {
      console.error('[Toolbar] 调用 removeWorksheet API 失败:', apiError);
      ElMessage.error('删除工作表失败: ' + (apiError.message || '未知错误'));
      return; // 阻止继续执行
    }

    // 发出删除页面事件
    emit('remove-page', currentPageIndex);
  } catch (error) {
    handleError(error, '删除页面失败');
  }
}

// 导出数据
const exportData = () => {
  try {
    emit('export-data')
  } catch (error) {
    handleError(error, '导出数据失败')
  }
}

// 导入数据
const importData = () => {
  try {
    emit('import-data')
  } catch (error) {
    handleError(error, '导入数据失败')
  }
}

// 保存所有数据
const saveAll = () => {
  try {
    emit('save-all')
  } catch (error) {
    handleError(error, '保存数据失败')
  }
}

// 显示解锁提示
const showUnlockMessage = () => {
  ElMessage.warning({
    message: '请先切换到编辑模式再执行此操作',
    duration: 3000
  })
}

// 资源清理
const cleanupTasks = []

// 添加内存泄漏检查
function checkMemoryUsage() {
  if (performance.memory) {
    const memoryUsage = performance.memory
    console.log('Toolbar Memory Usage:', {
      usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
      totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
      jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
    })

    // 如果内存使用超过阈值，触发警告
    if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
      console.warn('Toolbar: Memory usage is high, consider optimizing')
    }
  }
}

// 格式化字节大小
function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes'
  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

// 优化内存泄漏检查
function setupMemoryLeakCheck() {
  // 检查内存使用情况
  const memoryCheckInterval = setInterval(() => {
    checkMemoryUsage()
  }, 5 * 60 * 1000)

  // 添加清理任务
  cleanupTasks.push(() => clearInterval(memoryCheckInterval))
}

// 添加资源清理
function cleanupResources() {
  // 清理内存检查
  cleanupTasks.forEach(task => task())
}

// 在组件挂载时添加调试信息
onMounted(() => {
  // 初始化内存泄漏检查
  setupMemoryLeakCheck()

  console.log('Toolbar component mounted', {
    currentPage: props.currentPage,
    editable: props.editable
  });
})

// 工具栏拖动相关方法
const startDrag = (event) => {
  // 防止事件冒泡和默认行为
  event.stopPropagation()
  event.preventDefault()
  
  console.log('开始拖动工具栏', { isDocked: isDocked.value, clientX: event.clientX, clientY: event.clientY })
  
  // 如果工具栏已停靠，点击拖动手柄时解除停靠
  if (isDocked.value) {
    isDocked.value = false
    localStorage.setItem('toolbar-docked', 'false')
    
    // 设置初始位置在点击的位置附近
    position.x = Math.max(0, event.clientX - 100)
    position.y = Math.max(0, event.clientY - 20)
    
    // 确保视图更新后再开始拖动
    nextTick(() => {
      isDragging.value = true
      dragOffset.x = 100
      dragOffset.y = 20
      document.addEventListener('mousemove', handleDrag)
      document.addEventListener('mouseup', stopDrag)
    })
    return
  }
  
  // 已经是浮动状态，直接开始拖动
  isDragging.value = true
  
  // 计算鼠标点击位置相对于工具栏左上角的偏移量
  dragOffset.x = event.clientX - position.x
  dragOffset.y = event.clientY - position.y
  
  // 添加拖动开始样式
  toolbarContainer.value.classList.add('toolbar-dragging')
  
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', stopDrag)
  document.addEventListener('mouseleave', stopDrag) // 添加mouseleave事件
  document.addEventListener('selectstart', preventSelection) // 防止文本选择
  document.addEventListener('contextmenu', preventSelection) // 防止右键菜单
  
  // 防止选择文本和拖动过程中的默认行为
  event.preventDefault()
}

// 添加防抖函数
function debounce(func, delay) {
  let timeout
  return (...args) => {
    clearTimeout(timeout)
    timeout = setTimeout(() => {
      func.apply(this, args)
    }, delay)
  }
}

const handleDrag = debounce((event) => {
  if (!isDragging.value) return
  
  // 使用transform代替left/top提升性能
  const newX = event.clientX - dragOffset.x
  const newY = event.clientY - dragOffset.y
  
  // 快速边界检测
  const { clientWidth, clientHeight } = document.documentElement
  const toolbar = toolbarContainer.value
  
  if (toolbar) {
    const { width, height } = toolbar.getBoundingClientRect()
    
    // 使用位运算代替Math.max/Math.min提升性能
    const x = (newX > clientWidth - width ? clientWidth - width : newX) | 0
    const y = (newY > clientHeight - height ? clientHeight - height : newY) | 0
    
    position.x = x < 0 ? 0 : x
    position.y = y < 0 ? 0 : y
  }
}, 16) // 限制在16ms内只执行一次（约60fps）

// 防止文本选择
function preventSelection(e) {
  e.preventDefault()
}
// 工具栏停靠/浮动切换
const toggleDock = () => {
  isDocked.value = !isDocked.value
  localStorage.setItem('toolbar-docked', isDocked.value.toString())
  
  if (!isDocked.value) {
    // 如果未停靠，确保位置合理
    nextTick(() => {
      const toolbar = toolbarContainer.value
      if (toolbar) {
        const { width, height } = toolbar.getBoundingClientRect()
        const { clientWidth, clientHeight } = document.documentElement
        
        position.x = Math.max(0, Math.min(position.x, clientWidth - width))
        position.y = Math.max(0, Math.min(position.y, clientHeight - height))
      }
    })
  }
}

// 停止拖动
const stopDrag = () => {
  if (!isDragging.value) return
  
  isDragging.value = false
  
  // 移除拖动样式
  if (toolbarContainer.value) {
    toolbarContainer.value.classList.remove('toolbar-dragging')
  }
  
  // 移除事件监听器
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
  document.removeEventListener('mouseleave', stopDrag)
  document.removeEventListener('selectstart', preventSelection)
  document.removeEventListener('contextmenu', preventSelection)
  
  // 保存位置到本地存储
  localStorage.setItem('toolbar-position', JSON.stringify(position))
}

// 最小化/展开工具栏
const toggleMinimize = () => {
  isMinimized.value = !isMinimized.value
  localStorage.setItem('toolbar-minimized', isMinimized.value.toString())
}

// 开始编辑页面名称
const startEditPageName = (index) => {
  if (!isEditable.value) {
    showUnlockMessage()
    return
  }
  
  editingPageName.value = true
  editingPageIndex.value = index
  newPageName.value = props.pages[index].name || `页面 ${index + 1}`
  
  nextTick(() => {
    // 修复：正确访问Element Plus el-input组件的focus方法
    if (pageNameInput.value && typeof pageNameInput.value.focus === 'function') {
      pageNameInput.value.focus()
    } else if (pageNameInput.value && pageNameInput.value.$el) {
      // 如果是Element Plus组件实例，获取其内部的input元素
      const inputEl = pageNameInput.value.$el.querySelector('input')
      if (inputEl && typeof inputEl.focus === 'function') {
        inputEl.focus()
      }
    }
  })
}

// 保存页面名称
const savePageName = async (index) => {
  try {
    if (!isEditable.value) {
      showUnlockMessage()
      cancelEditPageName()
      return
    }
    
    const trimmedName = newPageName.value.trim()
    
    // 验证名称不为空
    if (!trimmedName) {
      ElMessage.warning('页面名称不能为空')
      return
    }
    
    // 检测名称重复
    const isDuplicate = props.pages.some((page, pageIndex) => 
      pageIndex !== index && (page.name || `页面 ${pageIndex + 1}`) === trimmedName)
    
    if (isDuplicate) {
      ElMessage.warning('页面名称不能重复')
      return
    }
    
    const oldName = props.pages[index].name || `页面 ${index + 1}`
    
    if (trimmedName && trimmedName !== oldName) {
      // 调用API服务重命名工作表
      try {
        const apiService = window.$apiService
        if (apiService && typeof apiService.renameWorksheet === 'function') {
          // 获取文件名
          const fileName = window.location.search.includes('fileName=') ? 
            decodeURIComponent(window.location.search.split('fileName=')[1].split('&')[0] || window.location.search.split('fileName=')[1]) : 
            '未命名文件'
          
          // 调用API服务的renameWorksheet方法
          const response = await apiService.renameWorksheet({
            filename: fileName,
            sheetname: oldName,
            newsheetname: trimmedName
          });
          if(response.errorCode !== 1000 || response.errorMessage !== "Success") {
            console.error('[Toolbar] 重命名工作表失败:', response);
            ElMessage.error('重命名工作表失败: ' + (response.errorMessage || '未知错误'));
            return;
          }
          console.log('[Toolbar] 成功调用 renameWorksheet API')
        } else {
          console.warn('[Toolbar] ApiService不可用或缺少renameWorksheet方法')
        }
      } catch (apiError) {
        console.error('[Toolbar] 调用 renameWorksheet API 失败:', apiError)
        ElMessage.error('重命名工作表失败: ' + (apiError.message || '未知错误'))
      }
      
      // 通知父组件执行重命名操作
      emit('rename-page', index, trimmedName)
      
      // 显示成功提示
      ElMessage.success('页面名称已更新')
    }
  } catch (error) {
    handleError(error, '重命名页面失败')
  } finally {
    cancelEditPageName()
  }
}

// 取消编辑页面名称
const cancelEditPageName = () => {
  editingPageName.value = false
  editingPageIndex.value = -1
  newPageName.value = ''
}

// 加载保存的工具栏状态
const loadToolbarState = () => {
  try {
    // 加载位置
    const savedPosition = localStorage.getItem('toolbar-position')
    if (savedPosition) {
      try {
        const parsed = JSON.parse(savedPosition)
        position.x = parsed.x || 20
        position.y = parsed.y || 20
      } catch (e) {
        console.error('解析工具栏位置失败', e)
        position.x = 20
        position.y = 20
      }
    }
    
    // 加载停靠状态，默认为浮动状态（非固定）
    const savedDocked = localStorage.getItem('toolbar-docked')
    if (savedDocked !== null) {
      isDocked.value = savedDocked === 'true'
    } else {
      // 默认为浮动状态
      isDocked.value = false
      localStorage.setItem('toolbar-docked', 'false')
    }
    
    // 加载最小化状态
    const savedMinimized = localStorage.getItem('toolbar-minimized')
    if (savedMinimized !== null) {
      isMinimized.value = savedMinimized === 'true'
    }
  } catch (error) {
    console.error('加载工具栏状态失败', error)
  }
}

// 在组件挂载时加载状态
onMounted(() => {
  // 初始化内存泄漏检查
  setupMemoryLeakCheck()
  
  // 加载工具栏状态
  loadToolbarState()

  console.log('Toolbar component mounted', {
    currentPage: props.currentPage,
    editable: props.editable,
    isDocked: isDocked.value,
    isMinimized: isMinimized.value
  });
  
  // 添加窗口大小变化监听器
  window.addEventListener('resize', handleWindowResize)
  cleanupTasks.push(() => window.removeEventListener('resize', handleWindowResize))
  
  // 添加键盘事件监听器
  document.addEventListener('keydown', handleKeyDown)
  cleanupTasks.push(() => document.removeEventListener('keydown', handleKeyDown))
})

// 处理窗口大小变化
const handleWindowResize = () => {
  // 使用requestAnimationFrame优化性能
  requestAnimationFrame(() => {
    if (!isDocked.value && toolbarContainer.value) {
      const { width, height } = toolbarContainer.value.getBoundingClientRect()
      const { clientWidth, clientHeight } = document.documentElement
      
      // 快速边界检测
      const x = position.x > clientWidth - width ? clientWidth - width : position.x
      const y = position.y > clientHeight - height ? clientHeight - height : position.y
      
      position.x = x < 0 ? 0 : x
      position.y = y < 0 ? 0 : y
    }
  })
}

  // 处理键盘事件
  const handleKeyDown = (event) => {
    // Ctrl+D 切换停靠状态
    if (event.ctrlKey && event.key === 'd') {
      event.preventDefault()
      toggleDock()
    }
    
    // Ctrl+M 切换最小化状态
    if (event.ctrlKey && event.key === 'm') {
      event.preventDefault()
      toggleMinimize()
    }
  }

// 在组件卸载前执行清理
onBeforeUnmount(() => {
  // 执行资源清理
  cleanupResources()
  
  // 移除事件监听器
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
  document.removeEventListener('mouseleave', stopDrag)
  document.removeEventListener('selectstart', preventSelection)
  document.removeEventListener('contextmenu', preventSelection)
  window.removeEventListener('resize', handleWindowResize)
  document.removeEventListener('keydown', handleKeyDown)
  
  console.log('Toolbar component about to be unmounted');
})

// 添加调试信息
defineExpose({
  position,
  isDragging,
  isDocked,
  isMinimized
})
</script>

<style scoped>
.toolbar-container {
  display: flex;
  align-items: center;
  padding: 8px 12px;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 8px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(0, 0, 0, 0.05);
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  z-index: 1000;
  user-select: none;
  margin-bottom: 15px;
  min-height: 48px;
}

.toolbar-container.toolbar-dragging {
  box-shadow: 0 4px 16px 0 rgba(0, 0, 0, 0.15);
  transform: scale(1.02);
}

.toolbar-container.toolbar-docked {
  position: static !important;
  margin: 0;
  border-radius: 0;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.1);
  width: 100%;
  backdrop-filter: none;
}

.toolbar-container.toolbar-minimized {
  width: auto;
  padding: 4px;
}

.toolbar-drag-handle {
  cursor: move;
  padding: 4px 8px;
  margin-right: 8px;
  border-radius: 4px;
  transition: background-color 0.2s;
  display: flex;
  align-items: center;
  justify-content: center;
}

.toolbar-drag-handle:hover {
  background-color: rgba(0, 0, 0, 0.05);
}

.toolbar-actions {
  display: flex;
  gap: 4px;
  margin-right: 8px;
  align-items: center;
}

.toolbar-action-button {
  padding: 6px;
  border: none;
  background: transparent;
  cursor: pointer;
  border-radius: 4px;
  transition: all 0.2s;
  display: flex;
  align-items: center;
  justify-content: center;
}

.toolbar-action-button:hover {
  background-color: rgba(0, 0, 0, 0.05);
}

.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  gap: 12px;
}

.toolbar-section {
  display: flex;
  align-items: center;
  gap: 8px;
}

.toolbar-left {
  flex: 1;
}

.toolbar-center {
  display: flex;
  align-items: center;
  gap: 12px;
}

.toolbar-right {
  display: flex;
  align-items: center;
  gap: 12px;
}

.button-text {
  margin-left: 4px;
}

.page-option {
  display: flex;
  align-items: center;
  width: 100%;
}

.page-name {
  cursor: pointer;
  padding: 2px 4px;
  border-radius: 2px;
}

.page-name:hover {
  background-color: rgba(0, 0, 0, 0.05);
}

@media (max-width: 768px) {
  .toolbar {
    flex-direction: column;
    align-items: stretch;
  }
  
  .toolbar-section {
    justify-content: center;
    margin: 4px 0;
  }
  
  .toolbar-right {
    justify-content: center;
  }
  
  .button-text {
    display: none;
  }

  .toolbar .el-button {
    padding: 6px;
    min-width: unset;
  }

  .toolbar-container:not(.toolbar-docked) {
    min-width: 200px;
  }
}

/* 在小屏幕上隐藏按钮文本，只显示图标 */
@media (max-width: 480px) {
  .button-text {
    display: none;
  }

  .toolbar .el-button {
    padding: 6px;
    min-width: unset;
  }

  .toolbar-container:not(.toolbar-docked) {
    min-width: 200px;
  }
}
</style>