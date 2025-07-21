<template>
  <div class="async-error">
    <div class="error-icon">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="48" height="48" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="10" />
        <line x1="12" y1="8" x2="12" y2="12" />
        <line x1="12" y1="16" x2="12.01" y2="16" />
      </svg>
    </div>
    <h3>组件加载失败</h3>
    <p>无法加载请求的组件，请检查您的网络连接</p>
    <button class="retry-button" @click="retryLoading">重试</button>
  </div>
</template>

<script setup>
import { onMounted } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()

const retryLoading = () => {
  // 刷新当前路由
  const currentRoute = router.currentRoute.value
  router.replace({
    path: '/redirect',
    query: { path: currentRoute.fullPath }
  }).catch(() => {
    // 直接刷新页面作为后备方案
    window.location.reload()
  })
}

onMounted(() => {
  console.error('组件加载失败，显示错误组件')
})
</script>

<style scoped>
.async-error {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 32px;
  text-align: center;
  color: var(--danger-color);
  background-color: rgba(245, 108, 108, 0.1);
  border-radius: 8px;
  margin: 20px 0;
}

.error-icon {
  margin-bottom: 16px;
  color: var(--danger-color);
}

h3 {
  margin: 0 0 8px;
  font-size: 18px;
}

p {
  margin: 0 0 16px;
  color: var(--text-regular);
}

.retry-button {
  padding: 8px 16px;
  background-color: var(--danger-color);
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;
}

.retry-button:hover {
  background-color: var(--danger-color-hover);
}
</style>
