<template>
  <div class="home-view">
    <!-- 顶部导航栏 -->
    <header class="header">
      <div class="container">
        <div class="header-content">
          <div class="logo">
            <img src="/logo.svg" alt="易雅熙世 Logo" class="logo-image">
            <h1 class="logo-text">易雅熙世</h1>
          </div>
          <!-- 导航链接 - 保留原有的导航链接 -->
          <nav class="navigation">
            <router-link to="/" class="nav-link active">首页</router-link>
            <router-link to="/about" class="nav-link">关于我们</router-link>
            <router-link to="/files" class="nav-link">文件管理</router-link>
            <button class="btn btn-primary">立即开始</button>
          </nav>
          <!-- 移动端菜单按钮 -->
          <div class="mobile-menu-btn">
            <el-icon @click="toggleMobileMenu"><Menu /></el-icon>
          </div>
        </div>
      </div>
      <!-- 移动端导航菜单 -->
      <div class="mobile-menu" v-if="showMobileMenu">
        <div class="mobile-menu-content">
          <router-link to="/" class="mobile-nav-link" @click="closeMobileMenu">首页</router-link>
          <router-link to="/about" class="mobile-nav-link" @click="closeMobileMenu">关于我们</router-link>
          <router-link to="/files" class="mobile-nav-link" @click="closeMobileMenu">文件管理</router-link>
          <button class="btn btn-primary" @click="closeMobileMenu">立即开始</button>
        </div>
      </div>
    </header>

    <!-- 英雄区域 -->
    <section class="hero">
      <div class="container">
        <div class="hero-content">
          <div class="hero-text">
            <h2>易见所想 易见所做 </h2>
            <p>这里提供强大而灵活的设想构建、管理和思维转换记录工具，让您的所想所做易于实现。</p>
            <div class="hero-actions">
              <button class="btn btn-primary btn-lg">开始使用</button>
              <button class="btn btn-outline btn-lg ml-4">了解更多</button>
            </div>
          </div>
          <div class="hero-image">
            <img src="https://picsum.photos/id/180/600/400" alt="LibreOfficeFun 产品展示" class="hero-img">
          </div>
        </div>
      </div>
    </section>

    <!-- 特性展示 -->
    <section class="features">
      <div class="container">
        <div class="section-header">
          <h3>强大功能</h3>
          <p>我们提供全面一直的构思广记工具，满足您的各种需求</p>
        </div>
        <div class="features-grid">
          <!-- 特性卡片 1 -->
          <div class="feature-card">
            <div class="feature-icon">
              <el-icon><Document /></el-icon>
            </div>
            <h4>构想构建与管理</h4>
            <p>轻松创建、编辑和管理各种类型的构想，支持拖拽定位和自定义样式。</p>
          </div>
          <!-- 特性卡片 2 -->
          <div class="feature-card">
            <div class="feature-icon">
              <el-icon><CollectionTag /></el-icon>
            </div>
            <h4>构想行文转换</h4>
            <p>一键将构想转换为精美的记录视图，提升构想展示效果和用户体验。</p>
          </div>
          <!-- 特性卡片 3 -->
          <div class="feature-card">
            <div class="feature-icon">
              <el-icon><Palette /></el-icon>
            </div>
            <h4>样式自定义</h4>
            <p>丰富的默认样式配置选项，超高自定义度，让您的构想与众不同，彰显品牌特色。</p>
          </div>
          <!-- 特性卡片 4 -->
          <div class="feature-card">
            <div class="feature-icon">
              <el-icon><Globe /></el-icon>
            </div>
            <h4>多语言支持</h4>
            <p>支持多种语言界面，满足全球化团队协作需求，消除语言障碍。</p>
          </div>
        </div>
      </div>
    </section>

    <!-- 数据统计 - 暂时注释不显示 -->
    <!-- <section class="stats">
      <div class="container">
        <div class="stats-grid">
          <div class="stat-item">
            <div class="stat-number">{{ formatNumber(10000) }}+</div>
            <div class="stat-label">用户数量</div>
          </div>
          <div class="stat-item">
            <div class="stat-number">{{ formatNumber(50000) }}+</div>
            <div class="stat-label">创建的构想</div>
          </div>
          <div class="stat-item">
            <div class="stat-number">{{ formatNumber(20) }}+</div>
            <div class="stat-label">支持的语言</div>
          </div>
          <div class="stat-item">
            <div class="stat-number">{{ formatNumber(99) }}%</div>
            <div class="stat-label">用户满意度</div>
          </div>
        </div>
      </div>
    </section> -->

    <!-- 合作伙伴 -->
    <section class="partners">
      <div class="container">
        <div class="section-header">
          <h3>值得信赖的合作伙伴</h3>
          <p>全球领先企业选择我们的解决方案</p>
        </div>
        <div class="partners-logos">
          <div class="partner-logo" v-for="i in 6" :key="i">
            <img :src="`/iyxshC.png`" alt="合作伙伴 Logo" class="logo-img">
          </div>
        </div>
      </div>
    </section>

    <!-- 号召性用语 -->
    <section class="cta">
      <div class="container">
        <div class="cta-content">
          <h3>准备好提升您的文档处理体验了吗？</h3>
          <p>加入成千上万的用户，体验 LibreOfficeFun 带来的全新工作方式。</p>
          <button class="btn btn-primary btn-lg">立即免费试用</button>
        </div>
      </div>
    </section>

    <!-- 页脚 -->
    <footer class="footer">
      <div class="container">
        <div class="footer-content">
          <div class="footer-logo">
            <h2>LibreOfficeFun</h2>
            <p>重新定义您的文档处理体验</p>
          </div>
          <div class="footer-links">
            <div class="footer-column">
              <h4>产品</h4>
              <ul>
                <li><a href="#">功能介绍</a></li>
                <li><a href="#">价格</a></li>
                <li><a href="#">更新日志</a></li>
                <li><a href="#">API 文档</a></li>
              </ul>
            </div>
            <div class="footer-column">
              <h4>资源</h4>
              <ul>
                <li><a href="#">帮助中心</a></li>
                <li><a href="#">教程</a></li>
                <li><a href="#">社区</a></li>
                <li><a href="#">常见问题</a></li>
              </ul>
            </div>
            <div class="footer-column">
              <h4>公司</h4>
              <ul>
                <li><a href="#">关于我们</a></li>
                <li><a href="#">联系我们</a></li>
                <li><a href="#">加入我们</a></li>
                <li><a href="#">隐私政策</a></li>
              </ul>
            </div>
          </div>
        </div>
        <div class="footer-bottom">
          <p>&copy; 2023 LibreOfficeFun. 保留所有权利。</p>
          <div class="social-links">
            <a href="#" class="social-link"><el-icon><Platform /></el-icon></a>
            <a href="#" class="social-link"><el-icon><Link /></el-icon></a>
            <a href="#" class="social-link"><el-icon><ChatDotRound /></el-icon></a>
            <a href="#" class="social-link"><el-icon><Share2 /></el-icon></a>
          </div>
        </div>
      </div>
    </footer>
  </div>
</template>

<script>
import { ref } from 'vue';
import { useRoute } from 'vue-router';

export default {
  name: 'HomeView',
  setup() {
    const route = useRoute();
    const showMobileMenu = ref(false);

    // 格式化数字显示
    const formatNumber = (num) => {
      return num.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ',');
    };

    // 切换移动端菜单
    const toggleMobileMenu = () => {
      showMobileMenu.value = !showMobileMenu.value;
    };

    // 关闭移动端菜单
    const closeMobileMenu = () => {
      showMobileMenu.value = false;
    };

    return {
      showMobileMenu,
      formatNumber,
      toggleMobileMenu,
      closeMobileMenu
    };
  }
};
</script>

<style scoped>
/* 基础样式 */
.home-view {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

/* 顶部导航 */
.header {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  background-color: white;
  box-shadow: var(--box-shadow-light);
  z-index: 1000;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 80px;
}

.logo {
  display: flex;
  align-items: center;
}

.logo-image {
  width: 40px;
  height: 40px;
  margin-right: 10px;
}

.logo-text {
  color: var(--primary-color);
  font-size: 24px;
  font-weight: 600;
  margin: 0;
}

.navigation {
  display: flex;
  align-items: center;
}

.nav-link {
  margin: 0 16px;
  color: var(--text-regular);
  text-decoration: none;
  font-weight: 500;
  transition: color var(--transition-duration);
}

.nav-link:hover,
.nav-link.active {
  color: var(--primary-color);
}

.mobile-menu-btn {
  display: none;
  cursor: pointer;
  padding: 8px;
}

.mobile-menu {
  position: fixed;
  top: 80px;
  left: 0;
  right: 0;
  background-color: white;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  z-index: 999;
}

.mobile-menu-content {
  padding: 16px;
  display: flex;
  flex-direction: column;
}

.mobile-nav-link {
  padding: 12px 0;
  color: var(--text-regular);
  text-decoration: none;
  border-bottom: 1px solid var(--border-color-light);
}

.mobile-nav-link:last-child {
  border-bottom: none;
}

/* 英雄区域 */
.hero {
  padding: 120px 0 80px;
  background: linear-gradient(135deg, var(--primary-color-light) 0%, #f0f9ff 100%);
  margin-top: 80px;
}

.hero-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.hero-text {
  flex: 1;
  max-width: 500px;
}

.hero-text h2 {
  font-size: 48px;
  font-weight: 600;
  color: var(--text-primary);
  margin-bottom: 20px;
  line-height: 1.2;
}

.hero-text p {
  font-size: 18px;
  color: var(--text-regular);
  margin-bottom: 32px;
  line-height: 1.6;
}

.hero-image {
  flex: 1;
  display: flex;
  justify-content: flex-end;
}

.hero-img {
  max-width: 100%;
  height: auto;
  border-radius: 12px;
  box-shadow: var(--box-shadow-base);
}

/* 特性展示 */
.features {
  padding: 80px 0;
}

.section-header {
  text-align: center;
  margin-bottom: 60px;
}

.section-header h3 {
  font-size: 32px;
  font-weight: 600;
  color: var(--text-primary);
  margin-bottom: 12px;
}

.section-header p {
  font-size: 16px;
  color: var(--text-secondary);
  max-width: 600px;
  margin: 0 auto;
}

.features-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 24px;
}

.feature-card {
  background-color: white;
  padding: 32px;
  border-radius: 12px;
  box-shadow: var(--box-shadow-light);
  transition: all var(--transition-duration);
  text-align: center;
}

.feature-card:hover {
  transform: translateY(-5px);
  box-shadow: var(--box-shadow-base);
}

.feature-icon {
  width: 64px;
  height: 64px;
  background-color: var(--primary-color-light);
  color: var(--primary-color);
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12px;
  margin: 0 auto 24px;
  font-size: 24px;
}

.feature-card h4 {
  font-size: 20px;
  font-weight: 600;
  color: var(--text-primary);
  margin-bottom: 12px;
}

.feature-card p {
  font-size: 14px;
  color: var(--text-regular);
  line-height: 1.6;
}

/* 数据统计 */
.stats {
  background-color: var(--primary-color-light);
  padding: 60px 0;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 40px;
}

.stat-item {
  text-align: center;
}

.stat-number {
  font-size: 36px;
  font-weight: 700;
  color: var(--primary-color);
  margin-bottom: 8px;
}

.stat-label {
  font-size: 14px;
  color: var(--text-regular);
  font-weight: 500;
}

/* 合作伙伴 */
.partners {
  padding: 80px 0;
}

.partners-logos {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 40px;
  align-items: center;
}

.partner-logo {
  display: flex;
  align-items: center;
  justify-content: center;
}

.logo-img {
  max-width: 100%;
  height: auto;
  opacity: 0.7;
  transition: opacity var(--transition-duration);
}

.logo-img:hover {
  opacity: 1;
}

/* 号召性用语 */
.cta {
  background: linear-gradient(135deg, var(--primary-color) 0%, var(--primary-color-hover) 100%);
  padding: 80px 0;
  color: white;
}

.cta-content {
  text-align: center;
  max-width: 800px;
  margin: 0 auto;
}

.cta-content h3 {
  font-size: 32px;
  font-weight: 600;
  margin-bottom: 16px;
}

.cta-content p {
  font-size: 18px;
  margin-bottom: 32px;
  opacity: 0.9;
}

.cta-content .btn-primary {
  background-color: white;
  color: var(--primary-color);
  border: none;
}

.cta-content .btn-primary:hover {
  background-color: var(--primary-color-light);
}

/* 页脚 */
.footer {
  background-color: var(--secondary-color);
  color: white;
  padding: 80px 0 40px;
}

.footer-content {
  display: flex;
  justify-content: space-between;
  margin-bottom: 40px;
}

.footer-logo h2 {
  font-size: 24px;
  font-weight: 600;
  margin-bottom: 8px;
}

.footer-logo p {
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
}

.footer-links {
  display: flex;
  gap: 60px;
}

.footer-column h4 {
  font-size: 16px;
  font-weight: 600;
  margin-bottom: 16px;
}

.footer-column ul {
  list-style: none;
}

.footer-column li {
  margin-bottom: 12px;
}

.footer-column a {
  color: rgba(255, 255, 255, 0.7);
  text-decoration: none;
  transition: color var(--transition-duration);
}

.footer-column a:hover {
  color: white;
}

.footer-bottom {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 20px;
  border-top: 1px solid rgba(255, 255, 255, 0.1);
}

.footer-bottom p {
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
}

.social-links {
  display: flex;
  gap: 16px;
}

.social-link {
  color: rgba(255, 255, 255, 0.7);
  text-decoration: none;
  transition: color var(--transition-duration);
}

.social-link:hover {
  color: white;
}

/* 响应式设计 */
@media (max-width: 1200px) {
  .features-grid {
    grid-template-columns: repeat(2, 1fr);
  }
  
  .stats-grid {
    grid-template-columns: repeat(2, 1fr);
    gap: 30px;
  }
  
  .partners-logos {
    grid-template-columns: repeat(3, 1fr);
  }
}

@media (max-width: 768px) {
  .navigation {
    display: none;
  }
  
  .mobile-menu-btn {
    display: block;
  }
  
  .hero-content {
    flex-direction: column;
    text-align: center;
  }
  
  .hero-text h2 {
    font-size: 36px;
  }
  
  .hero-image {
    margin-top: 40px;
    justify-content: center;
  }
  
  .features-grid {
    grid-template-columns: 1fr;
  }
  
  .partners-logos {
    grid-template-columns: repeat(2, 1fr);
  }
  
  .footer-content {
    flex-direction: column;
  }
  
  .footer-links {
    margin-top: 40px;
    flex-direction: column;
    gap: 30px;
  }
  
  .footer-bottom {
    flex-direction: column;
    text-align: center;
  }
  
  .social-links {
    margin-top: 16px;
  }
}

@media (max-width: 480px) {
  .hero-text h2 {
    font-size: 28px;
  }
  
  .hero-text p {
    font-size: 16px;
  }
  
  .section-header h3 {
    font-size: 24px;
  }
  
  .partners-logos {
    grid-template-columns: 1fr;
  }
}
</style>