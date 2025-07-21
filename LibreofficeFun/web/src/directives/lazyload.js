/**
 * 图片懒加载指令
 * 用于优化图片加载性能
 */

// 存储所有需要懒加载的元素
const lazyElements = new Set()

// 观察器实例
let observer = null

// 配置
const defaultOptions = {
  rootMargin: '50px 0px',
  threshold: 0.1,
  placeholder: 'data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="%23ccc" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><circle cx="8.5" cy="8.5" r="1.5"></circle><polyline points="21 15 16 10 5 21"></polyline></svg>'
}

/**
 * 初始化观察器
 */
function setupIntersectionObserver(options = {}) {
  if (observer) return

  const config = { ...defaultOptions, ...options }

  observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        const element = entry.target

        // 加载图片
        loadImage(element)

        // 停止观察这个元素
        observer.unobserve(element)
        lazyElements.delete(element)
      }
    })
  }, {
    rootMargin: config.rootMargin,
    threshold: config.threshold
  })

  // 开始观察所有元素
  lazyElements.forEach(element => {
    observer.observe(element)
  })
}

/**
 * 加载图片
 * @param {HTMLElement} element - 需要加载图片的元素
 */
function loadImage(element) {
  const src = element.dataset.src
  const srcset = element.dataset.srcset

  if (!src && !srcset) return

  // 创建一个新图片用于预加载
  const img = new Image()

  // 设置加载事件
  img.onload = () => {
    if (src) element.src = src
    if (srcset) element.srcset = srcset

    // 更新加载状态
    element.classList.add('lazy-loaded')
    element.classList.remove('lazy-loading')

    // 移除过渡类，避免抖动
    setTimeout(() => {
      element.classList.remove('lazy-transition')
    }, 500)

    // 触发自定义事件
    element.dispatchEvent(new CustomEvent('lazyloaded'))
  }

  // 设置错误事件
  img.onerror = () => {
    // 添加错误类
    element.classList.add('lazy-error')
    element.classList.remove('lazy-loading')

    // 触发自定义事件
    element.dispatchEvent(new CustomEvent('lazyloaderror'))
  }

  // 设置正在加载类
  element.classList.add('lazy-loading')
  element.classList.add('lazy-transition')

  // 开始加载
  if (srcset) img.srcset = srcset
  if (src) img.src = src
}

// 创建懒加载指令
export const lazyload = {
  mounted(el, binding) {
    // 获取配置
    const options = binding.value || {}

    // 保存原始链接
    if (el.tagName === 'IMG') {
      // 图片元素
      el.dataset.src = el.getAttribute('src') || ''

      if (el.getAttribute('srcset')) {
        el.dataset.srcset = el.getAttribute('srcset')
      }

      // 设置占位图
      el.setAttribute('src', options.placeholder || defaultOptions.placeholder)
      if (el.hasAttribute('srcset')) {
        el.removeAttribute('srcset')
      }
    } else if (el.tagName === 'DIV' || el.tagName === 'SECTION') {
      // 背景图片元素
      const bg = el.style.backgroundImage
      const urlMatch = bg.match(/url\(['"]?(.*?)['"]?\)/)

      if (urlMatch && urlMatch[1]) {
        el.dataset.bgSrc = urlMatch[1]
        el.style.backgroundImage = `url('${options.placeholder || defaultOptions.placeholder}')`
      }
    }

    // 添加到懒加载集合
    lazyElements.add(el)

    // 初始化观察器
    setupIntersectionObserver(options)

    // 观察元素
    if (observer) {
      observer.observe(el)
    }
  },

  updated(el, binding) {
    // 检查源是否已更新
    const newSrc = binding.value && binding.value.src
    const oldSrc = el.dataset.src

    if (newSrc && newSrc !== oldSrc) {
      // 更新数据源
      el.dataset.src = newSrc

      // 如果元素已加载，直接更新
      if (el.classList.contains('lazy-loaded')) {
        if (el.tagName === 'IMG') {
          el.src = newSrc
        } else {
          el.style.backgroundImage = `url('${newSrc}')`
        }
      }
      // 否则，重新观察元素
      else if (!lazyElements.has(el) && observer) {
        lazyElements.add(el)
        observer.observe(el)
      }
    }
  },

  unmounted(el) {
    // 停止观察元素
    if (observer) {
      observer.unobserve(el)
    }

    // 从集合中移除
    lazyElements.delete(el)
  }
}

// 导出指令
export default {
  install(app) {
    app.directive('lazyload', lazyload)
  }
}

// 导出工具函数
export const LazyLoadUtils = {
  // 手动观察元素
  observe(el, options = {}) {
    if (el) {
      lazyElements.add(el)
      setupIntersectionObserver(options)
      if (observer) {
        observer.observe(el)
      }
    }
  },

  // 手动加载元素
  load(el) {
    if (el) {
      loadImage(el)
      if (observer) {
        observer.unobserve(el)
      }
      lazyElements.delete(el)
    }
  },

  // 手动卸载观察器
  disconnect() {
    if (observer) {
      observer.disconnect()
      observer = null
    }
    lazyElements.clear()
  }
}
