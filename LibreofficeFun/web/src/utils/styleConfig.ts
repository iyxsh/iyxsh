// v-model参数接口定义
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

// 修复：正确导出CardGroups类型
export type CardGroups = {
  id?: string;
  title?: string;
  rows: CardRow[];
  style?: Record<string, any>;
}[]

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

// 样式接口
export interface StyleObject {
  enabled?: boolean;
  [key: string]: any;
}

// 转换样式对象为CSS对象（与StyleManager.vue中的toCSS方法保持一致）
export const toCSS = (style: StyleObject, visibleStyles: Set<string> = new Set(Object.keys(style))) => {
  // 如果样式未启用，返回空对象
  if (style.enabled === false) {
    return {};
  }
  
  const css: Record<string, string | number> = {};
  
  // 常用设置
  if (visibleStyles.has('fontSize') && style.fontSize) css['font-size'] = `${style.fontSize}px`;
  if (visibleStyles.has('color') && style.color) css['color'] = style.color;
  if (visibleStyles.has('fontFamily') && style.fontFamily) {
    // 处理字体族，如果是自定义则使用customFontFamily的值
    const fontFamilyValue = style.fontFamily === 'custom' ? style.customFontFamily : style.fontFamily;
    if (fontFamilyValue) css['font-family'] = fontFamilyValue;
  }
  if (visibleStyles.has('fontWeight') && style.fontWeight && style.fontWeight !== 'normal') css['font-weight'] = style.fontWeight;
  if (visibleStyles.has('fontStyle') && style.fontStyle && style.fontStyle !== 'normal') css['font-style'] = style.fontStyle;
  if (visibleStyles.has('textAlign') && style.textAlign && style.textAlign !== 'left') css['text-align'] = style.textAlign;
  if (visibleStyles.has('lineHeight') && style.lineHeight && style.lineHeight !== 1.2) css['line-height'] = style.lineHeight;
  if (visibleStyles.has('backgroundColor') && style.backgroundColor) css['background-color'] = style.backgroundColor;
  if (visibleStyles.has('opacity') && style.opacity !== 1) css['opacity'] = style.opacity;

  // 处理旋转和缩放 (textRotation和scale对应CSS的transform属性)
  if (visibleStyles.has('textRotation') || visibleStyles.has('scale')) {
    const rotation = style.textRotation || 0;
    const scale = style.scale || 1;
    css['transform'] = `rotate(${rotation}deg) scale(${scale})`;
  } else if (visibleStyles.has('transform') && style.transform) {
    css['transform'] = style.transform;
  }

  // 高级设置
  if (visibleStyles.has('padding') && style.padding !== undefined) css['padding'] = `${style.padding}px`;
  if (visibleStyles.has('margin') && style.margin !== undefined) css['margin'] = `${style.margin}px`;

  // 边框设置
  if (visibleStyles.has('borderStyle') && style.borderStyle && style.borderStyle !== 'none') {
    if (visibleStyles.has('borderWidth')) css['border-width'] = `${style.borderWidth}px`;
    css['border-style'] = style.borderStyle;
    if (visibleStyles.has('borderColor') && style.borderColor) css['border-color'] = style.borderColor;
  } else if (visibleStyles.has('borderStyle') && style.borderStyle === 'none') {
    css['border'] = 'none';
  }

  if (visibleStyles.has('borderRadius') && style.borderRadius !== undefined && style.borderRadius !== 0) css['border-radius'] = `${style.borderRadius}px`;
  if (visibleStyles.has('width') && style.width) css['width'] = style.width;
  if (visibleStyles.has('height') && style.height) css['height'] = style.height;
  if (visibleStyles.has('maxWidth') && style.maxWidth) css['max-width'] = style.maxWidth;
  if (visibleStyles.has('maxHeight') && style.maxHeight) css['max-height'] = style.maxHeight;
  if (visibleStyles.has('minWidth') && style.minWidth) css['min-width'] = style.minWidth;
  if (visibleStyles.has('minHeight') && style.minHeight) css['min-height'] = style.minHeight;
  if (visibleStyles.has('display') && style.display) css['display'] = style.display;
  if (visibleStyles.has('position') && style.position && style.position !== 'static') {
    css['position'] = style.position;
    if (visibleStyles.has('left') && style.left) css['left'] = style.left;
    if (visibleStyles.has('top') && style.top) css['top'] = style.top;
    if (visibleStyles.has('right') && style.right) css['right'] = style.right;
    if (visibleStyles.has('bottom') && style.bottom) css['bottom'] = style.bottom;
  }
  if (visibleStyles.has('boxShadow') && style.boxShadow) css['box-shadow'] = style.boxShadow;
  if (visibleStyles.has('textDecoration') && style.textDecoration && style.textDecoration !== 'none') css['text-decoration'] = style.textDecoration;
  if (visibleStyles.has('textTransform') && style.textTransform && style.textTransform !== 'none') css['text-transform'] = style.textTransform;
  if (visibleStyles.has('letterSpacing') && style.letterSpacing) css['letter-spacing'] = `${style.letterSpacing}px`;
  if (visibleStyles.has('wordSpacing') && style.wordSpacing) css['word-spacing'] = `${style.wordSpacing}px`;
  if (visibleStyles.has('fontVariant') && style.fontVariant && style.fontVariant !== 'normal') css['font-variant'] = style.fontVariant;
  if (visibleStyles.has('verticalAlign') && style.verticalAlign && style.verticalAlign !== 'baseline') css['vertical-align'] = style.verticalAlign;
  if (visibleStyles.has('overflow') && style.overflow && style.overflow !== 'visible') css['overflow'] = style.overflow;
  if (visibleStyles.has('cursor') && style.cursor && style.cursor !== 'default') css['cursor'] = style.cursor;
  if (visibleStyles.has('visibility') && style.visibility && style.visibility !== 'visible') css['visibility'] = style.visibility;
  if (visibleStyles.has('zIndex') && style.zIndex && style.zIndex !== 0) css['z-index'] = style.zIndex;
  if (visibleStyles.has('float') && style.float && style.float !== 'none') css['float'] = style.float;
  if (visibleStyles.has('clear') && style.clear && style.clear !== 'none') css['clear'] = style.clear;

  // Flexbox相关属性
  if (visibleStyles.has('display') && style.display === 'flex') {
    if (visibleStyles.has('flexDirection') && style.flexDirection && style.flexDirection !== 'row') css['flex-direction'] = style.flexDirection;
    if (visibleStyles.has('flexWrap') && style.flexWrap && style.flexWrap !== 'nowrap') css['flex-wrap'] = style.flexWrap;
    if (visibleStyles.has('justifyContent') && style.justifyContent && style.justifyContent !== 'flex-start') css['justify-content'] = style.justifyContent;
    if (visibleStyles.has('alignItems') && style.alignItems && style.alignItems !== 'stretch') css['align-items'] = style.alignItems;
    if (visibleStyles.has('alignContent') && style.alignContent && style.alignContent !== 'stretch') css['align-content'] = style.alignContent;
  }

  // Grid相关属性
  if (visibleStyles.has('display') && style.display === 'grid') {
    if (visibleStyles.has('gridTemplateColumns') && style.gridTemplateColumns) css['grid-template-columns'] = style.gridTemplateColumns;
    if (visibleStyles.has('gridTemplateRows') && style.gridTemplateRows) css['grid-template-rows'] = style.gridTemplateRows;
    if (visibleStyles.has('gridGap') && style.gridGap) css['grid-gap'] = style.gridGap;
    if (visibleStyles.has('gridColumnGap') && style.gridColumnGap) css['grid-column-gap'] = style.gridColumnGap;
    if (visibleStyles.has('gridRowGap') && style.gridRowGap) css['grid-row-gap'] = style.gridRowGap;
  }

  return css;
};

// 转换CSS对象为样式对象（与StyleManager.vue中的CSSto方法保持一致）
export const CSSto = (cssObj: Record<string, any>): StyleObject => {
  const localStyleObj: StyleObject = {};
  
  // CSS属性名到localStyle键名的映射
  const cssToStyleKeyMap = {
    'font-size': 'fontSize',
    'font-family': 'fontFamily',
    'font-weight': 'fontWeight',
    'font-style': 'fontStyle',
    'text-align': 'textAlign',
    'line-height': 'lineHeight',
    'background-color': 'backgroundColor',
    'border-width': 'borderWidth',
    'border-style': 'borderStyle',
    'border-color': 'borderColor',
    'border-radius': 'borderRadius',
    'max-width': 'maxWidth',
    'max-height': 'maxHeight',
    'min-width': 'minWidth',
    'min-height': 'minHeight',
    'box-shadow': 'boxShadow',
    'text-decoration': 'textDecoration',
    'text-transform': 'textTransform',
    'letter-spacing': 'letterSpacing',
    'word-spacing': 'wordSpacing',
    'font-variant': 'fontVariant',
    'vertical-align': 'verticalAlign',
    'z-index': 'zIndex',
    'grid-template-columns': 'gridTemplateColumns',
    'grid-template-rows': 'gridTemplateRows',
    'grid-column-gap': 'gridColumnGap',
    'grid-row-gap': 'gridRowGap',
    'place-content': 'placeContent',
    'background-image': 'backgroundImage',
    'background-repeat': 'backgroundRepeat',
    'background-size': 'backgroundSize',
    'background-position': 'backgroundPosition',
    'background-attachment': 'backgroundAttachment',
    'text-indent': 'textIndent',
    'text-shadow': 'textShadow',
    'text-overflow': 'textOverflow',
    'word-break': 'wordBreak',
    'word-wrap': 'wordWrap',
    'white-space': 'whiteSpace',
    'list-style-type': 'listStyleType',
    'list-style-position': 'listStylePosition',
    'list-style-image': 'listStyleImage',
    'user-select': 'userSelect',
    'box-sizing': 'boxSizing',
    'writing-mode': 'writingMode'
  };
  
  // 处理特殊属性
  for (const [cssKey, cssValue] of Object.entries(cssObj)) {
    // 获取对应的localStyle键名
    const localStyleKey = cssToStyleKeyMap[cssKey] || cssKey;
    
    // 特殊处理font-size（去除px单位）
    if (cssKey === 'font-size' && typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理letter-spacing和word-spacing（去除px单位）
    if ((cssKey === 'letter-spacing' || cssKey === 'word-spacing') && 
        typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理border-radius（去除px单位）
    if (cssKey === 'border-radius' && typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理grid-column-gap和grid-row-gap（去除px单位）
    if ((cssKey === 'grid-column-gap' || cssKey === 'grid-row-gap') && 
        typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理text-indent（去除px单位）
    if (cssKey === 'text-indent' && typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理padding和margin（去除px单位）
    if ((cssKey === 'padding' || cssKey === 'margin') && 
        typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理border-width（去除px单位）
    if (cssKey === 'border-width' && typeof cssValue === 'string' && cssValue.endsWith('px')) {
      localStyleObj[localStyleKey] = parseFloat(cssValue);
      continue;
    }
    
    // 特殊处理transform属性（提取旋转角度和缩放比例）
    if (cssKey === 'transform') {
      // 解析类似 rotate(30deg) scale(1.2) 的transform值
      const transformValue = cssValue;
      const rotationMatch = transformValue.match(/rotate\((\d+)deg\)/);
      const scaleMatch = transformValue.match(/scale\((\d+(?:\.\d+)?)\)/);
      
      if (rotationMatch) {
        localStyleObj['textRotation'] = parseInt(rotationMatch[1], 10);
      }
      
      if (scaleMatch) {
        localStyleObj['scale'] = parseFloat(scaleMatch[1]);
      }
      
      // 保留原始transform值
      localStyleObj[localStyleKey] = cssValue;
      continue;
    }
    
    // 一般情况直接赋值
    localStyleObj[localStyleKey] = cssValue;
  }
  
  return localStyleObj;
};

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