<template>
  <div class="media-uploader">
    <el-upload
      class="media-uploader"
      drag
      :on-success="handleSuccess"
      :show-file-list="false"
      :before-upload="beforeUpload"
      :auto-upload="false"
      :file-list="fileList"
      accept="image/*,video/*"
    >
      <i class="el-icon-upload"></i>
      <div class="el-upload__text">Drag or click to upload image/video</div>
    </el-upload>
    <div v-if="mediaUrl" class="media-preview">
      <img v-if="isImage" :src="mediaUrl" style="max-width:100%;max-height:120px;" />
      <video v-else controls :src="mediaUrl" style="max-width:100%;max-height:120px;" />
    </div>
  </div>
</template>

<script setup>
import { ref, watch } from 'vue'
import { ElMessage, ElMessageBox, ElDialog } from 'element-plus'
import { useI18n } from '../utils/i18n';

// 获取当前语言
const { t } = useI18n();

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

// 监听modelValue变化
watch(() => props.modelValue, v => {
  mediaUrl.value = v
  isImage.value = typeof v === 'string' ? v.match(/\.(jpg|jpeg|png|gif)$/i) : true
})

// 处理上传成功
function handleSuccess(res, file) {
  const url = URL.createObjectURL(file.raw)
  mediaUrl.value = url
  emit('update:modelValue', url)
}

// 处理上传前
function beforeUpload(file) {
  isImage.value = file.type.startsWith('image')
  const url = URL.createObjectURL(file)
  mediaUrl.value = url
  emit('update:modelValue', url)
  return false // Do not auto upload
}

// 资源清理
const cleanupTasks = []

// 添加内存泄漏检查
function checkMemoryUsage() {
  if (performance.memory) {
    const memoryUsage = performance.memory
    console.log('MediaUploader Memory Usage:', {
      usedJSHeapSize: formatBytes(memoryUsage.usedJSHeapSize),
      totalJSHeapSize: formatBytes(memoryUsage.totalJSHeapSize),
      jsHeapSizeLimit: formatBytes(memoryUsage.jsHeapSizeLimit)
    })
    
    // 如果内存使用超过阈值，触发警告
    if (memoryUsage.usedJSHeapSize / memoryUsage.jsHeapSizeLimit > 0.7) {
      console.warn('MediaUploader: Memory usage is high, consider optimizing')
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

// 添加内存泄漏检查
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

// 在组件挂载时初始化内存泄漏检查
onMounted(() => {
  setupMemoryLeakCheck()
})

// 在组件挂载时添加调试信息
onMounted(() => {
  console.log('MediaUploader component mounted', {
    modelValue: props.modelValue,
    editable: props.editable
  });
});

// 在组件卸载前执行清理
onBeforeUnmount(() => {
  // 执行资源清理
  cleanupResources()
})

// 在组件卸载前添加调试信息
onBeforeUnmount(() => {
  console.log('MediaUploader component about to be unmounted');
});
</script>

<style scoped>
.media-uploader {
  width: 100%;
}

.media-preview {
  margin-top: 8px;
}
</style>