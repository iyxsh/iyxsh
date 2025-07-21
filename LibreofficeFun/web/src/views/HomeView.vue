<template>
  <div class="home-view">
    <h1>Home Page</h1>
    <p>Welcome to the LibreOfficeFun application!</p>
    <!-- 添加导航链接 -->
    <div class="navigation">
      <router-link to="/about">Go to About Page</router-link>
      <router-link to="/page">Go to Page Manager</router-link>
    </div>
    <!-- 添加功能描述 -->
    <div class="features">
      <h2>Features</h2>
      <ul>
        <li>Create and manage forms</li>
        <li>Convert forms to cards</li>
        <li>Customize card styles</li>
        <li>Support for multiple languages</li>
      </ul>
    </div>
  </div>
</template>

<script>
export default {
  name: 'HomeView',
  // 添加mounted钩子进行调试信息输出
  mounted() {
    console.log('HomeView component mounted');
  } 
}
</script>

<style scoped>
.home-view {
  padding: 20px;
  text-align: center;
}

.navigation {
  margin-top: 20px;
}

.navigation a {
  margin: 0 10px;
  text-decoration: none;
  color: #42b883;
}

.navigation a:hover {
  text-decoration: underline;
}

.features {
  margin-top: 30px;
}

.features ul {
  list-style-type: none;
  padding: 0;
}

.features li {
  margin: 10px 0;
}
</style>