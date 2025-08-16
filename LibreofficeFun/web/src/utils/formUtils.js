// utils/formUtils.js

// 默认字段映射
export const fieldToColumnMapping = {
  id: 'A',
  title: 'B',
  value: 'C',
  remark: 'D',
  media: 'E',
  mediaType: 'F',
  showTitle: 'G',
  showValue: 'H',
  showRemark: 'I',
  showMedia: 'J',
  position: 'K',
};

// 动态生成字段映射
export const generateFieldToColumnMapping = (formDesign, rowNumber = 1) => {
  const mapping = {};
  let columnIndex = 0;

  // 检查 formDesign 是否为对象，如果是则转换为数组
  const fields = Array.isArray(formDesign) ? formDesign : [formDesign];

  fields.forEach((field) => {
    // 检查 value 是否为空值
    if (!field.value && field.value !== 0) {
      console.warn(`[generateFieldToColumnMapping] 忽略空值字段:`, field);
      return; // 跳过空值字段
    }

    // 生成列名（如 A1, B2, C3）
    const columnLetter = String.fromCharCode(65 + columnIndex); // A, B, C...
    const columnName = `${columnLetter}${rowNumber}`;

    // 添加 id 映射
    if (field.id !== undefined) {
      mapping[field.id] = columnName;
    }

    // 添加 title 映射
    if (field.title) {
      mapping[field.title] = columnName;
    }

    // 添加 value 映射
    if (field.value) {
      mapping[field.value] = columnName;
    }

    // 更新列索引
    columnIndex++;
  });

  return mapping;
};

// 新增逆映射函数：从单元格地址映射回字段名
export function generateColumnToFieldMapping(fieldToColumnMapping) {
  const columnToFieldMapping = {};

  // 遍历字段到列的映射，生成逆映射关系
  for (const [field, column] of Object.entries(fieldToColumnMapping)) {
    // 提取列标识（如 'A' 从 'A2', 'A3' 等）
    const baseColumn = column.replace(/\d+/g, ''); // 移除数字部分
    columnToFieldMapping[baseColumn] = field;
  }

  return columnToFieldMapping;
}
