<template>
  <div class="app-error">
    <h2>发生错误</h2>
    <p>{{ message }}</p>
    <button @click="$emit('reload')">重新加载应用</button>
  </div>
</template>

<script>
export default {
  name: 'AppError',
  props: {
    message: {
      type: String,
      default: '应用加载失败'
    }
  },
  emits: ['reload']
}
</script>

<style scoped>
.app-error {
  text-align: center;
  padding: 40px 20px;
  max-width: 600px;
  margin: 0 auto;
}

h2 {
  color: #e74c3c;
  margin-bottom: 20px;
}

p {
  margin-bottom: 20px;
  color: #333;
  font-size: 16px;
  line-height: 1.5;
}

button {
  padding: 10px 20px;
  background-color: #e74c3c;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 16px;
  transition: background-color 0.3s;
}

button:hover {
  background-color: #c0392b;
}
</style>
