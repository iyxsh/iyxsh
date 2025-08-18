// utils/formUtils.js

// 默认字段映射
export const fieldToColumnMapping = {
  "id": "A",
  "title": "B",
  "value": "C",
  "remark": "D",
  "media": "E",
  "mediaType": "F",
  "showTitle": "G",
  "showValue": "H",
  "showRemark": "I",
  "showMedia": "J",
  "position": "K",
};

// 表单转为 Excel 格式的映射函数
export const formToExcelMapping = (inputObject, columnToFieldMapping, columnNumber) => {
  const newMapping = {};

  for (const [field, value] of Object.entries(inputObject)) {
    // 过滤掉 value 为 '' 的字段
    if (value === '') continue;
    if (columnToFieldMapping[field]) {
      const columnValue = `${columnToFieldMapping[field]}${columnNumber}`;
      // 如果字段的值是对象，则将其整体作为 key
      if (typeof value === 'object' && value !== null) {
        newMapping[JSON.stringify(value)] = columnValue;
      } else {
        newMapping[value] = columnValue;
      }
    }
  }
  return newMapping;
};

// 重新实现 excelToFormMapping 函数
export const excelToFormMapping = (fieldToColumnMapping, excelMapping) => {
  // 初始化 columnToFieldMapping
  const columnToFieldMapping = {};

  // 遍历 fieldToColumnMapping，将 value 作为 key，key 作为 value
  for (const [field, column] of Object.entries(fieldToColumnMapping)) {
    columnToFieldMapping[column] = field;
  }

  const resultMapping = {};

  // 遍历 excelMapping，生成新的映射，并去除列标识后面的数字
  for (const [key, value] of Object.entries(excelMapping)) {
    const baseColumn = value.replace(/\d+/g, ''); // 去除数字部分
    if (columnToFieldMapping[baseColumn]) {
      resultMapping[columnToFieldMapping[baseColumn]] = key;
    } else {
      console.warn(`[excelToFormMapping] 未找到列映射: ${value}`);
    }
  }

  return resultMapping;
};
