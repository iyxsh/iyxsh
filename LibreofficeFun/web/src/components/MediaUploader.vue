<template>
  <div class="media-uploader">
    <el-upload
      class="media-uploader"
      drag
      :on-success="handleSuccess"
      :on-change="handleChange"
      :show-file-list="false"
      :before-upload="beforeUpload"
      :auto-upload="false"
      :file-list="fileList"
      accept="image/*,video/*"
    >
      <el-icon class="el-upload__icon"><Upload /></el-icon>
      <div class="el-upload__text">Drag or click to upload image/video</div>
    </el-upload>
    <div v-if="mediaUrl" class="media-preview">
      <img 
        v-if="isImage && typeof mediaUrl === 'string'"
        :src="mediaUrl"
        style="max-width:100%;max-height:120px;" 
        @error="handleMediaError" 
        @load="handleMediaLoad"
      />
      <video 
        v-else-if="!isImage && typeof mediaUrl === 'string'"
        :src="mediaUrl"
        controls 
        style="max-width:100%;max-height:120px;" 
        @error="handleMediaError" 
        @loadeddata="handleVideoLoad"
      />
      <div v-else>
        无法预览媒体内容
      </div>
      <el-button 
        type="danger" 
        size="small" 
        @click="clearMedia" 
        style="margin-top: 5px;"
      >
        清除媒体
      </el-button>
    </div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Upload } from '@element-plus/icons-vue'
import { isSupportedMediaFormat } from '@/utils/mediaUtils'

// 添加组件加载调试信息
console.log('MediaUploader component initializing');

// 定义props
const props = defineProps({
  modelValue: [String, Object]
})

// 定义emits
const emit = defineEmits(['update:modelValue'])

// 响应式数据
const fileList = ref([])
const mediaUrl = ref(props.modelValue || '')
const isImage = ref(true)
// 保存创建的URL对象，以便后续清理
const createdUrls = ref([])
// 添加一个ref来跟踪组件是否已卸载
const isUnmounted = ref(false)

// 组件挂载时标记
onMounted(() => {
  isUnmounted.value = false
})

// 组件卸载时清理所有创建的blob URL
onUnmounted(() => {
  isUnmounted.value = true
  console.log('MediaUploader component unmounting, cleaning up blob URLs');
  createdUrls.value.forEach(url => {
    try {
      URL.revokeObjectURL(url);
    } catch (e) {
      console.warn('Failed to revoke blob URL on unmount:', url, e);
    }
  });
  createdUrls.value = [];
  
  // 清理可能的内存泄漏检查任务
  if (typeof cleanupTasks !== 'undefined' && Array.isArray(cleanupTasks)) {
    cleanupTasks.forEach(task => {
      if (typeof task === 'function') {
        try {
          task();
        } catch (e) {
          console.warn('Error executing cleanup task:', e);
        }
      }
    });
  }
})

// 检查URL是否为图片
function isImageUrl(url) {
  // 通过文件扩展名判断
  if (typeof url === 'string') {
    // 处理可能带查询参数的URL
    const cleanUrl = url.split('?')[0];
    const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
    const videoExtensions = /\.(mp4|webm|ogg|avi|mov|wmv|flv|mkv)$/i;
    
    if (imageExtensions.test(cleanUrl)) {
      return true;
    }
    if (videoExtensions.test(cleanUrl)) {
      return false;
    }
  }
  // 默认返回true（当作图片处理）
  return true;
}

// 检查文件是否为图片
function isImageFile(file) {
  // 通过MIME类型判断
  if (file && file.type) {
    return file.type.startsWith('image/');
  }
  // 如果没有MIME类型，通过文件名判断
  if (file && file.name) {
    return isImageUrl(file.name);
  }
  // 默认返回true（当作图片处理）
  return true;
}

// 处理上传成功
function handleSuccess(res, file) {
  console.log('MediaUploader: handleSuccess called', { res, file });
  
  // 创建文件的临时URL
  const url = URL.createObjectURL(file.raw)
  
  // 记录创建的URL以便后续清理
  createdUrls.value.push(url)
  
  // 更新组件状态
  mediaUrl.value = url
  isImage.value = isImageFile(file.raw)
  
  // 通知父组件更新，传递包含URL和媒体类型的信息
  emit('update:modelValue', {
    url: url,
    mediaType: isImage.value ? 'image' : 'video',
    isImage: isImage.value,
    file: file.raw
  })
  console.log('MediaUploader: handleSuccess processed', { url, isImage: isImage.value });
}

// 处理上传前
function beforeUpload(file) {
  console.log('MediaUploader: beforeUpload called', { file });
  
  isImage.value = isImageFile(file)
  const url = URL.createObjectURL(file)
  // 记录创建的URL以便后续清理
  createdUrls.value.push(url)
  mediaUrl.value = url
  // 通知父组件更新，传递包含URL和媒体类型的信息
  emit('update:modelValue', {
    url: url,
    mediaType: isImage.value ? 'image' : 'video',
    isImage: isImage.value
  })
  console.log('MediaUploader: beforeUpload processed', { file, url, isImage: isImage.value })
  return false // Do not auto upload
}

// 处理文件变化
function handleChange(file, fileList) {
  console.log('MediaUploader: handleChange called', { file, fileList });
  
  // 如果有文件，处理第一个文件
  if (fileList && fileList.length > 0) {
    const currentFile = fileList[0].raw || fileList[0];
    if (currentFile) {
      // 检查文件类型是否受支持
      if (!isSupportedMediaFormat(currentFile)) {
        ElMessage.error('不支持的媒体格式');
        return;
      }
      
      beforeUpload(currentFile);
    }
  }
}

// 清除媒体
function clearMedia() {
  // 清理创建的URL对象
  if (mediaUrl.value && createdUrls.value.includes(mediaUrl.value)) {
    try {
      URL.revokeObjectURL(mediaUrl.value)
    } catch (e) {
      console.warn('Failed to revoke blob URL:', mediaUrl.value, e)
    }
    createdUrls.value = createdUrls.value.filter(url => url !== mediaUrl.value)
  }
  
  mediaUrl.value = ''
  isImage.value = true
  emit('update:modelValue', '')
}

// 处理媒体加载完成
function handleMediaLoad(event) {
  // 检查组件是否已卸载
  if (isUnmounted.value) {
    return
  }
  
  console.log('MediaUploader: Media loaded successfully', { 
    event, 
    mediaUrl: mediaUrl.value,
    isImage: isImage.value
  });
}

// 处理视频加载完成
function handleVideoLoad(event) {
  // 检查组件是否已卸载
  if (isUnmounted.value) {
    return
  }
  
  console.log('MediaUploader: Video loaded successfully', { 
    event, 
    mediaUrl: mediaUrl.value,
    isImage: isImage.value
  });
}

// 处理媒体错误
function handleMediaError(event) {
  // 检查组件是否已卸载
  if (isUnmounted.value) {
    return
  }
  
  console.error('MediaUploader: Media load error', { 
    event, 
    mediaUrl: mediaUrl.value,
    isImage: isImage.value
  });
  
  let errorMessage = '媒体加载失败';
  
  // 处理不同类型的媒体错误
  if (event && event.target && event.target.error) {
    const error = event.target.error;
    
    switch(error.code) {
      case MediaError.MEDIA_ERR_ABORTED:
        errorMessage = '用户取消了媒体加载';
        break;
      case MediaError.MEDIA_ERR_NETWORK:
        errorMessage = '网络错误导致媒体加载失败';
        break;
      case MediaError.MEDIA_ERR_DECODE:
        errorMessage = '媒体解码失败（文件可能已损坏）';
        break;
      case MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED:
        errorMessage = '不支持的媒体格式';
        break;
      default:
        errorMessage = `未知媒体错误 [${error.code}]`;
        break;
    }
    
    // 添加更多调试信息
    console.error('MediaUploader: Error details:', {
      code: error.code,
      message: error.message,
      src: event.target.src,
      mediaUrl: mediaUrl.value,
      isImage: isImage.value
    });
  } else if (event && event.message) {
    // 处理其他类型的错误事件
    console.error('MediaUploader: Error message:', event.message);
    errorMessage = `媒体加载错误: ${event.message}`;
  } else if (event && event.target && event.target.src) {
    // 处理一般加载错误
    const src = event.target.src;
    if (src.startsWith('blob:')) {
      errorMessage = '本地媒体文件加载失败';
    } else {
      errorMessage = `媒体加载失败 [URL: ${src}]`;
    }
  }
  
  ElMessage.error(errorMessage);
}

// 监听modelValue变化
watch(() => props.modelValue, (newVal, oldVal) => {
  // 清理旧的URL对象（仅当旧值不在当前媒体URL中时）
  if (oldVal && createdUrls.value.includes(oldVal) && oldVal !== newVal) {
    try {
      URL.revokeObjectURL(oldVal)
    } catch (e) {
      console.warn('Failed to revoke blob URL:', oldVal, e)
    }
    createdUrls.value = createdUrls.value.filter(url => url !== oldVal)
  }
  
  // 处理不同类型的newVal
  if (newVal && typeof newVal === 'object' && newVal.url) {
    // 对象格式 { url, mediaType }
    mediaUrl.value = newVal.url
    isImage.value = newVal.mediaType ? newVal.mediaType === 'image' : isImageUrl(newVal.url)
  } else if (newVal && typeof newVal === 'string') {
    // 字符串格式 URL
    mediaUrl.value = newVal
    // 检查是否是blob URL
    if (newVal.startsWith('blob:')) {
      // 对于blob URL，我们无法从URL本身判断类型
      // 保持当前的isImage值，除非有明确的指示
      // 默认情况下，我们假设它与之前相同或为图片（向后兼容）
    } else {
      // 对于普通URL，使用扩展名判断
      isImage.value = isImageUrl(newVal)
    }
  } else if (newVal instanceof File) {
    // File对象
    isImage.value = isImageFile(newVal)
  } else if (newVal && typeof newVal === 'object' && newVal.file) {
    // 包含文件对象的格式 { file }
    isImage.value = isImageFile(newVal.file)
  } else {
    // 默认情况
    mediaUrl.value = newVal || ''
    isImage.value = newVal ? (typeof newVal === 'string' ? isImageUrl(newVal) : isImageFile(newVal)) : true
  }
  
  // 确保组件状态与父组件同步
  console.log('MediaUploader: modelValue changed', { newVal, oldVal, mediaUrl: mediaUrl.value, isImage: isImage.value })
})
</script>

<style scoped>
.media-uploader {
  width: 100%;
}

.media-preview {
  margin-top: 8px;
}

.el-upload__icon {
  font-size: 67px;
  color: #c0c4cc;
  margin: 40px 0 16px;
  line-height: 50px;
}
</style>