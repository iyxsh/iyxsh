<template>
  <div class="about-view">
    <h1>About LibreOfficeFun</h1>
    <p>This application demonstrates form management and card conversion functionality.</p>
    
    <!-- 添加返回首页按钮 -->
    <div class="navigation">
      <router-link to="/">Back to Home</router-link>
    </div>
    
    <!-- 添加应用信息 -->
    <div class="app-info">
      <h2>Application Info</h2>
      <p>Version: 1.0.0</p>
      <p>Framework: Vue 3 with Composition API</p>
      <p>UI Library: Element Plus</p>
      <p>Features:</p>
      <ul>
        <li>Form creation and management</li>
        <li>Form to card conversion</li>
        <li>Card style customization</li>
        <li>Multi-language support</li>
      </ul>
    </div>
  </div>
</template>

<script>
export default {
  name: 'AboutView',
  // 添加mounted钩子进行调试信息输出
  mounted() {
    console.log('AboutView component mounted');
  }
}
</script>

<style scoped>
.about-view {
  padding: 20px;
}

.about-view h1 {
  color: #2c3e50;
  margin-bottom: 20px;
}

.navigation {
  margin-top: 20px;
}

.navigation a {
  text-decoration: none;
  color: #42b883;
}

.navigation a:hover {
  text-decoration: underline;
}

.app-info {
  margin-top: 30px;
}

.app-info ul {
  list-style-type: none;
  padding-left: 0;
}

.app-info li {
  margin: 10px 0;
}
</style>