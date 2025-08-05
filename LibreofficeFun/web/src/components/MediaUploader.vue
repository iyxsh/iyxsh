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
    
    <!-- 媒体预览区域 -->
    <div v-if="mediaUrl && showPreview" class="media-preview">
      <!-- 图片预览 -->
      <div v-if="isImageValue(mediaUrl)" class="image-preview">
        <img 
          :src="mediaUrl" 
          alt="Image preview"
          class="preview-image"
          @error="handlePreviewError"
          @load="handlePreviewLoad"
        />
      </div>
      
      <!-- 视频预览 -->
      <div v-else class="video-preview">
        <video 
          :src="mediaUrl"
          class="preview-video"
          controls
          preload="metadata"
          @error="handlePreviewError"
          @loadeddata="handleVideoLoaded"
        >
          您的浏览器不支持视频播放
        </video>
      </div>
      
      <!-- 清除按钮 -->
      <div class="preview-actions">
        <el-button 
          type="danger" 
          size="small" 
          @click="clearMedia"
        >
          {{ t('mediaUploader.clearMedia') }}
        </el-button>
      </div>
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
  modelValue: [String, Object],
  showPreview: {
    type: Boolean,
    default: true
  }
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
    return file.type.startsWith('image/');
  }
  
  // 如果没有MIME类型，通过文件名判断
  if (file.name) {
    const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
    return imageExtensions.test(file.name);
  }
  
  // 默认当作图片处理
  return true;
}

// 准确判断媒体URL是否为图片
const isImageValue = (url) => {
  // 如果url为空或不是字符串，返回默认值true（当作图片处理）
  if (!url || typeof url !== 'string') return true;
  
  // 检查base64数据URL
  if (url.startsWith('data:')) {
    return url.startsWith('data:image/');
  }
  
  // 通过文件扩展名判断
  const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
  const videoExtensions = /\.(mp4|webm|ogg|avi|mov|wmv|flv|mkv)$/i;
  
  // 检查扩展名
  if (imageExtensions.test(url)) {
    return true; // 匹配图片扩展名
  }
  if (videoExtensions.test(url)) {
    return false; // 匹配视频扩展名
  }
  
  // 未知扩展名，返回当前isImage值作为默认值
  return isImage.value;
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

// 处理预览加载错误
const handlePreviewError = (event) => {
  console.error('Media preview load error:', event);
  ElMessage.error(t('mediaUploader.loadError'));
}

// 处理图片预览加载完成
const handlePreviewLoad = (event) => {
  console.log('Image preview loaded successfully');
}

// 处理视频预览加载完成
const handleVideoLoaded = (event) => {
  console.log('Video preview loaded successfully');
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
      isImage.value = isImageValue(newVal.url);
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
    isImage.value = isImageValue(newVal);
  } 
  // 处理File对象的情况
  else if (newVal instanceof File) {
    isImage.value = isImageFile(newVal)
  } 
  // 默认情况
  else {
    isImage.value = isImageValue(newVal);
  }
  
  // 确保组件状态与父组件同步
  console.log('MediaUploader: modelValue changed', { 
    newVal, 
    mediaUrl: mediaUrl.value, 
    isImage: isImage.value 
  })
}, { immediate: true });
</script>

<style scoped>
.media-uploader {
  width: 100%;
}

.media-preview {
  margin-top: 15px;
  border: 1px dashed #dcdfe6;
  border-radius: 6px;
  padding: 10px;
  background-color: #f5f7fa;
}

.image-preview,
.video-preview {
  display: flex;
  justify-content: center;
  align-items: center;
  margin-bottom: 10px;
}

.preview-image {
  max-width: 100%;
  max-height: 200px;
  border-radius: 4px;
  object-fit: contain;
}

.preview-video {
  max-width: 100%;
  max-height: 200px;
  border-radius: 4px;
}

.preview-actions {
  display: flex;
  justify-content: center;
}

.el-upload__icon {
  font-size: 67px;
  color: #c0c4cc;
  margin: 40px 0 16px;
  line-height: 50px;
}

:deep(.el-upload-dragger) {
  width: 100%;
}
</style>