<template>
  <div class="single-form-card-wrapper" :class="{ 'preview-mode': previewMode }" :style="cardStyle"
    @mousedown="handleMouseDown" @dblclick="handleDblClick">
    <el-card :class="['single-form-card', { 'editable': editable, 'no-style': !cardStyleOn }]">
      <div class="form-content">
        <!-- 标题 -->
        <div v-if="shouldShowTitle" class="form-title" :style="titleStyle">
          {{ form.title }}
        </div>

        <!-- 内容 -->
        <div v-if="shouldShowValue" class="form-value" :style="valueStyle">
          {{ form.value }}
        </div>

        <!-- 备注 -->
        <div v-if="shouldShowRemark" class="form-remark" :style="remarkStyle">
          {{ form.remark }}
        </div>

        <!-- 媒体内容（如果有的话） -->
        <div v-if="shouldShowMedia" class="form-media" :style="mediaStyle">
          <!-- 处理MediaUploader传递的对象 -->
          <template v-if="form.media && typeof form.media === 'object' && form.media.url">
            <!-- 图片预览 - MediaUploader对象 -->
            <img v-if="(form.mediaType === 'image' || !form.mediaType)" :src="form.media.url"
              :alt="form.title || 'Form media'" class="media-image" @error="handleMediaError" @load="handleMediaLoad" />
            <!-- 视频预览 - MediaUploader对象 -->
            <video v-else-if="form.mediaType === 'video'" :src="form.media.url" controls class="media-video"
              @error="handleMediaError" @loadeddata="handleVideoLoad">
              您的浏览器不支持视频播放。
            </video>
          </template>
          <!-- 处理字符串URL -->
          <template
            v-else-if="typeof (form.media || form.mediaPreviewUrl) === 'string' && (form.media || form.mediaPreviewUrl)">
            <!-- 图片预览 - 字符串URL -->
            <img v-if="(form.mediaType === 'image' || !form.mediaType)" :src="form.media || form.mediaPreviewUrl"
              :alt="form.title || 'Form media'" class="media-image" @error="handleMediaError" @load="handleMediaLoad" />
            <!-- 视频预览 - 字符串URL -->
            <video v-else-if="form.mediaType === 'video'" :src="form.media || form.mediaPreviewUrl" controls
              class="media-video" @error="handleMediaError" @loadeddata="handleVideoLoad">
              您的浏览器不支持视频播放。
            </video>
          </template>
          <!-- 处理File对象 -->
          <template v-else-if="form.media instanceof File">
            <!-- 图片预览 - File对象 -->
            <img v-if="(form.mediaType === 'image' || !form.mediaType)" :src="form.mediaPreviewUrl"
              :alt="form.title || 'Form media'" class="media-image" @error="handleMediaError" @load="handleMediaLoad" />
            <!-- 视频预览 - File对象 -->
            <video v-else-if="form.mediaType === 'video'" :src="form.mediaPreviewUrl" controls class="media-video"
              @error="handleMediaError" @loadeddata="handleVideoLoad">
              您的浏览器不支持视频播放。
            </video>
          </template>
          <div v-else class="media-unavailable">
            <p>无法预览媒体内容</p>
          </div>
        </div>
      </div>

      <!-- 编辑模式下的操作菜单 -->
      <div v-if="editable" class="form-actions">
        <el-dropdown @command="handleCommand">
          <el-button class="action-button" size="small">
            <el-icon>
              <MoreFilled />
            </el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="edit">
                <el-icon>
                  <Edit />
                </el-icon> {{ t('singleFormShow.edit') }}
              </el-dropdown-item>
              <el-dropdown-item command="delete">
                <el-icon>
                  <Delete />
                </el-icon> {{ t('singleFormShow.delete') }}
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>

      <!-- 调整大小手柄 -->
      <div v-if="editable" class="resize-handle" @mousedown="startResize"></div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'
import { MoreFilled, Edit, Delete } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { getMediaType, createMediaPreviewUrl } from '@/utils/mediaUtils'
import { t } from '@/utils/i18n'

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
  // 处理空值情况
  if (!props.form.media) return true;

  // 如果是对象，优先检查对象属性
  if (typeof props.form.media === 'object') {
    // MediaUploader传递的对象，包含mediaType属性
    if (props.form.media.mediaType) {
      return props.form.media.mediaType === 'image';
    }

    // MediaUploader传递的对象，包含isImage属性
    if (typeof props.form.media.isImage === 'boolean') {
      return props.form.media.isImage;
    }

    // 检查是否有File对象
    if (props.form.media instanceof File || (props.form.media.file && props.form.media.file instanceof File)) {
      const file = props.form.media instanceof File ? props.form.media : props.form.media.file;
      if (file.type) {
        return file.type.startsWith('image/');
      }
    }

    // 检查url属性
    if (props.form.media.url && typeof props.form.media.url === 'string') {
      return getMediaType(props.form.media.url) === 'image';
    }

    // 默认当作图片处理
    return true;
  }

  // 处理字符串
  if (typeof props.form.media === 'string') {
    return getMediaType(props.form.media) === 'image';
  }

  // 默认当作图片处理
  return true;
})

// 计算媒体预览URL
const mediaPreviewUrl = computed(() => {
  return createMediaPreviewUrl(props.form.media);
});

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
  return props.form?.title && props.form.title.trim() !== ''
})

// 是否显示内容
const shouldShowValue = computed(() => {
  return props.form?.value && props.form.value.trim() !== ''
})

// 是否显示备注
const shouldShowRemark = computed(() => {
  return props.form?.remark && props.form.remark.trim() !== ''
})

// 是否显示媒体
const shouldShowMedia = computed(() => {
  // 如果showMedia字段不存在，默认为true（向后兼容）
  const showMedia = props.form?.showMedia !== undefined ? props.form.showMedia : true;
  // 检查是否有媒体内容
  const hasMedia = !!(props.form?.media || props.form?.mediaPreviewUrl);
  return showMedia !== false && hasMedia;
})

// 处理命令
const handleCommand = (command) => {
  switch (command) {
    case 'edit':
      emit('edit', props.form)
      break
    case 'delete':
      emit('delete', props.form)
      break
  }
}

// 处理双击事件
const handleDblClick = (event) => {
  // 添加防抖，防止频繁双击触发
  if (performance.now() - (handleDblClick.lastCall || 0) < 300) {
    return;
  }
  handleDblClick.lastCall = performance.now();
  
  // 如果是视频表单，添加特殊处理
  if (props.form && props.form.type === 'video') {
    // 使用requestIdleCallback推迟执行，避免强制回流（如果浏览器支持）
    if (window.requestIdleCallback) {
      window.requestIdleCallback(() => {
        emit('dblclick', props.form, event);
      }, { timeout: 1000 });
    } else {
      // 降级到requestAnimationFrame
      requestAnimationFrame(() => {
        emit('dblclick', props.form, event);
      });
    }
  } else {
    emit('dblclick', props.form, event);
  }
}

// 处理鼠标按下事件（开始拖动）
const handleMouseDown = (event) => {
  // 如果点击的是调整大小手柄，则不触发拖动
  if (event.target.classList.contains('resize-handle')) {
    return;
  }

  if (props.editable) {
    emit('mousedown', props.form, event)

    // 开始拖动
    startDrag(event)
  }
}

// 开始拖动
const startDrag = (event) => {
  event.preventDefault()
  isDragging.value = true

  startX.value = event.clientX
  startY.value = event.clientY
  startLeft.value = props.position.x
  startTop.value = props.position.y

  document.addEventListener('mousemove', drag)
  document.addEventListener('mouseup', stopDrag)
}

// 拖动过程
const drag = (event) => {
  if (!isDragging.value) return

  const deltaX = event.clientX - startX.value
  const deltaY = event.clientY - startY.value

  let newLeft = startLeft.value + deltaX
  let newTop = startTop.value + deltaY

  // 限制拖动边界，不能拖出容器左边和上边
  newLeft = Math.max(0, newLeft)
  newTop = Math.max(0, newTop)

  // 传递表单ID和新位置
  emit('update-position', {
    formId: props.form.id,
    position: {
      x: Math.round(newLeft),
      y: Math.round(newTop)
    }
  })
}

// 停止拖动
const stopDrag = () => {
  isDragging.value = false
  document.removeEventListener('mousemove', drag)
  document.removeEventListener('mouseup', stopDrag)
}

// 开始调整大小
const startResize = (event) => {
  event.stopPropagation()
  event.preventDefault()
  isResizing.value = true

  startX.value = event.clientX
  startY.value = event.clientY
  startWidth.value = props.size.width
  startHeight.value = props.size.height

  document.addEventListener('mousemove', resize)
  document.addEventListener('mouseup', stopResize)
}

// 调整大小 - 添加防抖处理
let resizeTimer = null;
const resize = (event) => {
  if (!isResizing.value) return

  const deltaX = event.clientX - startX.value
  const deltaY = event.clientY - startY.value

  // 设置最小尺寸为50x50像素
  const newWidth = Math.max(50, startWidth.value + deltaX)
  const newHeight = Math.max(50, startHeight.value + deltaY)

  // 使用防抖避免频繁更新
  if (resizeTimer) {
    clearTimeout(resizeTimer)
  }

  resizeTimer = setTimeout(() => {
    // 传递表单ID和新尺寸
    emit('update-size', {
      formId: props.form.id,
      size: {
        width: Math.round(newWidth),
        height: Math.round(newHeight)
      }
    })
  }, 16) // 约60fps
}

// 停止调整大小
const stopResize = () => {
  isResizing.value = false
  document.removeEventListener('mousemove', resize)
  document.removeEventListener('mouseup', stopResize)

  // 清除防抖定时器
  if (resizeTimer) {
    clearTimeout(resizeTimer)
    resizeTimer = null
  }
}

// 处理媒体加载错误
const handleMediaError = (event) => {
  try {
    console.error('[SingleFormShow] 媒体加载失败:', event);

    // 检查是否是base64格式的视频
    if (props.form.media && typeof props.form.media === 'string') {
      if (props.form.media.startsWith('data:video/')) {
        ElMessage.info('视频为base64格式，可能需要较长时间加载');
      } else {
        ElMessage.warning('媒体加载失败');
      }
    } else {
      ElMessage.warning('媒体加载失败');
    }

    // 如果有错误对象，记录详细信息
    if (event && event.target && event.target.error) {
      const error = event.target.error;
      let errorDetails = `媒体加载错误 [${error.code}]`;

      switch (error.code) {
        case MediaError.MEDIA_ERR_ABORTED:
          errorDetails += ': 用户中止加载';
          break;
        case MediaError.MEDIA_ERR_NETWORK:
          errorDetails += ': 网络错误';
          break;
        case MediaError.MEDIA_ERR_DECODE:
          errorDetails += ': 解码错误';
          break;
        case MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED:
          errorDetails += ': 不支持的媒体格式';
          break;
        default:
          errorDetails += ': 未知错误';
      }

      console.error(`[SingleFormShow] ${errorDetails}`, error);
    }
  } catch (error) {
    console.error('[SingleFormShow] 处理媒体错误时出错:', error);
  }
}

// 处理媒体加载完成
const handleMediaLoad = (event) => {
  try {
    console.log('[SingleFormShow] 媒体加载成功');
  } catch (error) {
    console.error('[SingleFormShow] 处理媒体加载事件时出错:', error);
  }
}

// 处理视频加载完成
const handleVideoLoad = (event) => {
  try {
    console.log('[SingleFormShow] 视频加载成功');
  } catch (error) {
    console.error('[SingleFormShow] 处理视频加载事件时出错:', error);
  }
}

// 组件挂载前
onMounted(() => {
  console.log('SingleFormShow component mounted', props.form);
})

// 组件卸载前
onBeforeUnmount(() => {
  // 确保在组件卸载前清理事件监听器
  if (isResizing.value) {
    document.removeEventListener('mousemove', resize)
    document.removeEventListener('mouseup', stopResize)
    isResizing.value = false
  }

  if (isDragging.value) {
    document.removeEventListener('mousemove', drag)
    document.removeEventListener('mouseup', stopDrag)
    isDragging.value = false
  }

  // 清除防抖定时器
  if (resizeTimer) {
    clearTimeout(resizeTimer)
    resizeTimer = null
  }

  console.log('SingleFormShow component unmounting', props.form);
})
</script>

<style scoped>
.single-form-card-wrapper {
  cursor: move;
}

.single-form-card-wrapper.preview-mode {
  cursor: default;
}

.single-form-card {
  height: 100%;
}

.single-form-card.no-style {
  border: 1px solid #e0e0e0;
  border-radius: 4px;
}

.single-form-card.editable:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.form-content {
  height: calc(100% - 32px);
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.form-title {
  font-weight: bold;
  margin-bottom: 8px;
  word-break: break-word;
}

.form-value {
  flex: 1;
  word-break: break-word;
  overflow-y: auto;
  margin-bottom: 8px;
}

.form-remark {
  font-size: 0.9em;
  color: #666;
  word-break: break-word;
  margin-bottom: 8px;
}

.form-media {
  margin-top: 8px;
}

.media-image,
.media-video {
  max-width: 100%;
  max-height: 200px;
  border-radius: 4px;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
}

.media-unavailable {
  color: #999;
  font-style: italic;
  text-align: center;
  padding: 10px;
}

.form-actions {
  position: absolute;
  top: 8px;
  right: 8px;
  z-index: 10;
}

.action-button {
  opacity: 0;
  transition: opacity 0.2s ease;
}

.single-form-card-wrapper:hover .action-button {
  opacity: 1;
}

.resize-handle {
  position: absolute;
  bottom: 0;
  right: 0;
  width: 12px;
  height: 12px;
  cursor: nwse-resize;
  background: linear-gradient(-45deg, transparent 40%, #409eff 40%, #409eff 60%, transparent 60%);
  z-index: 20;
  transition: all 0.2s ease;
  opacity: 0.7;
}

.resize-handle:hover {
  transform: scale(1.2);
  background: linear-gradient(-45deg, transparent 30%, #409eff 30%, #409eff 70%, transparent 70%);
  opacity: 1;
}

.single-form-card-wrapper:hover .resize-handle {
  opacity: 1;
}
</style>