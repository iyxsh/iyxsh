export const calculateCardStyle = (positions, element, index) => {
  return {
    position: 'absolute',
    left: (positions[element.id]?.x || 0) + 'px',
    top: (positions[element.id]?.y || 0) + 'px',
    zIndex: element.zIndex,
    transform: 'none'
  };
};

export const getDefaultCardStyles = () => ({
  padding: '8px'
});