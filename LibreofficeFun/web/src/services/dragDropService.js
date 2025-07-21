export const handleDragStart = (event, form) => {
  event.dataTransfer.setData('text/plain', form.id);
  event.dataTransfer.effectAllowed = 'move';
};

export const handleDragEnd = (event) => {
  // 拖拽结束逻辑
};

export const handleDragOver = (event, form) => {
  event.preventDefault();
  event.dataTransfer.dropEffect = 'move';
  return false;
};

export const handleDrop = (event) => {
  event.preventDefault();
  const formId = event.dataTransfer.getData('text/plain');
  // 处理放置逻辑
};