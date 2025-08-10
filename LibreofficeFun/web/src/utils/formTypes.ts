/**
 * 表单元素样式接口
 */
export interface ElementStyle {
  enabled: boolean;
  color: string;
  fontSize: number;
  fontWeight: string;
}

/**
 * 表单整体样式接口
 */
export interface FormStyle {
  backgroundColor: string;
  color: string;
  borderWidth: number;
  borderStyle: string;
  borderColor: string;
  borderRadius: number;
  fontSize: number;
  padding: number;
  hasShadow: boolean;
}

/**
 * 表单位置接口
 */
export interface FormPosition {
  x: number;
  y: number;
}

/**
 * 表单尺寸接口
 */
export interface FormSize {
  width: number;
  height: number;
}

/**
 * 表单元素样式集合接口
 */
export interface ElementStyles {
  title: ElementStyle;
  value: ElementStyle;
  remark: ElementStyle;
  media: ElementStyle;
}

/**
 * 表单数据接口
 */
export interface Form {
  id: string;
  title: string;
  value: string;
  remark: string;
  media: string;
  mediaType: 'image' | 'video';
  showTitle: boolean;
  showValue: boolean;
  showRemark: boolean;
  showMedia: boolean;
  style: FormStyle;
  elementStyles: ElementStyles;
  position: FormPosition;
  size: FormSize;
  mediaPreviewUrl: string;
}