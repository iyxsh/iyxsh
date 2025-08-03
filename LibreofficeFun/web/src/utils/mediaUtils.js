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