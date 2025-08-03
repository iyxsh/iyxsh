<template>
  <div class="form-editor">
    <div class="form-editor-header">
      <h3>{{ isEdit ? '编辑表单' : '新建表单' }}</h3>
      <el-button @click="cancel" size="small" type="default" icon="Close">取消</el-button>
    </div>

    <el-form :model="formState" label-position="top" :rules="rules" ref="formRef" class="form-content"
      @submit.prevent="submitForm">
      <!-- 标题字段 -->
      <el-form-item label="标题" prop="title">
        <div class="element-container">
          <el-input v-model="formState.title" placeholder="请输入表单标题" :maxlength="50" show-word-limit />
        </div>
      </el-form-item>

      <!-- 内容字段 -->
      <el-form-item label="内容" prop="value">
        <div class="element-container">
          <el-input v-model="formState.value" type="textarea" placeholder="请输入表单内容"
            :autosize="{ minRows: 3, maxRows: 6 }" :maxlength="200" show-word-limit />
        </div>
      </el-form-item>

      <!-- 备注字段 -->
      <el-form-item label="备注" prop="remark">
        <div class="element-container">
          <el-input v-model="formState.remark" type="textarea" placeholder="请输入备注信息（选填）"
            :autosize="{ minRows: 2, maxRows: 4 }" :maxlength="100" show-word-limit />
        </div>
      </el-form-item>

      <!-- 媒体字段 -->
      <el-form-item label="媒体" prop="media">
        <div class="element-container">
          <MediaUploader 
            v-model="formState.media" 
            @update:modelValue="handleMediaUpdate" 
          />
        </div>
        <!-- 媒体预览 -->
        <div v-if="shouldShowMedia" class="media-preview">
          <el-card 
            :body-style="{ padding: '15px' }" 
            style="margin-top: 12px; border: 1px solid #e8e8e8; border-radius: 6px;"
          >
            <div style="display: flex; align-items: center; margin-bottom: 8px;">
              <el-icon name="Picture" v-if="formState.mediaType === 'image' || !formState.mediaType" />
              <el-icon name="VideoCamera" v-else />
              <span style="margin-left: 8px; font-weight: 500;">媒体预览</span>
            </div>
            <!-- 处理MediaUploader传递的对象 -->
            <template v-if="formState.media && typeof formState.media === 'object' && formState.media.url">
              <!-- 图片预览 - MediaUploader对象 -->
              <img
                v-if="(formState.mediaType === 'image' || !formState.mediaType)"
                :src="formState.media.url"
                :alt="formState.title || 'Media preview'"
                style="width: 100%; border-radius: 4px; box-shadow: 0 2px 6px rgba(0,0,0,0.1);"
                @error="handleMediaPreviewError"
                @load="handleMediaPreviewLoad"
              />
              <!-- 视频预览 - MediaUploader对象 -->
              <video
                v-else
                :src="formState.media.url"
                controls
                style="width: 100%; border-radius: 4px; box-shadow: 0 2px 6px rgba(0,0,0,0.1);"
                @error="handleMediaPreviewError"
                @loadeddata="handleVideoLoaded"
              ></video>
            </template>
            <!-- 处理字符串URL -->
            <template v-else-if="typeof (formState.media || formState.mediaPreviewUrl) === 'string' && (formState.media || formState.mediaPreviewUrl)">
              <!-- 图片预览 - 字符串URL -->
              <img
                v-if="(formState.mediaType === 'image' || !formState.mediaType)"
                :src="formState.media || formState.mediaPreviewUrl" 
                :alt="formState.title || 'Media preview'"
                style="width: 100%; border-radius: 4px; box-shadow: 0 2px 6px rgba(0,0,0,0.1);"
                @error="handleMediaPreviewError"
                @load="handleMediaPreviewLoad"
              />
              <!-- 视频预览 - 字符串URL -->
              <video
                v-else
                :src="formState.media || formState.mediaPreviewUrl"
                controls
                style="width: 100%; border-radius: 4px; box-shadow: 0 2px 6px rgba(0,0,0,0.1);"
                @error="handleMediaPreviewError"
                @loadeddata="handleVideoLoaded"
              ></video>
            </template>
            <!-- 处理base64数据 -->
            <template v-else-if="typeof formState.media === 'string' && formState.media.startsWith('data:')">
              <!-- 图片预览 - base64数据 -->
              <img
                v-if="(formState.mediaType === 'image' || !formState.mediaType) && formState.media.startsWith('data:image/')"
                :src="formState.media"
                :alt="formState.title || 'Media preview'"
                style="width: 100%; border-radius: 4px; box-shadow: 0 2px 6px rgba(0,0,0,0.1);"
                @error="handleMediaPreviewError"
                @load="handleMediaPreviewLoad"
              />
              <!-- 视频预览 - base64数据 -->
              <video
                v-else-if="formState.mediaType === 'video' && formState.media.startsWith('data:video/')"
                :src="formState.media"
                controls
                style="width: 100%; border-radius: 4px; box-shadow: 0 2px 6px rgba(0,0,0,0.1);"
                @error="handleMediaPreviewError"
                @loadeddata="handleVideoLoaded"
              ></video>
            </template>
            <div v-else-if="formState.media || formState.mediaPreviewUrl">
              <!-- 当有媒体内容但条件不匹配时 -->
              <p>无法预览媒体内容</p>
            </div>
          </el-card>
        </div>
      </el-form-item>

      <!-- 可见性设置 -->
      <el-collapse v-model="activeCollapse" class="visibility-settings-panel">
        <el-collapse-item title="可见性设置" name="visibility">
          <div class="visibility-grid">
            <el-checkbox v-model="formState.showTitle">显示标题</el-checkbox>
            <el-checkbox v-model="formState.showValue">显示内容</el-checkbox>
            <el-checkbox v-model="formState.showRemark">显示备注</el-checkbox>
            <el-checkbox v-model="formState.showMedia">显示媒体</el-checkbox>
          </div>
        </el-collapse-item>
      </el-collapse>

      <!-- 样式管理器 -->
      <el-collapse v-model="activeCollapse" class="style-settings-panel">
        <el-collapse-item title="样式设置" name="style">
          <StyleManager v-model="formState.style" />
        </el-collapse-item>
      </el-collapse>

      <!-- 元素样式管理器 -->
      <el-collapse v-model="activeCollapse" class="element-style-settings-panel">
        <el-collapse-item title="元素样式设置" name="elementStyle">
          <div class="element-style-section">
            <h4>标题样式</h4>
            <ElementStylePanel 
              v-model="formState.elementStyles.title" 
              element-name="title"
              :form-data="formState"
            />
            
            <h4>内容样式</h4>
            <ElementStylePanel 
              v-model="formState.elementStyles.value" 
              element-name="value"
              :form-data="formState"
            />
            
            <h4>备注样式</h4>
            <ElementStylePanel 
              v-model="formState.elementStyles.remark" 
              element-name="remark"
              :form-data="formState"
            />
            
            <h4>媒体样式</h4>
            <ElementStylePanel 
              v-model="formState.elementStyles.media" 
              element-name="media"
              :form-data="formState"
            />
          </div>
        </el-collapse-item>
      </el-collapse>

    <!-- 操作按钮 -->
    <div class="form-actions">
      <el-button @click="cancel" type="default">取消</el-button>
      <el-button @click="saveDraft" type="info" :loading="isSaving">保存草稿</el-button>
      <el-button @click="submitForm" type="primary" :loading="isSaving">保存</el-button>
    </div>
  </el-form>
</div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, nextTick, watch, onUnmounted, onBeforeUnmount } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Close, Setting } from '@element-plus/icons-vue'
import StyleManager from './StyleManager.vue'
import MediaUploader from './MediaUploader.vue'
import SingleFormShow from './SingleFormShow.vue'
import errorLogService from '../services/errorLogService'
import { t as i18nTrans } from '../utils/i18n'

// 定义props
const props = defineProps({
  form: {
    type: Object,
    default: () => ({})
  },
  isEdit: {
    type: Boolean,
    default: false
  }
})

// 定义事件发射器
const emit = defineEmits(['save', 'cancel', 'error'])

// 响应式数据
const formRef = ref(null)
const isEdit = ref(false)
const isSaving = ref(false)
const activeCollapse = ref(['visibility', 'style', 'elementStyle'])
// 添加一个ref来跟踪组件是否已卸载
const isUnmounted = ref(false)

// 组件卸载前的清理工作
onBeforeUnmount(() => {
  isUnmounted.value = true
})

// 预览模式状态
const isPreviewMode = ref(false)

// 预览相关的SimpleCardConverter数据
const previewCardGroups = ref([])
const previewCardGroupStyles = ref({})
const previewCardRowStyles = ref({})
const previewCardStyles = ref({})
const previewGlobalTextStyles = ref({})

// 默认样式
const defaultStyle = () => ({
  backgroundColor: '#ffffff',
  color: '#333333',
  borderWidth: 1,
  borderStyle: 'solid',
  borderColor: '#e4e7ed',
  borderRadius: 4,
  fontSize: 14,
  padding: 12,
  hasShadow: true
})

// 默认的元素样式设置
const defaultElementStyle = () => ({
  enabled: false,
  color: '#333333',
  fontSize: 14,
  fontWeight: 'normal'
})

// 确保元素样式数据完整性的函数
const ensureElementStyleIntegrity = (style) => {
  const defaultStyle = defaultElementStyle()
  const result = { ...style }
  
  // 确保所有必要字段都存在
  Object.keys(defaultStyle).forEach(key => {
    if (!(key in result)) {
      console.warn(`Missing key "${key}" in element style, using default value`)
      result[key] = defaultStyle[key]
    }
  })
  
  return result
}


// 打开统一样式设置面板

// 关闭统一样式设置面板

// 应用统一样式到所有启用的元素样式

// 表单状态
/** @type {Form} */
const formState = reactive({
  id: props.form?.id || `form-${Date.now()}-${Math.random().toString(36).substring(2, 9)}`,
  title: props.isEdit ? (props.form?.title || '') : '',
  value: props.isEdit ? (props.form?.value || '') : '',
  remark: props.isEdit ? (props.form?.remark || '') : '',
  media: props.isEdit ? (props.form?.media || '') : '',
  mediaType: props.isEdit ? (props.form?.mediaType || 'image') : 'image',
  showTitle: props.isEdit ? (props.form?.showTitle !== false) : true,
  showValue: props.isEdit ? (props.form?.showValue !== false) : true,
  showRemark: props.isEdit ? (props.form?.showRemark !== false) : true,
  showMedia: props.isEdit ? (props.form?.showMedia !== false) : true,
  style: { 
    ...defaultStyle(), 
    ...(props.isEdit && props.form?.style ? props.form.style : {}) 
  },
  // 添加元素特定样式设置
  elementStyles: {
    title: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.title ? props.form.elementStyles.title : {})
    }),
    value: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.value ? props.form.elementStyles.value : {})
    }),
    remark: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.remark ? props.form.elementStyles.remark : {})
    }),
    media: ensureElementStyleIntegrity({
      ...defaultElementStyle(),
      ...(props.isEdit && props.form?.elementStyles?.media ? props.form.elementStyles.media : {})
    })
  },
  position: { 
    x: props.isEdit ? (props.form?.position?.x || 20) : 20, 
    y: props.isEdit ? (props.form?.position?.y || 20) : 20 
  },
  size: { 
    width: props.isEdit ? (props.form?.size?.width || 200) : 200, 
    height: props.isEdit ? (props.form?.size?.height || 100) : 100 
  },
  // 添加媒体预览URL字段
  mediaPreviewUrl: props.isEdit ? (
    props.form?.mediaPreviewUrl || 
    (typeof props.form?.media === 'string' ? props.form.media : '') 
  ) : ''
})

// 验证规则
const rules = {
  title: [
    { required: true, message: '请输入表单标题', trigger: 'blur' },
    { min: 1, max: 50, message: '长度在1到50个字符之间', trigger: 'blur' }
  ],
  value: [
    { required: true, message: '请输入表单内容', trigger: 'blur' },
    { min: 1, max: 200, message: '长度在1到200个字符之间', trigger: 'blur' }
  ],
  remark: [
    { max: 100, message: '长度不能超过100个字符', trigger: 'blur' }
  ],
  media: [
    { validator: (rule, value, callback) => {
      if (formState.showMedia && value) {
        // 处理MediaUploader传递的对象
        let mediaUrl = '';
        if (typeof value === 'string') {
          mediaUrl = value;
        } else if (value && typeof value === 'object' && value.url) {
          // MediaUploader传递的对象
          mediaUrl = value.url;
        } else if (value && typeof value === 'object' && value instanceof File) {
          // File对象，总是有效
          callback();
          return;
        }
        
        // 支持本地URL(blob:)和普通URL
        if (mediaUrl && (
          mediaUrl.startsWith('blob:') || 
          /^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$/.test(mediaUrl)
        )) {
          callback();
        } else {
          callback(new Error('请输入有效的URL'));
        }
      } else {
        callback();
      }
    }, trigger: 'blur' }
  ]
}

// 预览表单数据
const previewForm = computed(() => {
  // 确定要使用的媒体URL
  let mediaUrl = '';
  if (formState.media) {
    mediaUrl = formState.media;
  } else if (formState.mediaPreviewUrl) {
    mediaUrl = formState.mediaPreviewUrl;
  }
  
  // 确定媒体类型
  let mediaType = formState.mediaType || 'image';
  // 只有当mediaUrl是字符串时才检查是否为blob URL
  if (mediaUrl && typeof mediaUrl === 'string' && mediaUrl.startsWith('blob:')) {
    // 对于blob URL，使用本地判断的类型
    mediaType = isImageMedia(mediaUrl) ? 'image' : 'video';
  }
  
  return {
    ...formState,
    id: formState.id || `preview-${Date.now()}`,
    title: formState.title || '预览标题',
    value: formState.value || '预览内容',
    remark: formState.remark || '',
    media: mediaUrl,  // 确保使用处理后的媒体URL
    mediaPreviewUrl: formState.mediaPreviewUrl || mediaUrl,  // 确保预览URL也正确设置
    mediaType: mediaType,
    showTitle: formState.showTitle,
    showValue: formState.showValue,
    showRemark: formState.showRemark,
    showMedia: formState.showMedia,
    style: { ...formState.style },
    elementStyles: { ...formState.elementStyles },
    position: { ...formState.position },
    size: { ...formState.size }
  }
})

// 切换预览模式
const togglePreview = () => {
  isPreviewMode.value = !isPreviewMode.value
}

// 处理媒体更新
const handleMediaUpdate = (newMedia) => {
  console.log('FormEditor: Media updated:', newMedia);
  
  // 直接更新媒体字段
  formState.media = newMedia;
  
  // 如果是MediaUploader传递的对象（包含url和mediaType属性）
  if (newMedia && typeof newMedia === 'object' && newMedia.url) {
    formState.mediaPreviewUrl = newMedia.url;
    // 根据mediaType字段设置mediaType
    formState.mediaType = newMedia.mediaType || 'image';
  }
  // 如果是字符串URL，直接使用
  else if (typeof newMedia === 'string') {
    // 如果是空字符串，则清除媒体
    if (newMedia === '') {
      formState.mediaPreviewUrl = '';
      formState.mediaType = 'image';
    } else {
      formState.mediaPreviewUrl = newMedia;
      // 自动更新mediaType
      // 检查是否是blob URL
      if (newMedia.startsWith('blob:')) {
        // 对于blob URL，我们需要保留之前设置的mediaType
        // 如果没有设置，则默认为image（向后兼容）
        if (!formState.mediaType) {
          formState.mediaType = 'image';
        }
      } else {
        // 对于普通URL，根据扩展名判断
        formState.mediaType = isImageMedia(newMedia) ? 'image' : 'video';
      }
    }
  } 
  // 如果是File对象，创建预览URL
  else if (newMedia instanceof File) {
    const url = URL.createObjectURL(newMedia);
    formState.mediaPreviewUrl = url;
    // 根据文件类型设置mediaType
    formState.mediaType = newMedia.type.startsWith('image/') ? 'image' : 
                         newMedia.type.startsWith('video/') ? 'video' : 'image';
  } 
  // 其他情况（如null或undefined），清除预览URL
  else {
    formState.mediaPreviewUrl = '';
    formState.mediaType = 'image';
  }
  
  console.log('FormEditor: handleMediaUpdate processed', { 
    media: formState.media, 
    mediaPreviewUrl: formState.mediaPreviewUrl, 
    mediaType: formState.mediaType 
  });
}

// 处理媒体文件变化，生成本地预览
watch(() => formState.media, (newMedia) => {
  if (newMedia) {
    // 如果是本地文件对象，则创建预览URL
    if (typeof newMedia === 'object' && newMedia instanceof File) {
      const url = URL.createObjectURL(newMedia)
      formState.mediaPreviewUrl = url
    } else {
      // 否则使用现有URL作为预览URL
      formState.mediaPreviewUrl = newMedia
    }
    
    // 同时更新mediaType字段
    if (typeof newMedia === 'string') {
      // 如果是空字符串，则清除媒体
      if (newMedia === '') {
        formState.mediaType = 'image';
      } else {
        // 检查是否是blob URL
        if (newMedia.startsWith('blob:')) {
          // 对于blob URL，我们保留已设置的mediaType
          // 只有当mediaType未设置时才设置默认值
          if (!formState.mediaType) {
            formState.mediaType = 'image'; // 默认为image以保持向后兼容
          }
        } else {
          // 对于普通URL，根据扩展名判断
          if (isImageMedia(newMedia)) {
            formState.mediaType = 'image';
          } else {
            formState.mediaType = 'video';
          }
        }
      }
    } else if (typeof newMedia === 'object' && newMedia instanceof File) {
      // File对象的情况
      if (newMedia.type.startsWith('image/')) {
        formState.mediaType = 'image';
      } else if (newMedia.type.startsWith('video/')) {
        formState.mediaType = 'video';
      }
    }
  } else {
    // 清理预览URL（如果是blob URL）
    if (formState.mediaPreviewUrl && 
        typeof formState.mediaPreviewUrl === 'string' && 
        formState.mediaPreviewUrl.startsWith('blob:')) {
      try {
        URL.revokeObjectURL(formState.mediaPreviewUrl);
        console.log('Revoked blob URL:', formState.mediaPreviewUrl);
      } catch (e) {
        console.warn('Failed to revoke blob URL:', formState.mediaPreviewUrl, e);
      }
    }
    formState.mediaPreviewUrl = null
    formState.mediaType = 'image';
  }
}, { immediate: true })

// 当mediaPreviewUrl变化时，更新media字段
watch(() => formState.mediaPreviewUrl, (newPreviewUrl) => {
  // 只有当media为空且有预览URL时才更新media字段
  if (newPreviewUrl && !formState.media) {
    formState.media = newPreviewUrl
  }
  
  // 同时更新mediaType字段
  if (newPreviewUrl) {
    if (isImageMedia(newPreviewUrl)) {
      formState.mediaType = 'image';
    } else {
      formState.mediaType = 'video';
    }
  }
})

// 检查是否为本地媒体URL
const isLocalMediaUrl = (url) => {
  if (typeof url !== 'string') return false;
  return url.startsWith('blob:');
};

// 判断媒体是否为图片
const isImageMedia = (url) => {
  // 如果url为空或不是字符串，返回默认值true（当作图片处理）
  if (!url || typeof url !== 'string') return true;
  
  // 通过文件扩展名判断
  const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
  const videoExtensions = /\.(mp4|webm|ogg|avi|mov|wmv|flv|mkv)$/i;
  
  // 检查是否是blob URL
  if (url.startsWith('blob:')) {
    // 对于blob URL，我们检查媒体类型字段
    if (formState.media && typeof formState.media === 'object' && formState.media.mediaType) {
      return formState.media.mediaType === 'image';
    }
    if (formState.mediaType) {
      return formState.mediaType === 'image';
    }
    // 如果没有mediaType，尝试从URL中获取类型信息
    // blob URL格式: blob:http://localhost:3000/74f5d871-0d44-4c68-ae32-08fb5b667fc0
    // 我们无法直接从blob URL判断类型，所以返回true作为默认值（当作图片处理）
    return true;
  }
  
  // 检查base64数据URL
  if (url.startsWith('data:image/')) {
    return true;
  }
  if (url.startsWith('data:video/')) {
    return false;
  }
  
  // 检查扩展名
  if (imageExtensions.test(url)) {
    return true; // 匹配图片扩展名
  }
  if (videoExtensions.test(url)) {
    return false; // 匹配视频扩展名
  }
  
  // 未知扩展名，返回true作为默认值（当作图片处理）
  return true;
};

// 处理媒体预览加载完成
function handleMediaPreviewLoad(event) {
  // 检查组件是否已卸载
  if (isUnmounted.value) {
    return
  }
  
  console.log('FormEditor: Media preview loaded successfully', event);
}

// 处理视频预览加载完成
function handleVideoLoaded(event) {
  // 检查组件是否已卸载
  if (isUnmounted.value) {
    return
  }
  
  console.log('FormEditor: Video preview loaded successfully', event);
}

// 处理媒体预览错误
function handleMediaPreviewError(event) {
  // 检查组件是否已卸载
  if (isUnmounted.value) {
    return
  }
  
  console.error('FormEditor: Media preview error', event);
  
  let errorMessage = '媒体预览加载失败';
  
  if (event && event.target) {
    const mediaSrc = event.target.src;
    const mediaType = formState.mediaType || 'unknown';
    
    // 检查URL是否有效
    if (!mediaSrc) {
      errorMessage = '媒体预览URL为空';
    } else if (mediaSrc.startsWith('blob:')) {
      errorMessage = `Blob URL媒体预览加载失败 [类型: ${mediaType}]`;
    } else {
      errorMessage = `媒体预览加载失败 [URL: ${mediaSrc}, 类型: ${mediaType}]`;
    }
  }
  
  console.error('FormEditor: Detailed preview error info', {
    formState: { ...formState },
    media: formState.media,
    mediaPreviewUrl: formState.mediaPreviewUrl,
    mediaType: formState.mediaType
  });
}

// 关闭预览模式
const closePreview = () => {
  isPreviewMode.value = false
}

// 取消编辑
const cancel = () => {
  try {
    // 如果表单已修改，提示用户确认
    if (hasFormChanged()) {
      ElMessageBox.confirm(
        '表单已修改，确定要取消吗？',
        '提示',
        {
          confirmButtonText: '确定',
          cancelButtonText: '返回编辑',
          type: 'warning'
        }
      ).then(() => {
        emit('cancel')
      }).catch(() => {
        // 用户选择继续编辑，不做操作
      })
    } else {
      // 表单未修改，直接取消
      emit('cancel')
    }
  } catch (error) {
    handleError(error, '取消编辑失败')
  }
}

// 判断表单是否有变化
const hasFormChanged = () => {
  // 简单判断，只比较主要字段
  if (!props.form) return formState.title || formState.value || formState.remark || formState.media
  return (
    formState.title !== props.form.title ||
    formState.value !== props.form.value ||
    formState.remark !== props.form.remark ||
    formState.media !== props.form.media
  )
}

// 提交表单
const submitForm = async () => {
  if (!formRef.value) return
  
  await formRef.value.validate(async (valid) => {
    if (valid) {
      isSaving.value = true
      try {
        // 构造表单数据
        const formData = {
          ...formState,
          id: formState.id,
          title: formState.title,
          value: formState.value,
          remark: formState.remark,
          media: formState.media, // 确保媒体URL被正确保存
          mediaPreviewUrl: formState.mediaPreviewUrl, // 保存预览URL
          mediaType: formState.mediaType, // 确保媒体类型被正确保存
          showTitle: formState.showTitle,
          showValue: formState.showValue,
          showRemark: formState.showRemark,
          showMedia: formState.showMedia,
          titleFontSize: formState.titleFontSize,
          valueFontSize: formState.valueFontSize,
          remarkFontSize: formState.remarkFontSize,
          titleColor: formState.titleColor,
          valueColor: formState.valueColor,
          remarkColor: formState.remarkColor,
          style: formState.style,
          elementStyles: formState.elementStyles
        }
        
        // 特别处理blob URL的情况
        if (formData.media && typeof formData.media === 'object' && formData.media.url) {
          // 如果media是MediaUploader传递的对象，提取url
          if (formData.media.url.startsWith('blob:')) {
            try {
              const base64Data = await blobUrlToBase64(formData.media.url);
              formData.media = base64Data;
              // 同时更新mediaPreviewUrl
              formData.mediaPreviewUrl = base64Data;
            } catch (e) {
              console.warn('Failed to convert blob URL to base64, keeping original URL', e);
            }
          } else {
            // 非blob URL，直接使用url
            formData.media = formData.media.url;
          }
        } else if (formData.media && typeof formData.media === 'string' && formData.media.startsWith('blob:')) {
          // 如果media是blob URL，我们需要将其转换为可持久化的格式
          // 这里我们尝试获取文件内容并转换为base64
          try {
            const base64Data = await blobUrlToBase64(formData.media);
            formData.media = base64Data;
            // 同时更新mediaPreviewUrl
            formData.mediaPreviewUrl = base64Data;
          } catch (e) {
            console.warn('Failed to convert blob URL to base64, keeping original URL', e);
          }
        } else if (!formData.media && 
                   formData.mediaPreviewUrl && 
                   typeof formData.mediaPreviewUrl === 'string' && 
                   formData.mediaPreviewUrl.startsWith('blob:')) {
          // 如果media为空但mediaPreviewUrl是blob URL，则使用mediaPreviewUrl作为media
          try {
            const base64Data = await blobUrlToBase64(formData.mediaPreviewUrl);
            formData.media = base64Data;
            formData.mediaPreviewUrl = base64Data;
          } catch (e) {
            console.warn('Failed to convert blob URL to base64, keeping original URL', e);
          }
        }
        
        // 如果有媒体URL但没有设置mediaType，则自动判断
        if (formData.media && !formData.mediaType) {
          formData.mediaType = isImageMedia(formData.media) ? 'image' : 'video'
        }
        
        console.log('Submitting form data:', formData);
        emit('save', formData)
        ElMessage.success(props.isEdit ? '表单更新成功' : '表单创建成功')
      } catch (error) {
        console.error('表单提交失败:', error)
        ElMessage.error('表单提交失败')
        emit('error', error)
      } finally {
        isSaving.value = false
      }
    } else {
      ElMessage.error('请检查表单填写是否正确')
    }
  })
}

// 将blob URL转换为base64格式
const blobUrlToBase64 = (blobUrl) => {
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', blobUrl);
    xhr.responseType = 'blob';
    
    xhr.onload = function() {
      if (xhr.status === 200) {
        const reader = new FileReader();
        reader.onloadend = function() {
          resolve(reader.result);
        };
        reader.onerror = function() {
          reject(new Error('Failed to read blob as base64'));
        };
        reader.readAsDataURL(xhr.response);
      } else {
        reject(new Error(`Failed to fetch blob: ${xhr.status}`));
      }
    };
    
    xhr.onerror = function() {
      reject(new Error('Network error while fetching blob'));
    };
    
    xhr.send();
  });
}

// 错误处理
const handleError = (error, context) => {
  try {
    console.error(`[FormEditor] ${context} 发生错误:`, error);
    
    // 确保loading状态被重置
    isSaving.value = false;
    
    // 如果是表单验证错误，不发送错误事件
    if (context === '表单验证失败') {
      console.log('[FormEditor] 表单验证失败，不发送错误事件');
      return;
    }
    
    // 发送错误事件
    emit('error', { error, context });
    
    // 显示用户友好的错误消息
    ElMessage.error(context);
  } catch (e) {
    console.error('[FormEditor] 处理错误时发生异常:', e);
    // 即使处理错误时发生异常，也要确保loading状态被重置
    isSaving.value = false;
  }
};

// 组件挂载后的操作
onMounted(async () => {
  try {
    // 确保DOM已渲染
    await nextTick()

    // 如果是编辑模式，自动聚焦到标题输入框
    if (props.isEdit && formRef.value) {
      // 通过 $el 访问真实的DOM元素
      const formElement = formRef.value.$el || formRef.value
      const titleInput = formElement.querySelector('input[type="text"]')
      if (titleInput) titleInput.focus()
    }
  } catch (error) {
    handleError(error, '组件挂载失败')
  }
})

// 组件卸载时清理创建的blob URL
onUnmounted(() => {
  if (formState.mediaPreviewUrl && 
      typeof formState.mediaPreviewUrl === 'string' && 
      formState.mediaPreviewUrl.startsWith('blob:')) {
    try {
      URL.revokeObjectURL(formState.mediaPreviewUrl);
      console.log('Revoked blob URL on FormEditor unmount:', formState.mediaPreviewUrl);
    } catch (e) {
      console.warn('Failed to revoke blob URL on FormEditor unmount:', formState.mediaPreviewUrl, e);
    }
  }
})

// 是否显示媒体
const shouldShowMedia = computed(() => {
  // 如果showMedia字段不存在，默认为true（向后兼容）
  const showMedia = formState.showMedia !== undefined ? formState.showMedia : true;
  
  // 检查是否有媒体内容
  let hasMedia = false;
  
  // 检查media字段
  if (formState.media) {
    if (typeof formState.media === 'string' && formState.media.trim() !== '') {
      hasMedia = true;
    } else if (typeof formState.media === 'object') {
      // 处理MediaUploader传递的对象或File对象
      if (formState.media instanceof File) {
        hasMedia = formState.media.size > 0;
      } else if (formState.media.url && typeof formState.media.url === 'string') {
        // MediaUploader传递的对象
        hasMedia = formState.media.url.trim() !== '';
      } else {
        // 其他对象类型
        hasMedia = true;
      }
    }
  }
  
  // 如果media字段没有有效内容，检查mediaPreviewUrl字段
  if (!hasMedia && formState.mediaPreviewUrl) {
    hasMedia = typeof formState.mediaPreviewUrl === 'string' && formState.mediaPreviewUrl.trim() !== '';
  }
  
  return showMedia !== false && hasMedia;
});

// 检查对象是否为File类型
const isFileObject = (obj) => {
  return obj instanceof File || (obj && typeof obj === 'object' && obj.constructor && obj.constructor.name === 'File');
};

</script>

<style scoped>
.form-editor {
  width: 100%;
  padding: 20px;
  box-sizing: border-box;
}

.form-editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.media-preview,
.local-media-preview {
  margin-top: 10px;
}

.media-preview img,
.media-preview video,
.media-preview audio,
.local-media-preview img,
.local-media-preview video {
  max-width: 100%;
  max-height: 200px;
  border-radius: 4px;
}

.local-media-preview {
  margin-top: 10px;
  padding: 10px;
  border: 1px dashed #dcdfe6;
  border-radius: 4px;
  background-color: #f5f7fa;
}

.local-media-preview .preview-label {
  font-size: 12px;
  color: #909399;
  margin-bottom: 5px;
}

.local-media-image,
.local-media-video {
  max-width: 100%;
  max-height: 200px;
  display: block;
  margin: 5px 0;
}

.local-media-preview .preview-label {
  margin-top: 8px;
  font-size: 14px;
  color: #666;
}

.local-media-preview img {
  margin-top: 4px;
}

.local-media-preview video {
  margin-top: 4px;
  width: 100%;
}

.form-content {
  width: 100%;
}

.element-container {
  display: flex;
  flex-direction: column;
}

.element-style-toggle {
  margin-top: 10px;
}

.element-style-controls {
  margin-top: 10px;
}

.media-type-select {
  margin-top: 10px;
}

.visibility-settings-panel,
.style-settings-panel {
  margin-top: 20px;
}

.visibility-grid,
.style-grid,
.position-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px;
}

.form-actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 30px;
}

/* 实时预览样式 */
.live-preview-section {
  margin-top: 30px;
  padding: 20px;
  border: 1px solid #ebeef5;
  border-radius: 4px;
  background-color: #f5f7fa;
}

.live-preview-section h4 {
  margin-top: 0;
  margin-bottom: 15px;
  color: #606266;
}

.live-preview-container {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 20px;
  background-color: white;
  border-radius: 4px;
  min-height: 200px;
  box-sizing: border-box;
}

.live-preview-card {
  width: 100%;
  max-width: 300px;
}

/* 全屏预览样式 */
.preview-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 3000;
}

.preview-content {
  background: #ffffff;
  border-radius: 8px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 80%;
  max-width: 600px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
}

.preview-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 20px;
  border-bottom: 1px solid #ebeef5;
}

.preview-header h3 {
  margin: 0;
  color: #303133;
}

.preview-close {
  background: #f5f7fa;
  border: none;
  color: #909399;
}

.preview-close:hover {
  background: #e1e4e8;
  color: #606266;
}

.preview-card {
  padding: 20px;
  flex: 1;
  overflow-y: auto;
  display: flex;
  justify-content: center;
  align-items: center;
}

.preview-card :deep(.single-form-card) {
  width: 100%;
  max-width: 300px;
}

.preview-footer {
  padding: 20px;
  border-top: 1px solid #ebeef5;
  display: flex;
  justify-content: flex-end;
}

.element-style-panel-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 3001;
}

.element-style-panel-content {
  background: #ffffff;
  padding: 20px;
  border-radius: 4px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 80%;
  max-width: 600px;
  position: relative;
}

.element-style-panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.global-style-panel-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 3002;
}

.global-style-panel-content {
  background: #ffffff;
  padding: 20px;
  border-radius: 4px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  width: 80%;
  max-width: 600px;
  position: relative;
}

.global-style-panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.global-style-panel-body {
  max-height: 60vh;
  overflow-y: auto;
}

.global-style-panel-actions {
  margin-top: 20px;
  padding-top: 15px;
  border-top: 1px solid #eee;
  text-align: right;
}

.element-style-panel-actions {
  margin-top: 20px;
  padding-top: 15px;
  border-top: 1px solid #eee;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .form-editor {
    padding: 15px;
  }

  .style-grid,
  .position-grid {
    grid-template-columns: 1fr;
    gap: 15px;
  }

  .preview-card {
    padding: 10px;
  }

  .element-styles-section {
    padding: 10px;
  }

  .element-style-item {
    padding: 8px;
  }

  .preview-mode-content {
    width: 95%;
    padding: 15px;
  }

  .preview-mode-actions {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }
  
  .live-preview-container {
    padding: 5px;
  }
}
</style>