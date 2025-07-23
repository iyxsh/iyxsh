// cardStyleService.js
import { ref, reactive } from 'vue';

// 计算卡片样式
export const calculateCardStyle = (positions, element, index) => {
  // 确保 element 存在
  if (!element) {
    console.warn('元素未定义，无法计算样式');
    return {};
  }

  // 获取元素位置
  const position = positions[element.id] || { x: 0, y: 0 };

  return {
    position: 'absolute',
    left: `${position.x}px`,
    top: `${position.y}px`,
    zIndex: element.zIndex || 1,
    transform: 'none',
    transition: 'all 0.2s ease',
    boxShadow: '0 2px 8px rgba(0, 0, 0, 0.1)',
    borderRadius: '4px',
    margin: '5px'
  };
};

// 获取默认卡片样式
export const getDefaultCardStyles = () => {
  return {
    padding: '10px',
    margin: '5px',
    backgroundColor: '#f9f9f9',
    borderRadius: '4px',
    boxShadow: '0 1px 4px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.3s ease'
  };
};

// 获取卡片自动样式（支持响应式）
export const getCardAutoStyle = (forms, positions, index, element) => {
  if (!element || !forms[index]) {
    return {};
  }

  const position = positions[element.id] || { x: 0, y: 0 };
  const form = forms[index];

  return {
    position: 'absolute',
    left: `${position.x}px`,
    top: `${position.y}px`,
    width: `${form.width || 100}px`,
    height: `${form.height || 50}px`,
    zIndex: form.zIndex || 1,
    fontSize: `${form.fontSize || 14}px`,
    color: form.color || '#333',
    backgroundColor: form.backgroundColor || '#fff',
    transition: 'all 0.2s ease'
  };
};

// 初始化卡片样式服务
export const initializeCardStyleService = (containerRef, contentRefs, positions) => {
  if (!containerRef || !contentRefs || !positions) {
    console.warn('缺少初始化参数，无法初始化卡片样式服务');
    return;
  }

  // 设置容器引用
  const setContainer = (container) => {
    containerRef.value = container;
  };

  // 设置内容引用
  const setContentRefs = (refs) => {
    contentRefs.value = refs;
  };

  // 设置位置信息
  const setPositions = (newPositions) => {
    positions.value = newPositions;
  };

  return {
    setContainer,
    setContentRefs,
    setPositions
  };
};