<template>
  <div class="not-found">
    <div class="not-found-content">
      <h1>404</h1>
      <h2>页面未找到</h2>
      <p>很抱歉，您访问的页面不存在或已被移动。</p>
      <div class="actions">
        <router-link to="/" class="btn-primary">返回首页</router-link>
        <button @click="goBack" class="btn-outline">返回上一页</button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { useRouter } from 'vue-router'

const router = useRouter()

const goBack = () => {
  router.back()
}
</script>

<style scoped>
.not-found {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 80vh;
  text-align: center;
  padding: 20px;
}

.not-found-content {
  max-width: 500px;
}

h1 {
  font-size: 120px;
  margin: 0;
  color: var(--primary-color);
  line-height: 1.2;
}

h2 {
  font-size: 28px;
  margin: 0 0 20px;
  color: var(--text-primary);
}

p {
  font-size: 16px;
  margin-bottom: 30px;
  color: var(--text-secondary);
}

.actions {
  display: flex;
  gap: 16px;
  justify-content: center;
}

.btn-primary {
  display: inline-block;
  background-color: var(--primary-color);
  color: white;
  padding: 10px 20px;
  border-radius: 4px;
  text-decoration: none;
  transition: background-color 0.3s;
}

.btn-primary:hover {
  background-color: var(--primary-color-hover);
}

.btn-outline {
  display: inline-block;
  background-color: transparent;
  color: var(--text-regular);
  padding: 10px 20px;
  border-radius: 4px;
  border: 1px solid var(--border-color);
  cursor: pointer;
  transition: all 0.3s;
}

.btn-outline:hover {
  border-color: var(--primary-color);
  color: var(--primary-color);
}
</style>
