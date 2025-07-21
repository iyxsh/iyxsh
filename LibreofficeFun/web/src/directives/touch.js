/**
 * 触摸交互指令
 * 为组件提供触摸手势支持，包括滑动、拖动、缩放等
 */

// 配置选项
const DEFAULT_OPTIONS = {
  thresholdX: 10, // X轴最小滑动距离阈值
  thresholdY: 10, // Y轴最小滑动距离阈值
  swipeVelocity: 0.3, // 滑动速度阈值 (px/ms)
  longPressDelay: 600, // 长按延迟 (ms)
  doubleTapDelay: 300, // 双击延迟 (ms)
  preventDefaultEvents: false // 是否阻止默认事件
}

/**
 * 获取两点之间的距离
 */
const getDistance = (p1, p2) => {
  const dx = p2.clientX - p1.clientX
  const dy = p2.clientY - p1.clientY
  return Math.sqrt(dx * dx + dy * dy)
}

/**
 * 获取两点之间的角度
 */
const getAngle = (p1, p2) => {
  const dx = p2.clientX - p1.clientX
  const dy = p2.clientY - p1.clientY
  return Math.atan2(dy, dx) * 180 / Math.PI
}

/**
 * 实现触摸指令
 */
export const touch = {
  mounted(el, binding) {
    const options = { ...DEFAULT_OPTIONS, ...(binding.value || {}) }

    // 触摸状态
    const state = {
      startTime: 0,
      endTime: 0,
      startX: 0,
      startY: 0,
      endX: 0,
      endY: 0,
      startDistance: 0,
      startAngle: 0,
      longPressTimer: null,
      lastTapTime: 0,
      isSwiping: false,
      isPinching: false,
      isRotating: false,
      touchCount: 0
    }

    // 清理长按定时器
    const clearLongPressTimer = () => {
      if (state.longPressTimer) {
        clearTimeout(state.longPressTimer)
        state.longPressTimer = null
      }
    }

    // 触摸开始事件处理
    const handleTouchStart = (event) => {
      // 记录触摸数量
      state.touchCount = event.touches.length

      // 单指触摸
      if (event.touches.length === 1) {
        // 记录起始位置和时间
        state.startX = event.touches[0].clientX
        state.startY = event.touches[0].clientY
        state.startTime = Date.now()

        // 设置长按定时器
        clearLongPressTimer()
        state.longPressTimer = setTimeout(() => {
          const detail = {
            x: state.startX,
            y: state.startY,
            timeStamp: Date.now() - state.startTime
          }

          const longPressEvent = new CustomEvent('longpress', { detail })
          el.dispatchEvent(longPressEvent)
        }, options.longPressDelay)
      }
      // 双指触摸
      else if (event.touches.length === 2) {
        // 清除单指触摸相关状态
        clearLongPressTimer()

        // 计算初始距离和角度
        state.startDistance = getDistance(event.touches[0], event.touches[1])
        state.startAngle = getAngle(event.touches[0], event.touches[1])
      }

      // 触发触摸开始事件
      const touchStartEvent = new CustomEvent('touchstart:passive', {
        detail: {
          originalEvent: event,
          touchCount: state.touchCount
        }
      })
      el.dispatchEvent(touchStartEvent)

      if (options.preventDefaultEvents) {
        event.preventDefault()
      }
    }

    // 触摸移动事件处理
    const handleTouchMove = (event) => {
      // 获取当前位置
      const currentX = event.touches[0].clientX
      const currentY = event.touches[0].clientY

      // 单指触摸移动
      if (event.touches.length === 1) {
        // 计算移动距离
        const deltaX = currentX - state.startX
        const deltaY = currentY - state.startY

        // 判断是否达到滑动阈值
        if (Math.abs(deltaX) > options.thresholdX || Math.abs(deltaY) > options.thresholdY) {
          // 取消长按
          clearLongPressTimer()

          // 设置为滑动状态
          state.isSwiping = true

          // 触发滑动中事件
          const panEvent = new CustomEvent('pan', {
            detail: {
              startX: state.startX,
              startY: state.startY,
              currentX,
              currentY,
              deltaX,
              deltaY,
              direction: Math.abs(deltaX) > Math.abs(deltaY)
                ? (deltaX > 0 ? 'right' : 'left')
                : (deltaY > 0 ? 'down' : 'up')
            }
          })
          el.dispatchEvent(panEvent)
        }
      }
      // 双指触摸移动
      else if (event.touches.length === 2) {
        // 计算当前距离和角度
        const currentDistance = getDistance(event.touches[0], event.touches[1])
        const currentAngle = getAngle(event.touches[0], event.touches[1])

        // 计算缩放比例
        const scale = currentDistance / state.startDistance

        // 计算旋转角度
        const rotation = currentAngle - state.startAngle

        // 触发缩放事件
        if (Math.abs(scale - 1) > 0.05) {
          state.isPinching = true

          const pinchEvent = new CustomEvent('pinch', {
            detail: {
              scale,
              center: {
                x: (event.touches[0].clientX + event.touches[1].clientX) / 2,
                y: (event.touches[0].clientY + event.touches[1].clientY) / 2
              }
            }
          })
          el.dispatchEvent(pinchEvent)
        }

        // 触发旋转事件
        if (Math.abs(rotation) > 5) {
          state.isRotating = true

          const rotateEvent = new CustomEvent('rotate', {
            detail: {
              rotation,
              center: {
                x: (event.touches[0].clientX + event.touches[1].clientX) / 2,
                y: (event.touches[0].clientY + event.touches[1].clientY) / 2
              }
            }
          })
          el.dispatchEvent(rotateEvent)
        }
      }

      // 触发触摸移动事件
      const touchMoveEvent = new CustomEvent('touchmove:passive', {
        detail: {
          originalEvent: event,
          touchCount: event.touches.length,
          currentX,
          currentY
        }
      })
      el.dispatchEvent(touchMoveEvent)

      if (options.preventDefaultEvents || state.isSwiping || state.isPinching || state.isRotating) {
        event.preventDefault()
      }
    }

    // 触摸结束事件处理
    const handleTouchEnd = (event) => {
      // 记录结束位置和时间
      state.endX = event.changedTouches[0].clientX
      state.endY = event.changedTouches[0].clientY
      state.endTime = Date.now()

      // 计算移动距离和时间
      const deltaX = state.endX - state.startX
      const deltaY = state.endY - state.startY
      const deltaTime = state.endTime - state.startTime

      // 计算移动速度
      const velocityX = Math.abs(deltaX) / deltaTime
      const velocityY = Math.abs(deltaY) / deltaTime

      // 清除长按定时器
      clearLongPressTimer()

      // 处理单击/双击
      if (!state.isSwiping && !state.isPinching && !state.isRotating) {
        // 检测双击
        const now = Date.now()
        const timeSinceLastTap = now - state.lastTapTime

        if (timeSinceLastTap < options.doubleTapDelay) {
          // 双击事件
          const doubleTapEvent = new CustomEvent('doubletap', {
            detail: {
              x: state.endX,
              y: state.endY
            }
          })
          el.dispatchEvent(doubleTapEvent)

          // 重置最后点击时间
          state.lastTapTime = 0
        } else {
          // 单击事件
          const tapEvent = new CustomEvent('tap', {
            detail: {
              x: state.endX,
              y: state.endY
            }
          })
          el.dispatchEvent(tapEvent)

          // 记录本次点击时间，用于检测双击
          state.lastTapTime = now
        }
      }

      // 处理滑动结束
      if (state.isSwiping) {
        // 计算滑动方向
        const isHorizontal = Math.abs(deltaX) > Math.abs(deltaY)
        const direction = isHorizontal
          ? (deltaX > 0 ? 'right' : 'left')
          : (deltaY > 0 ? 'down' : 'up')

        // 判断是否满足快速滑动条件
        const isSwipe = (isHorizontal ? velocityX : velocityY) > options.swipeVelocity

        if (isSwipe) {
          // 快速滑动事件
          const swipeEvent = new CustomEvent(`swipe${direction}`, {
            detail: {
              distance: isHorizontal ? Math.abs(deltaX) : Math.abs(deltaY),
              duration: deltaTime,
              velocity: isHorizontal ? velocityX : velocityY
            }
          })
          el.dispatchEvent(swipeEvent)

          // 通用滑动事件
          const swipeGenericEvent = new CustomEvent('swipe', {
            detail: {
              direction,
              distance: isHorizontal ? Math.abs(deltaX) : Math.abs(deltaY),
              duration: deltaTime,
              velocity: isHorizontal ? velocityX : velocityY
            }
          })
          el.dispatchEvent(swipeGenericEvent)
        }

        // 触发滑动结束事件
        const panEndEvent = new CustomEvent('panend', {
          detail: {
            startX: state.startX,
            startY: state.startY,
            endX: state.endX,
            endY: state.endY,
            deltaX,
            deltaY,
            duration: deltaTime,
            isSwipe
          }
        })
        el.dispatchEvent(panEndEvent)
      }

      // 处理缩放结束
      if (state.isPinching) {
        const pinchEndEvent = new CustomEvent('pinchend', {
          detail: {
            duration: deltaTime
          }
        })
        el.dispatchEvent(pinchEndEvent)
      }

      // 处理旋转结束
      if (state.isRotating) {
        const rotateEndEvent = new CustomEvent('rotateend', {
          detail: {
            duration: deltaTime
          }
        })
        el.dispatchEvent(rotateEndEvent)
      }

      // 触发触摸结束事件
      const touchEndEvent = new CustomEvent('touchend:passive', {
        detail: {
          originalEvent: event,
          touchCount: state.touchCount
        }
      })
      el.dispatchEvent(touchEndEvent)

      // 重置状态
      state.isSwiping = false
      state.isPinching = false
      state.isRotating = false
      state.touchCount = 0

      if (options.preventDefaultEvents) {
        event.preventDefault()
      }
    }

    // 触摸取消事件处理
    const handleTouchCancel = (event) => {
      clearLongPressTimer()

      // 重置状态
      state.isSwiping = false
      state.isPinching = false
      state.isRotating = false
      state.touchCount = 0

      // 触发触摸取消事件
      const touchCancelEvent = new CustomEvent('touchcancel:passive', {
        detail: {
          originalEvent: event
        }
      })
      el.dispatchEvent(touchCancelEvent)

      if (options.preventDefaultEvents) {
        event.preventDefault()
      }
    }

    // 绑定事件
    el.addEventListener('touchstart', handleTouchStart, { passive: !options.preventDefaultEvents })
    el.addEventListener('touchmove', handleTouchMove, { passive: !options.preventDefaultEvents })
    el.addEventListener('touchend', handleTouchEnd, { passive: !options.preventDefaultEvents })
    el.addEventListener('touchcancel', handleTouchCancel, { passive: !options.preventDefaultEvents })

    // 保存事件处理函数和状态，以便在 unmounted 中解绑
    el._touchHandlers = {
      handleTouchStart,
      handleTouchMove,
      handleTouchEnd,
      handleTouchCancel,
      options
    }
  },

  unmounted(el) {
    // 解绑事件
    if (el._touchHandlers) {
      const { handleTouchStart, handleTouchMove, handleTouchEnd, handleTouchCancel, options } = el._touchHandlers

      el.removeEventListener('touchstart', handleTouchStart, { passive: !options.preventDefaultEvents })
      el.removeEventListener('touchmove', handleTouchMove, { passive: !options.preventDefaultEvents })
      el.removeEventListener('touchend', handleTouchEnd, { passive: !options.preventDefaultEvents })
      el.removeEventListener('touchcancel', handleTouchCancel, { passive: !options.preventDefaultEvents })

      delete el._touchHandlers
    }
  }
}

// 注册指令
export default {
  install(app) {
    app.directive('touch', touch)
  }
}
