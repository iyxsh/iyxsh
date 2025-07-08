<template>
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
</template>

<script setup>
import { ref, watch } from 'vue'
const props = defineProps({
  modelValue: [String, Object]
})
const emit = defineEmits(['update:modelValue'])
const fileList = ref([])
const mediaUrl = ref(props.modelValue || '')
const isImage = ref(true)
watch(() => props.modelValue, v => {
  mediaUrl.value = v
  isImage.value = typeof v === 'string' ? v.match(/\.(jpg|jpeg|png|gif)$/i) : true
})
function handleSuccess(res, file) {
  const url = URL.createObjectURL(file.raw)
  mediaUrl.value = url
  emit('update:modelValue', url)
}
function beforeUpload(file) {
  isImage.value = file.type.startsWith('image')
  const url = URL.createObjectURL(file)
  mediaUrl.value = url
  emit('update:modelValue', url)
  return false // Do not auto upload
}
</script>

<style scoped>
.media-uploader {
  width: 100%;
}
.media-preview {
  margin-top: 8px;
}
</style>
