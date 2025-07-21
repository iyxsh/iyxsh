export const validateForm = (form) => {
  if (!form) return false;
  
  // 验证表单必填字段
  if (!form.name || form.name.trim() === '') {
    console.error('表单名称不能为空');
    return false;
  }
  
  // 验证表单字段
  if (form.fields && Array.isArray(form.fields)) {
    for (const field of form.fields) {
      if (!field.label || field.label.trim() === '') {
        console.error('表单字段标签不能为空');
        return false;
      }
      
      // 验证字段类型
      const validTypes = ['text', 'number', 'select', 'checkbox', 'radio'];
      if (!validTypes.includes(field.type)) {
        console.error(`无效的字段类型: ${field.type}`);
        return false;
      }
      
      // 如果是选择框，验证选项
      if ((field.type === 'select' || field.type === 'checkbox' || field.type === 'radio') && 
          (!field.options || !Array.isArray(field.options) || field.options.length === 0)) {
        console.error(`字段 ${field.label} 缺少选项`);
        return false;
      }
    }
  }
  
  return true;
};

export const validateAllForms = (forms) => {
  if (!forms || !Array.isArray(forms) || forms.length === 0) {
    console.error('表单列表不能为空');
    return false;
  }
  
  for (const form of forms) {
    if (!validateForm(form)) {
      return false;
    }
  }
  
  return true;
};

export const useFormValidation = () => {
  return {
    validateForm,
    validateAllForms
  };
};