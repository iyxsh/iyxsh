let cardStyleServiceInstance = null;
let handleDragStartInstance = null;
let handleDragEndInstance = null;
let handleDragOverInstance = null;
let handleDropInstance = null;

export const handleDragStart = (event, form) => {
  if (handleDragStartInstance) {
    handleDragStartInstance(event, form);
  }
  event.dataTransfer.setData('text/plain', form.id);
  event.dataTransfer.effectAllowed = 'move';
};

export const handleDragEnd = (event) => {
  if (handleDragEndInstance) {
    handleDragEndInstance(event);
  }
  // 拖拽结束逻辑
};

export const handleDragOver = (event, form) => {
  if (handleDragOverInstance) {
    handleDragOverInstance(event, form);
  }
  event.preventDefault();
  event.dataTransfer.dropEffect = 'move';
  return false;
};

export const handleDrop = (event) => {
  if (handleDropInstance) {
    handleDropInstance(event);
  }
  event.preventDefault();
  const formId = event.dataTransfer.getData('text/plain');
  // 处理放置逻辑
};

// 设置卡片样式服务
export const setCardStyleService = (service) => {
  cardStyleServiceInstance = service;
};

// 设置拖拽开始处理程序
export const setHandleDragStart = (handler) => {
  handleDragStartInstance = handler;
};

// 设置拖拽结束处理程序
export const setHandleDragEnd = (handler) => {
  handleDragEndInstance = handler;
};

// 设置拖拽悬停处理程序
export const setHandleDragOver = (handler) => {
  handleDragOverInstance = handler;
};

// 设置拖拽释放处理程序
export const setHandleDrop = (handler) => {
  handleDropInstance = handler;
};