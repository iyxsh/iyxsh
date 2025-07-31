// v-model参数接口定义
export interface CardGroups {
  id?: string;
  title?: string;
  rows: CardRow[];
  style?: Record<string, any>;
}[]

export interface CardRow {
  type: string;
  cards: Card[];
  style?: Record<string, any>;
}

export interface Card {
  text: string;
  type: string;
  textStyle?: Record<string, any>;
  style?: Record<string, any>;
}

// 修复：确保正确导出这些类型
export interface CardGroupStyles {
  [key: string]: any;
}

export interface CardRowStyles {
  [key: string]: any;
}

export interface CardStyles {
  [key: string]: any;
}

export interface GlobalTextStyles {
  [key: string]: any;
}

// 样式模板选项定义
export interface StyleTemplate {
  value: string;
  label: string;
}

// 卡片样式模板选项
export const cardStyleTemplates: StyleTemplate[] = [
  { value: 'card-default', label: '默认卡片样式' },
  { value: 'card-minimal', label: '极简卡片样式' },
  { value: 'card-rounded', label: '圆角卡片样式' },
  { value: 'card-outlined', label: '边框卡片样式' },
  { value: 'card-elevation', label: '投影卡片样式' },
  { value: 'card-gradient', label: '渐变卡片样式' }
];

// 卡片行样式模板选项
export const cardRowStyleTemplates: StyleTemplate[] = [
  { value: 'row-default', label: '默认行样式' },
  { value: 'row-compact', label: '紧凑行样式' },
  { value: 'row-spaced', label: '宽松行样式' },
  { value: 'row-centered', label: '居中行样式' },
  { value: 'row-highlighted', label: '高亮行样式' }
];

// 卡片组样式模板选项
export const cardGroupStyleTemplates: StyleTemplate[] = [
  { value: 'group-default', label: '默认组样式' },
  { value: 'group-minimal', label: '极简组样式' },
  { value: 'group-bordered', label: '边框组样式' },
  { value: 'group-elevated', label: '投影组样式' },
  { value: 'group-spaced', label: '间隔组样式' }
];

// 默认样式参数
export const defaultTextStyles = {
  // 常用设置
  fontSize: 16,
  color: '#000000',
  fontWeight: 'normal',
  fontStyle: 'normal',
  textAlign: 'left',
  lineHeight: 1.2,
  backgroundColor: '#ffffff',
  opacity: 1,
  textRotation: 0,
  scale: 1,

  // 文本相关属性
  textDecoration: 'none',
  textTransform: 'none',
  letterSpacing: 0,
  wordSpacing: 0,
  fontFamily: '',
  fontVariant: 'normal',
  verticalAlign: 'baseline',
  textIndent: 0,
  textShadow: '',
  textOverflow: '',
  wordBreak: 'normal',
  wordWrap: 'normal',
  whiteSpace: 'normal',
  direction: 'ltr',
  writingMode: 'horizontal-tb',

  // 盒模型相关（文本容器）
  padding: 0,
  margin: 0,
  borderStyle: 'none',
  borderWidth: 1,
  borderColor: '#000000',
  borderRadius: 0,

  // 其他可能影响文本显示的属性
  boxShadow: '',
  transform: '',
  cursor: 'default',
  visibility: 'visible',
  zIndex: 0,
  display: '',
  overflow: 'visible',
  userSelect: '',
  filter: '',
  outline: '',
  boxSizing: ''
};

// StyleManager默认样式
export const defaultStyleManagerStyles = {
  // 常用设置
  fontSize: 16,
  color: '#000000',
  fontWeight: 'normal',
  fontStyle: 'normal',
  textAlign: 'left',
  lineHeight: 1.2,
  backgroundColor: '#ffffff',
  opacity: 1,
  textRotation: 0,
  scale: 1,

  // 高级设置
  padding: 0,
  margin: 0,
  borderStyle: 'none',
  borderWidth: 1,
  borderColor: '#000000',
  borderRadius: 0,
  width: '',
  height: '',
  maxWidth: '',
  maxHeight: '',
  minWidth: '',
  minHeight: '',
  display: '',
  position: 'static',
  left: '',
  top: '',
  right: '',
  bottom: '',
  boxShadow: '',
  transform: '',
  textDecoration: 'none',
  textTransform: 'none',
  letterSpacing: 0,
  wordSpacing: 0,
  fontFamily: '',
  fontVariant: 'normal',
  verticalAlign: 'baseline',
  overflow: 'visible',
  cursor: 'default',
  visibility: 'visible',
  zIndex: 0,
  float: 'none',
  clear: 'none',

  // Flexbox相关属性
  flexDirection: 'row',
  flexWrap: 'nowrap',
  justifyContent: 'flex-start',
  alignItems: 'stretch',

  // Grid相关属性
  gridTemplateColumns: '',
  gridTemplateRows: '',
  gridColumnGap: 0,
  gridRowGap: 0,

  // 背景相关属性
  backgroundImage: '',
  backgroundRepeat: 'repeat',
  backgroundSize: 'auto',
  backgroundPosition: '',
  backgroundAttachment: 'scroll',

  // 文本相关属性
  textIndent: 0,
  textShadow: '',
  textOverflow: '',
  wordBreak: 'normal',
  wordWrap: 'normal',

  // 列表样式属性
  listStyleType: 'none',
  listStylePosition: 'outside',
  listStyleImage: '',

  // 过渡和动画属性
  transition: '',
  animation: '',

  // 滤镜属性
  filter: '',

  // outline属性
  outline: '',

  // white-space属性
  whiteSpace: 'normal',

  // 用户选择属性
  userSelect: '',

  // 盒模型属性
  boxSizing: '',

  // 方向属性
  direction: 'ltr',
  writingMode: 'horizontal-tb'
};

// 各种样式模板的默认值
export const cardStyleDefaults: Record<string, any> = {
  'card-default': {
    padding: 2,
    margin: 3,
    backgroundColor: '#ffffff',
    textColor: '#000000',
    borderStyle: 'none',
    borderRadius: 5,
    boxShadow: 'none',
    gradient: 'none',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'center',
    lineHeight: 1.2
  },
  'card-minimal': {
    padding: 2,
    margin: 3,
    backgroundColor: '#ffffff',
    textColor: '#000000',
    borderStyle: 'none',
    borderRadius: 0,
    boxShadow: 'none',
    gradient: 'none',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'center',
    lineHeight: 1.2
  },
  'card-rounded': {
    padding: 2,
    margin: 3,
    backgroundColor: '#ffffff',
    textColor: '#000000',
    borderStyle: 'none',
    borderRadius: 5,
    boxShadow: 'none',
    gradient: 'none',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'center',
    lineHeight: 1.2
  },
  'card-outlined': {
    padding: 2,
    margin: 3,
    backgroundColor: 'transparent',
    textColor: '#333333',
    borderStyle: 'solid',
    borderWidth: 1,
    borderColor: '#cccccc',
    borderRadius: 5,
    boxShadow: 'none',
    gradient: 'none',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'center',
    lineHeight: 1.2
  },
  'card-elevation': {
    padding: 2,
    margin: 3,
    backgroundColor: '#f8f8f8',
    textColor: '#000000',
    borderStyle: 'none',
    borderRadius: 5,
    boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
    gradient: 'none',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'center',
    lineHeight: 1.2
  },
  'card-gradient': {
    padding: 2,
    margin: 3,
    backgroundColor: 'linear-gradient(135deg, #ffffff, #f0f0f0)',
    textColor: '#000000',
    borderStyle: 'none',
    borderRadius: 5,
    boxShadow: 'none',
    gradient: 'linear-gradient(135deg, #ffffff, #f0f0f0)',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'center',
    lineHeight: 1.2
  }
};

export const cardRowStyleDefaults: Record<string, any> = {
  'row-default': {
    padding: '5px',
    margin: '2px 0',
    gap: '2px',
    backgroundColor: 'transparent'
  },
  'row-compact': {
    padding: '2px',
    margin: '1px',
    gap: '1px',
    backgroundColor: 'transparent'
  },
  'row-spaced': {
    padding: '8px',
    margin: '4px 0',
    gap: '8px',
    backgroundColor: '#f9f9f9'
  },
  'row-centered': {
    padding: '5px',
    margin: '3px 0',
    gap: '3px',
    justifyContent: 'center',
    backgroundColor: 'transparent'
  },
  'row-highlighted': {
    padding: '6px',
    margin: '4px 0',
    gap: '4px',
    backgroundColor: '#e6f7ff',
    border: '1px solid #91d5ff',
    borderRadius: '4px'
  }
};

export const cardGroupStyleDefaults: Record<string, any> = {
  'group-default': {
    padding: '8px',
    margin: '10px 0',
    border: 'none',
    borderRadius: '4px',
    backgroundColor: '#ffffff'
  },
  'group-minimal': {
    padding: '0',
    margin: '5px 0',
    border: 'none',
    backgroundColor: 'transparent'
  },
  'group-bordered': {
    padding: '10px',
    margin: '10px 0',
    border: '1px solid #d9d9d9',
    borderRadius: '6px',
    backgroundColor: '#ffffff'
  },
  'group-elevated': {
    padding: '12px',
    margin: '12px 0',
    border: 'none',
    borderRadius: '6px',
    backgroundColor: '#ffffff',
    boxShadow: '0 2px 8px rgba(0, 0, 0, 0.15)'
  },
  'group-spaced': {
    padding: '8px',
    margin: '15px 0',
    border: 'none',
    backgroundColor: 'transparent'
  }
};