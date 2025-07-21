export const savePositions = (positions, sizes) => {
  localStorage.setItem('formPositions', JSON.stringify(positions))
  localStorage.setItem('formSizes', JSON.stringify(sizes))
}

export const loadPositions = () => {
  const savedPositions = localStorage.getItem('formPositions')
  const savedSizes = localStorage.getItem('formSizes')
  
  return {
    positions: savedPositions ? JSON.parse(savedPositions) : {},
    sizes: savedSizes ? JSON.parse(savedSizes) : {}
  }
}

export const clearPositions = () => {
  localStorage.removeItem('formPositions')
  localStorage.removeItem('formSizes')
}