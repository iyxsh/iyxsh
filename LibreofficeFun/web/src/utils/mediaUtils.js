/**
 * 检查文件是否为受支持的媒体格式
 * @param {File} file - 要检查的文件对象
 * @returns {boolean} - 是否受支持
 */
export function isSupportedMediaFormat(file) {
  if (!file || !file.type) {
    return false;
  }
  
  // 支持的图片MIME类型
  const supportedImageTypes = [
    'image/jpeg',
    'image/png',
    'image/gif',
    'image/bmp',
    'image/webp',
    'image/svg+xml'
  ];
  
  // 支持的视频MIME类型
  const supportedVideoTypes = [
    'video/mp4',
    'video/webm',
    'video/ogg',
    'video/quicktime',
    'video/x-msvideo',
    'video/x-flv',
    'video/x-matroska'
  ];
  
  return supportedImageTypes.includes(file.type) || supportedVideoTypes.includes(file.type);
}

/**
 * 统一的媒体类型判断函数
 * @param {string|object} media - 媒体对象或URL
 * @param {string} [defaultType='image'] - 默认类型
 * @returns {'image'|'video'} 媒体类型
 */
export function getMediaType(media, defaultType = 'image') {
  // 处理空值情况
  if (!media) {
    return defaultType;
  }

  // 如果是对象，优先检查对象属性
  if (typeof media === 'object') {
    // MediaUploader传递的对象，包含mediaType属性
    if (media.mediaType) {
      return media.mediaType;
    }
    
    // MediaUploader传递的对象，包含isImage属性
    if (typeof media.isImage === 'boolean') {
      return media.isImage ? 'image' : 'video';
    }
    
    // 检查是否有File对象
    if (media instanceof File || (media.file && media.file instanceof File)) {
      const file = media instanceof File ? media : media.file;
      if (file.type) {
        return file.type.startsWith('image/') ? 'image' : 
               file.type.startsWith('video/') ? 'video' : defaultType;
      }
    }
    
    // 检查url属性
    if (media.url && typeof media.url === 'string') {
      return getMediaType(media.url, defaultType);
    }
  }
  
  // 处理字符串
  if (typeof media === 'string') {
    // 处理data URL
    if (media.startsWith('data:')) {
      return media.startsWith('data:image/') ? 'image' : 
             media.startsWith('data:video/') ? 'video' : defaultType;
    }
    
    // 处理带前缀的URL
    if (media.startsWith('image:')) {
      return 'image';
    }
    
    if (media.startsWith('video:')) {
      return 'video';
    }
    
    // 处理带查询参数或hash的URL
    try {
      const urlObj = new URL(media.split('?')[0].split('#')[0]);
      const urlParams = new URLSearchParams(urlObj.search);
      
      // 检查查询参数中的mediaType
      if (urlParams.has('mediaType')) {
        const paramType = urlParams.get('mediaType');
        return (paramType === 'image' || paramType === 'video') ? paramType : defaultType;
      }
      
      // 检查hash
      if (urlObj.hash === '#image') {
        return 'image';
      }
      
      if (urlObj.hash === '#video') {
        return 'video';
      }
      
      // 检查扩展名
      const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
      const videoExtensions = /\.(mp4|webm|ogg|avi|mov|wmv|flv|mkv)$/i;
      
      if (imageExtensions.test(urlObj.pathname)) {
        return 'image';
      }
      
      if (videoExtensions.test(urlObj.pathname)) {
        return 'video';
      }
    } catch (e) {
      // URL解析失败，继续使用扩展名检查
      const cleanUrl = media.split('?')[0].split('#')[0];
      const imageExtensions = /\.(jpg|jpeg|png|gif|bmp|webp|svg)$/i;
      const videoExtensions = /\.(mp4|webm|ogg|avi|mov|wmv|flv|mkv)$/i;
      
      if (imageExtensions.test(cleanUrl)) {
        return 'image';
      }
      
      if (videoExtensions.test(cleanUrl)) {
        return 'video';
      }
    }
  }
  
  // 默认返回值
  return defaultType;
}

/**
 * 创建媒体预览URL
 * @param {File|Blob|string|object} media - 媒体文件或URL
 * @returns {string|null}
 */
export function createMediaPreviewUrl(media) {
  if (!media) return null;
  
  // 如果是包含url属性的对象，直接返回url
  if (typeof media === 'object' && media.url && typeof media.url === 'string') {
    // 移除媒体类型前缀
    return media.url.replace(/^(image:|video:)/, '');
  }
  
  // 如果是File或Blob对象，创建预览URL
  if (media instanceof Blob) {
    return URL.createObjectURL(media);
  }
  
  // 如果是字符串类型的URL，移除媒体类型前缀
  if (typeof media === 'string') {
    return media.replace(/^(image:|video:)/, '');
  }
  
  return null;
}

/**
 * 安全地撤销Blob URL
 * @param {string} url - 要撤销的URL
 */
export function revokeBlobURL(url) {
  if (url && typeof url === 'string' && url.startsWith('blob:')) {
    try {
      URL.revokeObjectURL(url);
    } catch (e) {
      console.warn('Failed to revoke blob URL:', url, e);
    }
  }
}