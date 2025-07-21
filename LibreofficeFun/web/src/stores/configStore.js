import { defineStore } from 'pinia'
import { ref, watch } from 'vue'
import { getLocale, setLocale } from '../utils/i18n'

/**
 * 应用配置状态存储
 */
export const useConfigStore = defineStore('config', () => {
  // 状态
  const locale = ref(getLocale())
  const theme = ref(localStorage.getItem('theme') || 'light')
  const debugMode = ref(localStorage.getItem('debug-mode') === 'true')

  // 用户界面配置
  const uiConfig = ref({
    sidebarCollapsed: localStorage.getItem('sidebar-collapsed') === 'true',
    fontSize: localStorage.getItem('font-size') || 'medium', // small, medium, large
    compactMode: localStorage.getItem('compact-mode') === 'true',
    animationsEnabled: localStorage.getItem('animations-enabled') !== 'false', // 默认开启
  })

  // 监听并保存配置变化
  watch(locale, (newLocale) => {
    setLocale(newLocale)
    localStorage.setItem('locale', newLocale)
  })

  watch(theme, (newTheme) => {
    localStorage.setItem('theme', newTheme)
    applyTheme(newTheme)
  })

  watch(debugMode, (enabled) => {
    localStorage.setItem('debug-mode', enabled.toString())
  })

  watch(uiConfig, (newConfig) => {
    localStorage.setItem('sidebar-collapsed', newConfig.sidebarCollapsed.toString())
    localStorage.setItem('font-size', newConfig.fontSize)
    localStorage.setItem('compact-mode', newConfig.compactMode.toString())
    localStorage.setItem('animations-enabled', newConfig.animationsEnabled.toString())
    applyUIConfig(newConfig)
  }, { deep: true })

  // 设置语言
  const setAppLocale = (newLocale) => {
    if (['en-US', 'zh-CN'].includes(newLocale)) {
      locale.value = newLocale
      return true
    }
    return false
  }

  // 设置主题
  const setTheme = (newTheme) => {
    if (['light', 'dark', 'system'].includes(newTheme)) {
      theme.value = newTheme
      return true
    }
    return false
  }

  // 切换调试模式
  const toggleDebugMode = () => {
    debugMode.value = !debugMode.value
    return debugMode.value
  }

  // 切换侧边栏
  const toggleSidebar = () => {
    uiConfig.value.sidebarCollapsed = !uiConfig.value.sidebarCollapsed
    return uiConfig.value.sidebarCollapsed
  }

  // 设置字体大小
  const setFontSize = (size) => {
    if (['small', 'medium', 'large'].includes(size)) {
      uiConfig.value.fontSize = size
      return true
    }
    return false
  }

  // 切换紧凑模式
  const toggleCompactMode = () => {
    uiConfig.value.compactMode = !uiConfig.value.compactMode
    return uiConfig.value.compactMode
  }

  // 切换动画
  const toggleAnimations = () => {
    uiConfig.value.animationsEnabled = !uiConfig.value.animationsEnabled
    return uiConfig.value.animationsEnabled
  }

  // 应用主题
  const applyTheme = (themeName) => {
    let effectiveTheme = themeName

    // 如果是系统主题，则检测系统偏好
    if (themeName === 'system') {
      effectiveTheme = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
    }

    // 应用主题类到文档
    document.documentElement.classList.remove('theme-light', 'theme-dark')
    document.documentElement.classList.add(`theme-${effectiveTheme}`)

    // 更新meta主题颜色
    const themeColor = effectiveTheme === 'dark' ? '#1a1a1a' : '#ffffff'
    const metaTheme = document.querySelector('meta[name="theme-color"]')

    if (metaTheme) {
      metaTheme.setAttribute('content', themeColor)
    } else {
      const meta = document.createElement('meta')
      meta.name = 'theme-color'
      meta.content = themeColor
      document.head.appendChild(meta)
    }
  }

  // 应用UI配置
  const applyUIConfig = (config) => {
    // 应用字体大小
    document.documentElement.style.fontSize = {
      'small': '14px',
      'medium': '16px',
      'large': '18px'
    }[config.fontSize] || '16px'

    // 应用紧凑模式
    document.documentElement.classList.toggle('compact-mode', config.compactMode)

    // 应用动画设置
    document.documentElement.classList.toggle('no-animations', !config.animationsEnabled)
  }

  // 重置所有配置
  const resetAllConfig = () => {
    locale.value = 'en-US'
    theme.value = 'light'
    debugMode.value = false
    uiConfig.value = {
      sidebarCollapsed: false,
      fontSize: 'medium',
      compactMode: false,
      animationsEnabled: true
    }
  }

  // 初始化应用配置
  const initialize = () => {
    applyTheme(theme.value)
    applyUIConfig(uiConfig.value)

    // 监听系统主题变化
    if (window.matchMedia) {
      window.matchMedia('(prefers-color-scheme: dark)')
        .addEventListener('change', (e) => {
          if (theme.value === 'system') {
            applyTheme('system')
          }
        })
    }
  }

  // 执行初始化
  initialize()

  return {
    // 状态
    locale,
    theme,
    debugMode,
    uiConfig,

    // 方法
    setAppLocale,
    setTheme,
    toggleDebugMode,
    toggleSidebar,
    setFontSize,
    toggleCompactMode,
    toggleAnimations,
    resetAllConfig
  }
})
