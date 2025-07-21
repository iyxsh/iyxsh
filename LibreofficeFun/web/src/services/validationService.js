/**
 * 通用表单验证服务
 * 提供可复用的表单验证规则和工具函数
 */

/**
 * 检查是否为空值
 * @param {any} value - 要检查的值
 * @returns {boolean} 是否为空
 */
export const isEmpty = (value) => {
  if (value === null || value === undefined) return true
  if (typeof value === 'string') return value.trim() === ''
  if (Array.isArray(value)) return value.length === 0
  if (typeof value === 'object') return Object.keys(value).length === 0
  return false
}

/**
 * 验证电子邮件
 * @param {string} email - 电子邮件地址
 * @returns {boolean} 是否有效
 */
export const isValidEmail = (email) => {
  if (isEmpty(email)) return false
  const re = /^(([^<>()\[\]\\.,;:\s@"]+(\.[^<>()\[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/
  return re.test(String(email).toLowerCase())
}

/**
 * 验证URL
 * @param {string} url - URL地址
 * @returns {boolean} 是否有效
 */
export const isValidUrl = (url) => {
  if (isEmpty(url)) return false
  try {
    new URL(url)
    return true
  } catch (e) {
    return false
  }
}

/**
 * 检查字符串最小长度
 * @param {string} value - 要检查的字符串
 * @param {number} length - 最小长度
 * @returns {boolean} 是否满足最小长度
 */
export const minLength = (value, length) => {
  if (isEmpty(value)) return false
  return String(value).length >= length
}

/**
 * 检查字符串最大长度
 * @param {string} value - 要检查的字符串
 * @param {number} length - 最大长度
 * @returns {boolean} 是否不超过最大长度
 */
export const maxLength = (value, length) => {
  if (isEmpty(value)) return true
  return String(value).length <= length
}

/**
 * 检查数值范围
 * @param {number} value - 要检查的数值
 * @param {number} min - 最小值
 * @param {number} max - 最大值
 * @returns {boolean} 是否在范围内
 */
export const inRange = (value, min, max) => {
  if (isEmpty(value)) return false
  const num = Number(value)
  return !isNaN(num) && num >= min && num <= max
}

/**
 * 正则表达式验证
 * @param {string} value - 要验证的字符串
 * @param {RegExp} regex - 正则表达式
 * @returns {boolean} 是否匹配
 */
export const matchesPattern = (value, regex) => {
  if (isEmpty(value)) return false
  return regex.test(String(value))
}

/**
 * 验证表单
 * @param {Object} data - 表单数据
 * @param {Object} rules - 验证规则
 * @returns {Object} 验证结果 { isValid, errors }
 */
export const validateForm = (data, rules) => {
  const errors = {}
  let isValid = true

  for (const field in rules) {
    const fieldRules = rules[field]
    const value = data[field]
    const fieldErrors = []

    // 遍历字段的所有规则
    for (const rule of fieldRules) {
      let valid = true
      let message = ''

      // 必填字段检查
      if (rule.required && isEmpty(value)) {
        valid = false
        message = rule.message || `${field} 是必填项`
      }
      // 电子邮件格式检查
      else if (rule.email && !isValidEmail(value)) {
        valid = false
        message = rule.message || `请输入有效的电子邮件地址`
      }
      // URL格式检查
      else if (rule.url && !isValidUrl(value)) {
        valid = false
        message = rule.message || `请输入有效的URL`
      }
      // 最小长度检查
      else if (rule.minLength && !minLength(value, rule.minLength)) {
        valid = false
        message = rule.message || `长度不能少于 ${rule.minLength} 个字符`
      }
      // 最大长度检查
      else if (rule.maxLength && !maxLength(value, rule.maxLength)) {
        valid = false
        message = rule.message || `长度不能超过 ${rule.maxLength} 个字符`
      }
      // 数值范围检查
      else if ((rule.min !== undefined || rule.max !== undefined) && 
               !inRange(value, rule.min || Number.MIN_SAFE_INTEGER, rule.max || Number.MAX_SAFE_INTEGER)) {
        valid = false
        message = rule.message || `值必须在 ${rule.min || '最小值'} 和 ${rule.max || '最大值'} 之间`
      }
      // 模式匹配检查
      else if (rule.pattern && !matchesPattern(value, rule.pattern)) {
        valid = false
        message = rule.message || `输入格式不正确`
      }
      // 自定义验证函数
      else if (rule.validator && typeof rule.validator === 'function') {
        const result = rule.validator(value, data)
        valid = result === true
        if (!valid) {
          message = typeof result === 'string' ? result : (rule.message || `验证失败`)
        }
      }

      if (!valid) {
        fieldErrors.push(message)
        isValid = false
      }
    }

    if (fieldErrors.length > 0) {
      errors[field] = fieldErrors
    }
  }

  return { isValid, errors }
}

/**
 * 表单卡片数据验证
 * @param {Object} formData - 表单数据
 * @returns {Object} 验证结果 { isValid, errors }
 */
export const validateFormCard = (formData) => {
  const rules = {
    title: [
      { required: true, message: '标题不能为空' },
      { maxLength: 50, message: '标题不能超过50个字符' }
    ],
    value: [
      { required: true, message: '值不能为空' },
      { maxLength: 100, message: '值不能超过100个字符' }
    ],
    remark: [
      { maxLength: 200, message: '备注不能超过200个字符' }
    ]
  }

  return validateForm(formData, rules)
}

/**
 * 表单卡片数据合规化处理
 * @param {Object} formData - 表单数据
 * @returns {Object} 处理后的表单数据
 */
export const sanitizeFormCard = (formData) => {
  return {
    ...formData,
    title: formData.title?.trim() || '',
    value: formData.value?.trim() || '',
    remark: formData.remark?.trim() || ''
  }
}
