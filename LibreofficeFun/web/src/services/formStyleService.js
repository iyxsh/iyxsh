export const getFormStyle = (form) => {
  return {
    padding: '10px',
    backgroundColor: '#f9f9f9',
    borderRadius: '4px',
    boxShadow: '0 2px 4px rgba(0, 0, 0, 0.1)'
  };
};

export const getCardStyle = (positions, element) => {
  return {
    position: 'absolute',
    left: (positions[element.id]?.x || 0) + 'px',
    top: (positions[element.id]?.y || 0) + 'px',
    zIndex: element.zIndex
  };
};

export const applyFormStyles = (forms, positions) => {
  const styles = {};
  
  forms.forEach(form => {
    styles[form.id] = {
      ...getFormStyle(form),
      ...getCardStyle(positions, form)
    };
  });
  
  return styles;
};

export const useFormStyle = () => {
  return {
    getFormStyle,
    getCardStyle,
    applyFormStyles
  };
};