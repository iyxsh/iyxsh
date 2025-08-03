<template>
  <div 
    class="single-form-card-wrapper"
    :class="{ 'preview-mode': previewMode }"
    :style="cardStyle"
    @mousedown="handleMouseDown"
    @dblclick="handleDblClick"
  >
    <el-card 
      :class="['single-form-card', { 'editable': editable, 'no-style': !cardStyleOn }]">
      <div class="form-content">
        <!-- 标题 -->
        <div 
          v-if="shouldShowTitle" 
          class="form-title"
          :style="titleStyle"
        >
          {{ form.title }}
        </div>

        <!-- 内容 -->
        <div 
          v-if="shouldShowValue" 
          class="form-value"
          :style="valueStyle"
        >
          {{ form.value }}
        </div>

        <!-- 备注 -->
        <div 
          v-if="shouldShowRemark" 
          class="form-remark"
          :style="remarkStyle"
        >
          {{ form.remark }}
        </div>

        <!-- 媒体内容（如果有的话） -->
        <div 
          v-if="shouldShowMedia" 
          class="form-media"
          :style="mediaStyle"
        >
          <!-- 处理MediaUploader传递的对象 -->
          <template v-if="form.media && typeof form.media === 'object' && form.media.url">
            <!-- 图片预览 - MediaUploader对象 -->
            <img
              v-if="(form.mediaType === 'image' || !form.mediaType)"
              :src="form.media.url"
              :alt="form.title || 'Form media'"
              class="media-image"
              @error="handleMediaError"
              @load="handleMediaLoad"
            >
            <!-- 视频预览 - MediaUploader对象 -->
            <video
              v-else
              :src="form.media.url"
              controls
              class="media-video"
              @error="handleMediaError"
              @loadeddata="handleVideoLoad"
            ></video>
          </template>
          <!-- 处理字符串URL -->
          <template v-else-if="typeof (form.media || form.mediaPreviewUrl) === 'string' && (form.media || form.mediaPreviewUrl)">
            <!-- 图片预览 - 字符串URL -->
            <img 
              v-if="(form.mediaType === 'image' || !form.mediaType)"
              :src="form.media || form.mediaPreviewUrl"
              :alt="form.title || 'Form media'"
              class="media-image"
              @error="handleMediaError"
              @load="handleMediaLoad"
            >
            <!-- 视频预览 - 字符串URL -->
            <video 
              v-else
              :src="form.media || form.mediaPreviewUrl"
              controls
              class="media-video"
              @error="handleMediaError"
              @loadeddata="handleVideoLoad"
            ></video>
          </template>
          <!-- 处理base64数据 -->
          <template v-else-if="typeof form.media === 'string' && form.media.startsWith('data:')">
            <!-- 图片预览 - base64数据 -->
            <img
              v-if="(form.mediaType === 'image' || !form.mediaType) && form.media.startsWith('data:image/')"
              :src="form.media"
              :alt="form.title || 'Form media'"
              class="media-image"
              @error="handleMediaError"
              @load="handleMediaLoad"
            >
            <!-- 视频预览 - base64数据 -->
            <video
              v-else-if="form.mediaType === 'video' && form.media.startsWith('data:video/')"
              :src="form.media"
              controls
              class="media-video"
              @error="handleMediaError"
              @loadeddata="handleVideoLoad"
            ></video>
          </template>
          <div v-else-if="form.media || form.mediaPreviewUrl" class="media-unavailable">
            <p>无法预览媒体内容</p>
          </div>
        </div>
      </div>

      <!-- 编辑模式下的操作菜单 -->
      <div v-if="editable" class="form-actions">
        <el-dropdown @command="handleCommand">
          <el-button class="action-button" size="small">
            <el-icon><MoreFilled /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="edit">
                <el-icon><Edit /></el-icon> 编辑
              </el-dropdown-item>
              <el-dropdown-item command="delete">
                <el-icon><Delete /></el-icon> 删除
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
      
      <!-- 调整大小手柄 -->
      <div 
        v-if="editable" 
        class="resize-handle" 
        @mousedown="startResize"
      ></div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'
import { MoreFilled, Edit, Delete } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

// 添加Blob工具类
class BlobUtils {
  /**
   * 将Blob转换为可显示的URL
   * @param {Blob} blob 
   * @returns {string} 
   */
  static createObjectURL(blob) {
    return URL.createObjectURL(blob)
  }

  /**
   * 解析Blob URL获取文件扩展名
   * @param {string} url 
   * @returns {string|null}
   */
  static getExtensionFromBlobURL(url) {
    if (!url || !url.startsWith('blob:')) return null
    
    try {
      // Blob URLs don't contain file extensions in the standard format
      // We can't determine the extension from the URL itself
      // Return null to indicate we can't determine the extension
      return null
    } catch (e) {
      console.warn('无法解析Blob URL:', url, e)
      return null
    }
  }

  /**
   * 根据Blob类型确定媒体类型
   * @param {string} url 
   * @param {string} [fallbackType] 
   * @returns {'image'|'video'|null}
   */
  static determineMediaTypeFromBlob(url, fallbackType) {
    if (!url || !url.startsWith('blob:')) return fallbackType || 'image'
    
    // For blob URLs, we cannot determine the media type from the URL itself
    // Return the fallback type or default to 'image'
    return fallbackType || 'image'
  }
}

// 定义props并添加类型注释
const props = defineProps({
  /** @type {Form} */
  form: {
    type: Object,
    required: true
  },
  editable: {
    type: Boolean,
    default: false
  },
  cardStyleOn: {
    type: Boolean,
    default: true
  },
  position: {
    type: Object,
    default: () => ({ x: 20, y: 20 })
  },
  size: {
    type: Object,
    default: () => ({ width: 200, height: 100 })
  },
  // 预览模式标识
  previewMode: {
    type: Boolean,
    default: false
  }
})

// 定义emits
const emit = defineEmits([
  'edit', 
  'delete', 
  'dblclick',
  'mousedown',
  'update-position',
  'update-size'
])

// 响应式数据
const isResizing = ref(false)
const isDragging = ref(false)
const startX = ref(0)
const startY = ref(0)
const startWidth = ref(0)
const startHeight = ref(0)
const startLeft = ref(0)
const startTop = ref(0)

// 计算样式
const cardStyle = computed(() => {
  const baseStyle = {
    position: 'absolute',
    left: `${props.position.x}px`,
    top: `${props.position.y}px`,
    width: `${props.size.width}px`,
    height: `${props.size.height}px`,
    zIndex: props.form.zIndex || 1
  }

  return baseStyle
})

// 判断媒体是否为图片
const isImageMedia = computed(() => {
  // 优先使用media字段，如果没有则使用mediaPreviewUrl字段
  const mediaUrl = props.form?.media || props.form?.mediaPreviewUrl;
  if (!mediaUrl) return true;
  
  // 优先使用表单中的mediaType字段
  if (props.form.mediaType) {
    return props.form.mediaType === 'image';
  }
  
  // 处理Blob URL
  if (typeof mediaUrl === 'string' && mediaUrl.startsWith('blob:')) {
    // 使用BlobUtils类进行媒体类型判断
    return BlobUtils.determineMediaTypeFromBlob(mediaUrl) === 'image';
  }
  
  // 处理base64数据URL
  if (typeof mediaUrl === 'string' && mediaUrl.startsWith('data:image/')) {
    return true;
  }
  if (typeof mediaUrl === 'string' && mediaUrl.startsWith('data:video/')) {
    return false;
  }
  
  // 通过文件扩展名判断
  const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
  const videoExtensions = /\.(mp4|webm|ogg|avi|mov|wmv|flv|mkv)$/i;
  
  if (typeof mediaUrl === 'string') {
    if (imageExtensions.test(mediaUrl)) {
      return true;
    }
    if (videoExtensions.test(mediaUrl)) {
      return false;
    }
  }
  // 默认返回true（当作图片处理）
  return true;
})

// 检查对象是否为File类型
const isFileObject = (obj) => {
  return obj instanceof File || (obj && obj.constructor && obj.constructor.name === 'File')
};

/**
 * 创建媒体预览URL
 * @param {File|Blob|string} media - 媒体文件或URL
 * @returns {string|null}
 */
const createMediaPreviewUrl = (media) => {
  if (!media) return null
  
  // 如果是File或Blob对象，创建预览URL
  if (media instanceof Blob) {
    return URL.createObjectURL(media)
  }
  
  // 如果是字符串类型的URL直接返回
  if (typeof media === 'string') {
    return media
  }
  
  return null
}

// 元素样式计算 - 使用表单的elementStyles并通过StyleManager处理
const titleStyle = computed(() => {
  if (!props.form?.elementStyles?.title?.enabled) return {}
  const style = props.form.elementStyles.title
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

const valueStyle = computed(() => {
  if (!props.form?.elementStyles?.value?.enabled) return {}
  const style = props.form.elementStyles.value
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

const remarkStyle = computed(() => {
  if (!props.form?.elementStyles?.remark?.enabled) return {}
  const style = props.form.elementStyles.remark
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
  }
})

const mediaStyle = computed(() => {
  if (!props.form?.elementStyles?.media?.enabled) return {}
  const style = props.form.elementStyles.media
  // 使用StyleManager方式处理样式
  return {
    color: style.color || '',
    fontSize: style.fontSize ? `${style.fontSize}px` : '',
    fontWeight: style.fontWeight || 'normal'
    // 添加其他媒体样式
  }
})

// 是否显示标题
const shouldShowTitle = computed(() => {
  return props.form?.showTitle !== false && props.form?.title
})

// 是否显示内容
const shouldShowValue = computed(() => {
  return props.form?.showValue !== false && props.form?.value
})

// 是否显示备注
const shouldShowRemark = computed(() => {
  return props.form?.showRemark !== false && props.form?.remark
})

// 是否显示媒体
const shouldShowMedia = computed(() => {
  // 如果showMedia字段不存在，默认为true（向后兼容）
  const showMedia = props.form?.showMedia !== undefined ? props.form?.showMedia : true
  
  // 检查是否有media或mediaPreviewUrl
  let hasMedia = false;
  
  // 检查media字段
  if (props.form?.media) {
    if (typeof props.form.media === 'string' && props.form.media.trim() !== '') {
      hasMedia = true;
    } else if (typeof props.form.media === 'object') {
      // 处理MediaUploader传递的对象或File对象
      if (props.form.media instanceof File) {
        hasMedia = props.form.media.size > 0;
      } else if (props.form.media.url && typeof props.form.media.url === 'string') {
        // MediaUploader传递的对象
        hasMedia = props.form.media.url.trim() !== '';
      } else {
        // 其他对象类型
        hasMedia = true;
      }
    }
  }
  
  // 如果media字段没有有效内容，检查mediaPreviewUrl字段
  if (!hasMedia && props.form?.mediaPreviewUrl) {
    hasMedia = typeof props.form.mediaPreviewUrl === 'string' && props.form.mediaPreviewUrl.trim() !== '';
  }
  
  return showMedia !== false && hasMedia
});

// 处理命令
const handleCommand = (command) => {
  switch (command) {
    case 'edit':
      editForm()
      break
    case 'delete':
      deleteForm()
      break
  }
}

// 处理双击事件
const handleDblClick = (event) => {
  event.stopPropagation()
  emit('dblclick', props.form)
}

// 处理鼠标按下事件（拖拽开始）
const handleMouseDown = (event) => {
  event.stopPropagation()
  
  // 如果点击的是调整大小的手柄，则不处理拖拽
  if (event.target.classList.contains('resize-handle')) {
    return
  }
  
  isDragging.value = true
  startX.value = event.clientX
  startY.value = event.clientY
  startLeft.value = props.position.x
  startTop.value = props.position.y
  
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', stopDrag)
  
  emit('mousedown', props.form)
}

// 处理拖拽
const handleDrag = (event) => {
  if (!isDragging.value) return
  
  const deltaX = event.clientX - startX.value
  const deltaY = event.clientY - startY.value
  
  const newX = startLeft.value + deltaX
  const newY = startTop.value + deltaY
  
  emit('update-position', props.form.id, newX, newY)
}

// 停止拖拽
const stopDrag = () => {
  isDragging.value = false
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
}

// 开始调整大小
const startResize = (event) => {
  event.stopPropagation()
  isResizing.value = true
  startX.value = event.clientX
  startY.value = event.clientY
  startWidth.value = props.size.width
  startHeight.value = props.size.height
  
  document.addEventListener('mousemove', handleResize)
  document.addEventListener('mouseup', stopResize)
}

// 处理调整大小过程
const handleResize = (event) => {
  if (!isResizing.value) return
  
  const deltaX = event.clientX - startX.value
  const deltaY = event.clientY - startY.value
  
  const newWidth = Math.max(100, startWidth.value + deltaX)
  const newHeight = Math.max(50, startHeight.value + deltaY)
  
  emit('update-size', {
    formId: props.form.id,
    size: {
      width: newWidth,
      height: newHeight
    }
  })
}

// 停止调整大小
const stopResize = () => {
  isResizing.value = false
  document.removeEventListener('mousemove', handleResize)
  document.removeEventListener('mouseup', stopResize)
}

// 编辑表单
const editForm = () => {
  emit('edit', props.form)
}

// 删除表单
const deleteForm = () => {
  emit('delete', props.form)
}

// 处理媒体加载完成
const handleMediaLoad = (event) => {
  console.log('SingleFormShow: Image loaded successfully', event);
}

// 处理视频加载完成
const handleVideoLoad = (event) => {
  console.log('SingleFormShow: Video loaded successfully', event);
}

// 处理媒体错误
const handleMediaError = (event) => {
  console.error('SingleFormShow: Media load error', event)
  
  let errorMessage = '媒体加载失败'
  
  // 处理不同类型的媒体错误
  if (event && event.target && event.target.error) {
    const error = event.target.error
    
    switch(error.code) {
      case MediaError.MEDIA_ERR_ABORTED:
        errorMessage = '用户取消了媒体加载'
        break
      case MediaError.MEDIA_ERR_NETWORK:
        errorMessage = '网络错误导致媒体加载失败'
        break
      case MediaError.MEDIA_ERR_DECODE:
        errorMessage = '媒体解码失败（文件可能已损坏）'
        break
      case MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED:
        errorMessage = '不支持的媒体格式'
        break
      default:
        errorMessage = `未知媒体错误 [${error.code}]`
        break
    }
    
    // 添加更多调试信息
    console.error('SingleFormShow: Error details:', {
      code: error.code,
      message: error.message,
      src: event.target.src
    })
  } else if (event && event.message) {
    // 处理其他类型的错误事件
    console.error('SingleFormShow: Error message:', event.message)
    errorMessage = `媒体加载错误: ${event.message}`
  } else if (event && event.target && event.target.src) {
    // 处理一般加载错误
    const src = event.target.src
    if (src.startsWith('blob:')) {
      errorMessage = '本地媒体文件加载失败'
    } else {
      errorMessage = `媒体加载失败 [URL: ${src}]`
    }
  }
  
  // 只在编辑模式下显示错误消息
  if (props.editable) {
    ElMessage.error(errorMessage)
  }
}

// 组件卸载前
onBeforeUnmount(() => {
  // 清理事件监听器
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
  document.removeEventListener('mousemove', handleResize)
  document.removeEventListener('mouseup', stopResize)
})
</script>

<style scoped>
.single-form-card-wrapper {
  cursor: move;
  transition: box-shadow 0.3s ease, opacity 0.3s ease;
}

/* 预览模式下不使用绝对定位 */
.single-form-card-wrapper.preview-mode {
  position: relative !important;
  left: auto !important;
  top: auto !important;
  width: 100% !important;
  height: auto !important;
}

.single-form-card-wrapper:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.single-form-card {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.single-form-card.editable {
  cursor: move;
}

.single-form-card :deep(.el-card__body) {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 12px;
  overflow: hidden;
}

.form-title,
.form-value,
.form-remark {
  margin-bottom: 8px;
  word-wrap: break-word;
  white-space: pre-wrap;
}

.form-title:last-child,
.form-value:last-child,
.form-remark:last-child {
  margin-bottom: 0;
}

.form-media {
  margin-top: 8px;
}

.media-image,
.media-video {
  max-width: 100%;
  max-height: 200px;
  object-fit: contain;
  border-radius: 4px;
}

.form-actions {
  position: absolute;
  top: 8px;
  right: 8px;
  z-index: 10;
}

.action-button {
  background: rgba(255, 255, 255, 0.9);
  border: 1px solid #dcdfe6;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.resize-handle {
  position: absolute;
  bottom: 0;
  right: 0;
  width: 12px;
  height: 12px;
  background-color: #409eff;
  cursor: se-resize;
  z-index: 10;
  opacity: 0;
  transition: opacity 0.3s ease;
}

.single-form-card-wrapper:hover .resize-handle {
  opacity: 1;
}

.no-style {
  background: transparent !important;
  border: none !important;
  box-shadow: none !important;
}

.no-style :deep(.el-card__body) {
  padding: 0 !important;
  background: transparent !important;
}
</style>