/**
 * 数据处理Web Worker
 * 用于处理大量计算，避免阻塞主线程
 */

// 计算表单数据统计信息
function calculateFormStats(forms) {
  // 模拟耗时计算
  const startTime = performance.now()

  if (!forms || !Array.isArray(forms) || forms.length === 0) {
    return {
      count: 0,
      stats: {},
      processingTime: 0
    }
  }

  // 字段统计
  const fieldCounts = {
    withTitle: 0,
    withValue: 0,
    withRemark: 0,
    withMedia: 0,
    empty: 0
  }

  // 内容长度统计
  const lengthStats = {
    titleLengths: [],
    valueLengths: [],
    remarkLengths: []
  }

  // 计算每个表单的统计信息
  forms.forEach(form => {
    // 计算字段是否填写
    if (form.title && form.title.trim()) fieldCounts.withTitle++
    if (form.value && form.value.trim()) fieldCounts.withValue++
    if (form.remark && form.remark.trim()) fieldCounts.withRemark++
    if (form.media) fieldCounts.withMedia++

    // 判断表单是否为空
    if (
      (!form.title || !form.title.trim()) && 
      (!form.value || !form.value.trim()) && 
      (!form.remark || !form.remark.trim()) &&
      !form.media
    ) {
      fieldCounts.empty++
    }

    // 计算内容长度
    if (form.title) lengthStats.titleLengths.push(form.title.length)
    if (form.value) lengthStats.valueLengths.push(form.value.length)
    if (form.remark) lengthStats.remarkLengths.push(form.remark.length)
  })

  // 计算平均长度
  const calculateAverage = (arr) => arr.length ? (arr.reduce((a, b) => a + b, 0) / arr.length) : 0

  const averageLengths = {
    title: calculateAverage(lengthStats.titleLengths),
    value: calculateAverage(lengthStats.valueLengths),
    remark: calculateAverage(lengthStats.remarkLengths)
  }

  // 计算完整度
  const completeness = {
    title: forms.length ? (fieldCounts.withTitle / forms.length) * 100 : 0,
    value: forms.length ? (fieldCounts.withValue / forms.length) * 100 : 0,
    remark: forms.length ? (fieldCounts.withRemark / forms.length) * 100 : 0,
    media: forms.length ? (fieldCounts.withMedia / forms.length) * 100 : 0,
    overall: forms.length ? 
      ((fieldCounts.withTitle + fieldCounts.withValue + fieldCounts.withRemark + fieldCounts.withMedia) / 
      (forms.length * 4)) * 100 : 0
  }

  // 计算处理时间
  const processingTime = performance.now() - startTime

  return {
    count: forms.length,
    stats: {
      fieldCounts,
      averageLengths,
      completeness,
      emptyPercentage: forms.length ? (fieldCounts.empty / forms.length) * 100 : 0
    },
    processingTime
  }
}

// 排序和过滤表单
function sortAndFilterForms(forms, options) {
  const { sortBy, sortOrder, filters } = options

  if (!forms || !Array.isArray(forms) || forms.length === 0) {
    return {
      forms: [],
      processingTime: 0
    }
  }

  const startTime = performance.now()

  // 首先应用过滤
  let result = [...forms]

  if (filters) {
    if (filters.text) {
      const searchText = filters.text.toLowerCase()
      result = result.filter(form => 
        (form.title && form.title.toLowerCase().includes(searchText)) ||
        (form.value && form.value.toLowerCase().includes(searchText)) ||
        (form.remark && form.remark.toLowerCase().includes(searchText))
      )
    }

    if (filters.hasMedia !== undefined) {
      result = result.filter(form => 
        filters.hasMedia ? !!form.media : !form.media
      )
    }

    if (filters.isEmpty !== undefined) {
      result = result.filter(form => {
        const isEmpty = 
          (!form.title || !form.title.trim()) && 
          (!form.value || !form.value.trim()) && 
          (!form.remark || !form.remark.trim()) &&
          !form.media

        return filters.isEmpty ? isEmpty : !isEmpty
      })
    }
  }

  // 然后应用排序
  if (sortBy) {
    const direction = sortOrder === 'desc' ? -1 : 1

    result.sort((a, b) => {
      let valueA, valueB

      switch (sortBy) {
        case 'title':
          valueA = a.title || ''
          valueB = b.title || ''
          return direction * valueA.localeCompare(valueB)

        case 'value':
          valueA = a.value || ''
          valueB = b.value || ''
          return direction * valueA.localeCompare(valueB)

        case 'length':
          valueA = (a.title?.length || 0) + (a.value?.length || 0) + (a.remark?.length || 0)
          valueB = (b.title?.length || 0) + (b.value?.length || 0) + (b.remark?.length || 0)
          return direction * (valueA - valueB)

        case 'createdAt':
          valueA = a.createdAt ? new Date(a.createdAt).getTime() : 0
          valueB = b.createdAt ? new Date(b.createdAt).getTime() : 0
          return direction * (valueA - valueB)

        case 'updatedAt':
          valueA = a.updatedAt ? new Date(a.updatedAt).getTime() : 0
          valueB = b.updatedAt ? new Date(b.updatedAt).getTime() : 0
          return direction * (valueA - valueB)

        default:
          return 0
      }
    })
  }

  const processingTime = performance.now() - startTime

  return {
    forms: result,
    processingTime
  }
}

// 处理表单批量操作
function batchProcessForms(forms, operation) {
  const { type, params } = operation

  if (!forms || !Array.isArray(forms) || forms.length === 0) {
    return {
      forms: [],
      processingTime: 0
    }
  }

  const startTime = performance.now()

  // 创建表单副本
  let result = forms.map(form => ({ ...form }))

  switch (type) {
    case 'updateStyle':
      // 更新所有表单的样式
      result = result.map(form => ({
        ...form,
        ...params
      }))
      break

    case 'removeEmptyForms':
      // 移除空表单
      result = result.filter(form => 
        (form.title && form.title.trim()) || 
        (form.value && form.value.trim()) || 
        (form.remark && form.remark.trim()) ||
        form.media
      )
      break

    case 'normalizeTitles':
      // 标准化标题格式
      result = result.map(form => {
        if (form.title) {
          let title = form.title.trim()

          // 首字母大写
          if (params.capitalizeFirst) {
            title = title.charAt(0).toUpperCase() + title.slice(1)
          }

          // 移除多余空格
          if (params.removeExtraSpaces) {
            title = title.replace(/\s+/g, ' ')
          }

          return { ...form, title }
        }
        return form
      })
      break

    default:
      // 未知操作，返回原始表单
      break
  }

  const processingTime = performance.now() - startTime

  return {
    forms: result,
    processingTime
  }
}

// 事件监听器
self.addEventListener('message', (event) => {
  const { id, action, data } = event.data

  try {
    let result

    switch (action) {
      case 'calculateFormStats':
        result = calculateFormStats(data.forms)
        break

      case 'sortAndFilterForms':
        result = sortAndFilterForms(data.forms, data.options)
        break

      case 'batchProcessForms':
        result = batchProcessForms(data.forms, data.operation)
        break

      default:
        throw new Error(`Unknown action: ${action}`)
    }

    // 发送成功结果
    self.postMessage({
      id,
      success: true,
      result
    })
  } catch (error) {
    // 发送错误信息
    self.postMessage({
      id,
      success: false,
      error: {
        message: error.message,
        stack: error.stack
      }
    })
  }
})

// 通知主线程worker已准备就绪
self.postMessage({
  id: 'init',
  success: true,
  result: {
    status: 'ready',
    supportedActions: [
      'calculateFormStats',
      'sortAndFilterForms',
      'batchProcessForms'
    ]
  }
})
