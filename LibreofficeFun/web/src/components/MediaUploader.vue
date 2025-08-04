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
      <div class="el-upload__text">{{ t('mediaUploader.dragText') }}</div>
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
        {{ t('mediaUploader.previewUnavailable') }}
      </div>
      <el-button 
        type="danger" 
        size="small" 
        @click="clearMedia" 
        style="margin-top: 5px;"
      >
        {{ t('mediaUploader.clearMedia') }}
      </el-button>
    </div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Upload } from '@element-plus/icons-vue'
import { isSupportedMediaFormat, getMediaType, revokeBlobURL } from '@/utils/mediaUtils'
import { t } from '@/utils/i18n'

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
    revokeBlobURL(url);
  });
  createdUrls.value = [];
})

// 检查文件是否为图片
function isImageFile(file) {
  // 如果没有文件对象，返回true（当作图片处理）
  if (!file) return true
  
  // 通过MIME类型判断
  if (file.type) {
    return file.type.startsWith('image/')
  }
  
  // 如果没有MIME类型，通过文件名扩展名判断
  if (file.name) {
    return getMediaType(file.name) === 'image';
  }
  
  // 默认返回true（无法确定类型时当作图片处理）
  return true
}

// 处理上传成功
function handleSuccess(res, file) {
  console.log('MediaUploader: handleSuccess called', { res, file });
  
  // 获取原始文件
  const rawFile = file.raw || file
  
  // 检测媒体类型
  const isImg = isImageFile(rawFile)
  
  // 创建文件的临时URL
  const url = URL.createObjectURL(rawFile);
  
  // 记录创建的URL以便后续清理
  createdUrls.value.push(url)
  
  // 更新组件状态
  mediaUrl.value = url
  isImage.value = isImg
  
  // 通知父组件更新，传递包含URL和媒体类型的信息
  emit('update:modelValue', {
    url: url,
    mediaType: isImg ? 'image' : 'video',
    isImage: isImg,
    file: rawFile
  })
  console.log('MediaUploader: handleSuccess processed', { url, isImage: isImg });
}

// 处理上传前
function beforeUpload(file) {
  console.log('MediaUploader: beforeUpload called', { file });
  
  // 检查文件类型是否受支持
  if (!isSupportedMediaFormat(file)) {
    ElMessage.error(t('mediaUploader.unsupportedFormat'));
    return false;
  }
  
  // 检测媒体类型
  const isImg = isImageFile(file)
  
  // 创建文件的临时URL
  const url = URL.createObjectURL(file);
  
  // 记录创建的URL以便后续清理
  createdUrls.value.push(url)
  
  // 更新组件状态
  mediaUrl.value = url
  isImage.value = isImg
  
  // 通知父组件更新，传递包含URL和媒体类型的信息
  emit('update:modelValue', {
    url: url,
    mediaType: isImg ? 'image' : 'video',
    isImage: isImg
  })
  
  console.log('MediaUploader: beforeUpload processed', { file, url, isImage: isImg })
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
        ElMessage.error(t('mediaUploader.unsupportedFormat'));
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
    revokeBlobURL(mediaUrl.value);
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
  
  // 收集错误上下文信息
  const errorContext = {
    mediaUrl: mediaUrl.value,
    isImage: isImage.value,
    timestamp: new Date().toISOString(),
    userAgent: navigator.userAgent
  };
  
  let errorMessage = t('mediaUploader.loadError');
  
  // 标准媒体元素错误
  if (event && event.target && event.target.error) {
    const error = event.target.error;
    
    switch(error.code) {
      case MediaError.MEDIA_ERR_ABORTED:
        errorMessage = t('mediaUploader.errorAborted');
        break;
      case MediaError.MEDIA_ERR_NETWORK:
        errorMessage = t('mediaUploader.errorNetwork');
        break;
      case MediaError.MEDIA_ERR_DECODE:
        errorMessage = t('mediaUploader.errorDecode');
        errorContext.fileSize = event.target.size;
        break;
      case MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED:
        errorMessage = t('mediaUploader.errorNotSupported');
        break;
      default:
        errorMessage = `${t('mediaUploader.errorUnknown')} [${error.code}]`;
        break;
    }
    
    // 添加标准错误详细信息
    Object.assign(errorContext, {
      errorCode: error.code,
      errorMessage: error.message,
      src: event.target.src
    });
  } 
  // 通用错误对象
  else if (event && typeof event === 'object') {
    // 尝试提取错误信息
    const error = event.message ? event : 
      (event.target && event.target.error ? event.target.error : {});
      
    if (error.message) {
      errorMessage = `${t('mediaUploader.loadError')}: ${error.message}`;
    } else if (event.target && event.target.src) {
      const src = event.target.src;
      if (src.startsWith('blob:')) {
        errorMessage = t('mediaUploader.localFileError');
        
        // 清理无效的blob URL
        if (createdUrls.value.includes(src)) {
          revokeBlobURL(src);
          createdUrls.value = createdUrls.value.filter(url => url !== src);
        } else {
          errorMessage = `${t('mediaUploader.loadError')} [URL: ${src}]`;
        }
        
        errorContext.src = src;
      }
    }
    
    // 尝试序列化错误对象用于调试
    try {
      Object.assign(errorContext, {
        errorDetails: JSON.parse(JSON.stringify(error))
      });
    } catch (e) {
      errorContext.errorDetails = t('mediaUploader.errorSerializationFailed');
    }
  } 
  // 未知错误类型
  else {
    errorContext.errorDetails = t('mediaUploader.unknownErrorType');
  }
  
  // 记录详细的错误信息
  console.error('MediaUploader: Media load error', errorContext);
  
  // 显示用户友好的错误消息
  ElMessage.error(errorMessage);
}

// 监听modelValue变化
watch(() => props.modelValue, (newVal, oldVal) => {
  // 清理旧的URL对象（仅当旧值不在当前媒体URL中时）
  if (oldVal && createdUrls.value.includes(oldVal) && oldVal !== newVal) {
    revokeBlobURL(oldVal);
    createdUrls.value = createdUrls.value.filter(url => url !== oldVal)
  }
  
  // 处理不同类型的newVal
  if (newVal && typeof newVal === 'object') {
    // 如果是包含url的对象
    if ('url' in newVal) {
      mediaUrl.value = newVal.url
      // 使用统一的媒体类型判断函数
      isImage.value = getMediaType(newVal) === 'image';
    } 
    // 如果是包含File对象的格式
    else if ('file' in newVal && newVal.file instanceof File) {
      isImage.value = isImageFile(newVal.file)
    }
  } 
  // 处理字符串URL的情况
  else if (typeof newVal === 'string') {
    mediaUrl.value = newVal
    // 使用统一的媒体类型判断函数
    isImage.value = getMediaType(newVal) === 'image';
  } 
  // 处理File对象的情况
  else if (newVal instanceof File) {
    isImage.value = isImageFile(newVal)
  } 
  // 默认情况
  else {
    isImage.value = getMediaType(newVal) === 'image';
  }
  
  // 确保组件状态与父组件同步
  console.log('MediaUploader: modelValue changed', { 
    newVal, 
    oldVal, 
    mediaUrl: mediaUrl.value, 
    isImage: isImage.value,
    currentType: isImage.value ? 'image' : 'video'
  })
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