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

// 重新实现 excelToFormMapping 函数，添加对JSON字符串的解析
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
      const fieldName = columnToFieldMapping[baseColumn];
      // 尝试解析key为JSON对象，特别是对于position和size等字段
      if ((fieldName === 'position' || fieldName === 'size') && typeof key === 'string') {
        try {
          // 尝试将字符串解析为对象
          const parsedValue = JSON.parse(key);
          // 验证解析结果是否为对象且包含必要的坐标属性
          if (typeof parsedValue === 'object' && parsedValue !== null) {
            // 对于position，确保包含x和y属性
            if (fieldName === 'position' && ('x' in parsedValue || 'y' in parsedValue)) {
              resultMapping[fieldName] = parsedValue;
              continue;
            }
            // 对于size，确保包含width和height属性
            if (fieldName === 'size' && ('width' in parsedValue || 'height' in parsedValue)) {
              resultMapping[fieldName] = parsedValue;
              continue;
            }
          }
        } catch (e) {
          // 解析失败，继续使用原始值
          console.warn(`[excelToFormMapping] 解析${fieldName}对象失败:`, e);
        }
      }
      // 处理其他可能包含对象的字段
      if (typeof key === 'string' && key.startsWith('{') && key.endsWith('}')) {
        try {
          const parsedValue = JSON.parse(key);
          if (typeof parsedValue === 'object' && parsedValue !== null) {
            resultMapping[fieldName] = parsedValue;
            continue;
          }
        } catch (e) {
          // 解析失败，使用原始值
        }
      }
      // 使用原始值
      resultMapping[fieldName] = key;
    } else {
      console.warn(`[excelToFormMapping] 未找到列映射: ${value}`);
    }
  }

  // 确保返回的对象包含必要的默认属性
  if (!resultMapping.position) {
    resultMapping.position = { x: 0, y: 0 };
  } else {
    // 确保position有x和y属性
    resultMapping.position.x = resultMapping.position.x || 0;
    resultMapping.position.y = resultMapping.position.y || 0;
  }
  
  if (!resultMapping.size) {
    resultMapping.size = { width: 200, height: 80 };
  } else {
    // 确保size有width和height属性
    resultMapping.size.width = resultMapping.size.width || 200;
    resultMapping.size.height = resultMapping.size.height || 80;
  }

  return resultMapping;
};
