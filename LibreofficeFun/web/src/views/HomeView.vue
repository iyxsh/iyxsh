<template>
  <div class="home-view">
    <!-- 顶部导航栏 -->
    <header class="header" :class="{ scrolled: scrolled }">
      <div class="container">
        <div class="header-content">
          <div class="logo">
            <img src="/logo.svg" alt="易雅熙世 Logo" class="logo-image">
            <h1 class="logo-text">易雅熙世</h1>
          </div>
          <!-- 导航链接 -->
          <nav class="navigation">
            <router-link 
              to="/" 
              class="nav-link" 
              :class="{ active: isActiveRoute('/') }"
              @click="closeMobileMenu"
              aria-current="page"
              aria-label="首页"
            >
              首页
            </router-link>
            <router-link 
              to="/about" 
              class="nav-link" 
              :class="{ active: isActiveRoute('/about') }"
              @click="closeMobileMenu"
            >
              关于我们
            </router-link>
            <router-link 
              to="/files" 
              class="nav-link" 
              :class="{ active: isActiveRoute('/files') }"
              @click="closeMobileMenu"
            >
              文件管理
            </router-link>
            <button class="btn btn-primary" @click="scrollToSection('cta')">
              立即开始
            </button>
          </nav>
          <!-- 移动端菜单按钮 -->
          <button class="mobile-menu-btn" 
                  @click="toggleMobileMenu" 
                  aria-expanded="false" 
                  :aria-expanded="showMobileMenu.toString()"
                  aria-controls="mobile-menu"
                  aria-label="打开菜单">
            <el-icon><Menu /></el-icon>
          </button>
        </div>
      </div>
      <!-- 移动端导航菜单 -->
      <div class="mobile-menu" v-if="showMobileMenu" id="mobile-menu" role="navigation" aria-label="移动端导航">
        <div class="mobile-menu-content">
          <router-link to="/" class="mobile-nav-link" @click="closeMobileMenu">首页</router-link>
          <router-link to="/about" class="mobile-nav-link" @click="closeMobileMenu">关于我们</router-link>
          <router-link to="/files" class="mobile-nav-link" @click="closeMobileMenu">文件管理</router-link>
          <button class="btn btn-primary" @click="scrollToSection('cta')">立即开始</button>
        </div>
      </div>
    </header>

    <!-- 英雄区域 -->
    <section class="hero animate-on-load" id="hero">
      <div class="container">
        <div class="hero-content">
          <div class="hero-text">
            <h2>易见所想 易见所做</h2>
            <p>这里提供强大而灵活的设想构建、管理和思维转换记录工具，让您的所想所做易于实现。</p>
            <div class="hero-actions">
              <button class="btn btn-primary btn-lg" @click="scrollToSection('features')">开始使用</button>
              <button class="btn btn-outline btn-lg" @click="scrollToSection('partners')">了解更多</button>
            </div>
           </div>
          <!-- 图片视频轮播组件 -->
          <div class="hero-carousel">
            <div class="carousel-container" ref="carouselContainer">
              <!-- 轮播内容 -->
              <div class="carousel-slide" v-if="carouselItems[currentSlide]" @click="openMediaPreview(carouselItems[currentSlide])">
                <!-- 统一使用缩略图显示，点击后再打开完整资源 -->
                <div class="thumbnail-container cursor-pointer">
                  <!-- 图片缩略图 -->
                  <img 
                    v-if="carouselItems[currentSlide].type === 'image'"
                    :src="carouselItems[currentSlide].path"
                    :alt="carouselItems[currentSlide].alt"
                    class="carousel-thumbnail"
                    loading="lazy"
                  >
                  
                  <!-- 视频缩略图 (使用poster) -->
                  <div v-else-if="carouselItems[currentSlide].type === 'video'" class="video-thumbnail-wrapper">
                    <img 
                      :src="carouselItems[currentSlide].poster || '/main.png'"
                      :alt="carouselItems[currentSlide].alt"
                      class="carousel-thumbnail"
                      loading="lazy"
                    >
                    <div class="video-overlay">
                      <div class="play-icon">▶</div>
                    </div>
                  </div>
                </div>
                <div class="carousel-caption">
                  {{ carouselItems[currentSlide].caption }}
                </div>
              </div>
            </div>
            
            <!-- 轮播控制按钮 -->
            <button class="carousel-control prev" @click.stop="prevSlide">
              <span>&lt;</span>
            </button>
            <button class="carousel-control next" @click.stop="nextSlide">
              <span>&gt;</span>
            </button>
            
            <!-- 轮播指示器 -->
            <div class="carousel-indicators">
              <button 
                v-for="(item, index) in carouselItems" 
                :key="index"
                :class="{ active: index === currentSlide }"
                @click.stop="goToSlide(index)"
                :aria-label="`转到轮播项 ${index + 1}`"
              ></button>
            </div>
            
            <!-- 播放/暂停控制 -->
            <button class="carousel-play-pause" @click.stop="togglePlay" :aria-label="isPlaying ? '暂停轮播' : '开始轮播'">
              {{ isPlaying ? '⏸️' : '▶️' }}
            </button>
          </div>
        </div>
      </div>
    </section>

    <!-- 特性展示 -->
    <section class="features animate-on-scroll" id="features">
      <div class="container">
        <div class="section-header">
          <h3>强大功能</h3>
          <p>我们提供全面一致的构思广记工具，满足您的各种需求</p>
        </div>
        <div class="features-grid">
          <!-- 特性卡片 1 -->
          <div class="feature-card animate-on-scroll">
            <div class="feature-icon">
              <el-icon><Check /></el-icon>
            </div>
            <h4>构想构建与管理</h4>
            <p>轻松创建、编辑和管理各种类型的构想，支持拖拽定位和自定义样式。</p>
          </div>
          <!-- 特性卡片 2 -->
          <div class="feature-card animate-on-scroll">
            <div class="feature-icon">
              <el-icon><Warning /></el-icon>
            </div>
            <h4>构想行文转换</h4>
            <p>一键将构想转换为精美的记录视图，提升构想展示效果和用户体验。</p>
          </div>
          <!-- 特性卡片 3 -->
          <div class="feature-card animate-on-scroll">
            <div class="feature-icon">
              <el-icon><Star /></el-icon>
            </div>
            <h4>样式自定义</h4>
            <p>丰富的默认样式配置选项，超高自定义度，让您的构想与众不同，彰显品牌特色。</p>
          </div>
          <!-- 特性卡片 4 -->
          <div class="feature-card animate-on-scroll">
            <div class="feature-icon">
              <el-icon><Help /></el-icon>
            </div>
            <h4>多语言支持</h4>
            <p>支持多种语言界面，满足全球化团队协作需求，消除语言障碍。</p>
          </div>
        </div>
      </div>
    </section>

    <!-- 数据统计 - 暂时注释不显示 -->
    <!-- <section class="stats animate-on-scroll">
      <div class="container">
        <div class="stats-grid">
          <div class="stat-item animate-on-scroll">
            <div class="stat-number">{{ formatNumber(10000) }}+</div>
            <div class="stat-label">用户数量</div>
          </div>
          <div class="stat-item animate-on-scroll">
            <div class="stat-number">{{ formatNumber(50000) }}+</div>
            <div class="stat-label">创建的构想</div>
          </div>
          <div class="stat-item animate-on-scroll">
            <div class="stat-number">{{ formatNumber(20) }}+</div>
            <div class="stat-label">支持的语言</div>
          </div>
          <div class="stat-item animate-on-scroll">
            <div class="stat-number">{{ formatNumber(99) }}%</div>
            <div class="stat-label">用户满意度</div>
          </div>
        </div>
      </div>
    </section> -->

    <!-- 产品展示 -->
    <section class="partners" id="partners">
      <div class="container">
        <div class="section-header">
          <h3>产品展示</h3>
          <p>展示产品预览及部分功能介绍</p>
        </div>
        <div class="partners-grid">
          <div class="partner-item" v-for="(logo, index) in partnerLogos" :key="index">
          <img 
            :src="logo.path" 
            :alt="logo.alt" 
            class="partner-logo cursor-pointer transition-transform hover:scale-105" 
            loading="lazy" 
            style="max-width: 100%; height: auto; display: block;" 
            @click="openImagePreview(logo)"
          >
        </div>
        </div>
      </div>
    </section>

    <!-- 号召性用语 -->
    <section class="cta animate-on-scroll" id="cta">
      <div class="container">
        <div class="cta-content">
          <h3>准备好提升您想法构思的实现了吗？</h3>
          <p>加入我们，体验 它 带来的全新生产方式。</p>
          <button class="btn btn-primary btn-lg">立即试用体验模型（非正式版）</button>
        </div>
      </div>
    </section>

    <!-- 页脚 -->
    <footer class="footer">
      <div class="container">
        <div class="footer-content">
          <div class="footer-logo">
            <h2>想法构思在这里</h2>
            <p>重新定义您想法构思的实现</p>
          </div>
          <div class="footer-links">
            <div class="footer-column">
              <h4>产品</h4>
              <ul>
                <li><a href="#" @click.prevent="scrollToSection('features')">功能介绍</a></li>
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
                <li><a href="#" @click.prevent="scrollToSection('about')">关于我们</a></li>
                <li><a href="#">联系我们</a></li>
                <li><a href="#">加入我们</a></li>
                <li><a href="#">隐私政策</a></li>
              </ul>
            </div>
          </div>
        </div>
        <div class="footer-bottom">
          <p>&copy; 2025 易雅熙世. 保留所有权利。</p>
          <div class="social-links">
          <a href="#" class="social-link"><el-icon><Star /></el-icon></a>
          <a href="#" class="social-link"><el-icon><Menu /></el-icon></a>
          <a href="#" class="social-link"><el-icon><Check /></el-icon></a>
        </div>
        </div>
      </div>
    </footer>
  </div>

  <!-- 图片预览模态框 - 添加无障碍支持 -->
  <div v-if="showImagePreview" class="image-preview-modal" @click="closeMediaPreview" role="dialog" aria-modal="true" aria-label="图片预览">
    <div class="image-preview-content" @click.stop>
      <button class="close-button" @click="closeMediaPreview">&times;</button>
      <img :src="selectedImage" :alt="selectedImageAlt" class="preview-image">
      <div class="preview-caption">{{ selectedImageAlt }}</div>
    </div>
  </div>
  
  <!-- 视频预览模态框 - 添加无障碍支持 -->
  <div v-if="showVideoPreview" class="video-preview-modal" @click="closeMediaPreview" role="dialog" aria-modal="true" aria-label="视频预览">
    <div class="video-preview-content" @click.stop>
      <button class="close-button" @click="closeMediaPreview" aria-label="关闭">&times;</button>
      <video 
        :src="selectedVideo" 
        :alt="selectedVideoAlt" 
        class="preview-video"
        controls
        autoplay
      ></video>
      <div class="preview-caption">{{ selectedVideoAlt }}</div>
    </div>
  </div>
</template>

<script>
import { ref, onMounted, onUnmounted, computed, onErrorCaptured } from 'vue';
import { useRoute, useRouter } from 'vue-router';
// 导入Element Plus图标 - 使用绝对存在的最基础图标确保兼容性
import { Check, Warning, Star, Help, Menu } from '@element-plus/icons-vue';

export default {
  name: 'HomeView',
  emits: ['load', 'errorCapture'],
  setup(props, { emit }) {
    // 首先导入路由
    const route = useRoute();
    const router = useRouter();
    
    // 定义状态变量 - 添加默认值和类型提示
    const showMobileMenu = ref(false);
    const scrolled = ref(false);
    const showImagePreview = ref(false);
    const showVideoPreview = ref(false);
    const selectedImage = ref('');
    const selectedImageAlt = ref('');
    const selectedVideo = ref('');
    const selectedVideoAlt = ref('');
    const carouselContainer = ref(null);
    
    // 轮播相关状态
    const carouselItems = ref([
      { path: '/main.png', alt: '主产品展示', type: 'image', caption: '强大的综合辅助工具' },
      { path: '/page.png', alt: '页面编辑工具预览', type: 'image', caption: '灵活的页面设计功能' },
      { path: '/form.png', alt: '表单创建工具展示', type: 'image', caption: '智能表单生成系统' },
      { path: '/card.png', alt: '卡片设计工具预览', type: 'image', caption: '精美的卡片设计模板' },
      { path: '/易见未来.mp4', alt: '功能展示视频', type: 'video', caption: '观看功能演示视频', poster: '/main.png' }
    ]);
    const currentSlide = ref(0);
    const isPlaying = ref(true);
    const videoPlaying = ref(false);
    let carouselInterval = null;
    
    // 错误捕获处理 - 改进：允许错误继续向上传播以便全局处理
    onErrorCaptured((err) => {
      emit('errorCapture', err);
      return true; // 返回true允许错误继续传播
    })
    
    // 产品展示图片数据 - 包含public目录下的所有图片资源
    const partnerLogos = ref([
      { path: '/main.png', alt: '主产品展示' },
      { path: '/page.png', alt: '页面编辑工具预览' },
      { path: '/form.png', alt: '表单创建工具展示' },
      { path: '/card.png', alt: '卡片设计工具预览' },
      { path: '/style.png', alt: '样式定制工具展示' }
    ]);

    // 打开媒体预览 - 支持图片和视频
    const openMediaPreview = (media) => {
      if (media.type === 'image') {
        selectedImage.value = media.path;
        selectedImageAlt.value = media.alt || '产品图片';
        showImagePreview.value = true;
      } else if (media.type === 'video') {
        selectedVideo.value = media.path;
        selectedVideoAlt.value = media.alt || '产品视频';
        showVideoPreview.value = true;
      }
      
      document.documentElement.classList.add('modal-open'); // 使用类来控制body滚动
      
      // 设置焦点到模态框，增强无障碍性
      setTimeout(() => {
        const modal = document.querySelector('.image-preview-modal, .video-preview-modal');
        if (modal) {
          modal.setAttribute('tabindex', '-1');
          modal.focus();
        }
      }, 100);
    };
    
    // 打开大图预览 - 保持向后兼容
    const openImagePreview = (image) => {
      openMediaPreview({
        path: typeof image === 'object' ? image.path : image,
        alt: typeof image === 'object' ? (image.alt || '产品图片') : '产品图片',
        type: 'image'
      });
    };

    // 关闭媒体预览 - 改进：使用类而不是直接修改样式
    const closeMediaPreview = () => {
      showImagePreview.value = false;
      showVideoPreview.value = false;
      document.documentElement.classList.remove('modal-open'); // 移除控制滚动的类
    };
    
    // 关闭大图预览 - 保持向后兼容
    const closeImagePreview = () => {
      closeMediaPreview();
    };

    // 键盘事件处理 - 支持ESC键关闭模态框
    const handleKeydown = (event) => {
      if (event.key === 'Escape' && (showImagePreview.value || showVideoPreview.value)) {
        closeMediaPreview();
      }
    };

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

    // 防抖函数 - 优化滚动事件处理
    const debounce = (func, wait) => {
      let timeout;
      return function executedFunction(...args) {
        const later = () => {
          clearTimeout(timeout);
          func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
      };
    };

    // 滚动监听，用于导航栏样式变化
    const handleScroll = debounce(() => {
      scrolled.value = window.scrollY > 20;
    }, 50);

    // 使用Intersection Observer API优化动画触发
    let observer = null;
    const setupIntersectionObserver = () => {
      // 仅在浏览器支持IntersectionObserver时使用
      if ('IntersectionObserver' in window) {
        observer = new IntersectionObserver(
          (entries) => {
            entries.forEach((entry) => {
              if (entry.isIntersecting) {
                entry.target.classList.add('animated');
                observer.unobserve(entry.target); // 动画触发后停止观察
              }
            });
          },
          {
            rootMargin: '0px 0px -100px 0px', // 底部100px的边距
            threshold: 0.1 // 10%的元素可见时触发
          }
        );

        // 观察所有需要动画的元素
        document.querySelectorAll('.animate-on-scroll:not(.animated)').forEach((el) => {
          observer.observe(el);
        });
      } else {
        // 降级方案：使用传统的滚动检测
        const fallbackAnimate = () => {
          document.querySelectorAll('.animate-on-scroll:not(.animated)').forEach((el) => {
            const rect = el.getBoundingClientRect();
            if (rect.top < window.innerHeight - 100) {
              el.classList.add('animated');
            }
          });
        };
        window.addEventListener('scroll', debounce(fallbackAnimate, 100));
        fallbackAnimate(); // 初始检查
      }
    };

    // 平滑滚动到指定区域
    const scrollToSection = (id) => {
      const element = document.getElementById(id);
      if (element) {
        window.scrollTo({
          top: element.offsetTop - 80,
          behavior: 'smooth'
        });
      }
      closeMobileMenu();
    };
    
    // 轮播控制方法
    const nextSlide = () => {
      currentSlide.value = (currentSlide.value + 1) % carouselItems.value.length;
      resetVideo();
    };
    
    const prevSlide = () => {
      currentSlide.value = (currentSlide.value - 1 + carouselItems.value.length) % carouselItems.value.length;
      resetVideo();
    };
    
    const goToSlide = (index) => {
      currentSlide.value = index;
      resetVideo();
    };
    
    const togglePlay = () => {
      isPlaying.value = !isPlaying.value;
      if (isPlaying.value) {
        startCarousel();
      } else {
        stopCarousel();
      }
    };
    
    const resetVideo = () => {
      if (carouselItems.value[currentSlide.value].type === 'video' && videoPlaying.value) {
        videoPlaying.value = false;
      }
    };
    
    const startCarousel = () => {
      if (carouselInterval) {
        clearInterval(carouselInterval);
      }
      carouselInterval = setInterval(() => {
        nextSlide();
      }, 5000); // 每5秒自动切换
    };
    
    const stopCarousel = () => {
      if (carouselInterval) {
        clearInterval(carouselInterval);
        carouselInterval = null;
      }
    };

    // 路由活动状态检测函数 - 改为普通函数以避免computed相关问题
    const isActiveRoute = (path) => route.path === path;

    // 组件挂载时初始化
    onMounted(() => {
      emit('load'); // 触发load事件
      window.addEventListener('scroll', handleScroll);
      window.addEventListener('keydown', handleKeydown);
      
      // 初始时触发一次滚动检测
      handleScroll();
      
      // 添加初始动画类
      setTimeout(() => {
        document.querySelectorAll('.animate-on-load').forEach(el => {
          el.classList.add('animated');
        });
      }, 100);
      
      // 设置Intersection Observer用于滚动动画
      setupIntersectionObserver();
      
      // 启动轮播
      if (isPlaying.value) {
        startCarousel();
      }
    });

    // 组件卸载时清理资源
    onUnmounted(() => {
      window.removeEventListener('scroll', handleScroll);
      window.removeEventListener('keydown', handleKeydown);
      // 清理Intersection Observer
      if (observer && 'disconnect' in observer) {
        observer.disconnect();
        observer = null;
      }
      // 停止轮播
      stopCarousel();
    });

    // 重新构建返回对象，确保只包含已定义的变量和图标
    return {
      // 状态变量
      showMobileMenu,
      scrolled,
      partnerLogos,
      showImagePreview,
      showVideoPreview,
      selectedImage,
      selectedImageAlt,
      selectedVideo,
      selectedVideoAlt,
      carouselItems,
      currentSlide,
      isPlaying,
      videoPlaying,
      // 方法
      formatNumber,
      toggleMobileMenu,
      closeMobileMenu,
      scrollToSection,
      isActiveRoute,
      openImagePreview,
      closeImagePreview,
      openMediaPreview,
      closeMediaPreview,
      nextSlide,
      prevSlide,
      goToSlide,
      togglePlay,
      
      // Element Plus图标 - 只导出已导入的图标
      Check,
      Warning,
      Star,
      Help,
      Menu
    };
  }
};
</script>

<style scoped>
/* CSS 变量 - 便于全局样式统一管理 */
:root {
  --primary-color: #409EFF;
  --primary-color-hover: #66b1ff;
  --primary-color-light: #ecf5ff;
  --secondary-color: #303133;
  --text-primary: #303133;
  --text-regular: #606266;
  --transition-duration: 0.5s;
  --border-radius: 12px;
  --box-shadow-light: 0 4px 16px rgba(0, 0, 0, 0.08);
  --box-shadow-base: 0 8px 24px rgba(0, 0, 0, 0.12);
  --border-color-light: #ebeef5;
}

/* 动画基础样式 */
.animate-on-scroll,
.animate-on-load {
  opacity: 0;
  transform: translateY(30px);
  transition: all 0.8s ease-out;
}

.animate-on-scroll.animated,
.animate-on-load.animated {
  opacity: 1;
  transform: translateY(0);
}

/* 延迟动画 */
.animate-on-scroll:nth-child(2).animated {
  transition-delay: 0.2s;
}

.animate-on-scroll:nth-child(3).animated {
  transition-delay: 0.4s;
}

.animate-on-scroll:nth-child(4).animated {
  transition-delay: 0.6s;
}
/* 基础样式 */
.home-view {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  overflow-x: hidden;
}

/* 容器样式 */
.container {
  width: 100%;
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
  box-sizing: border-box;
}

/* 顶部导航 - 增强版 (已合并到下方完整定义) */

/* 模态框打开时的body滚动控制 */
.modal-open {
  overflow: hidden;
  touch-action: none;
}

/* 模态框样式 - 统一处理图片和视频预览 */
.image-preview-modal,
.video-preview-modal {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.9);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  animation: fadeIn 0.3s ease-out;
  overflow: hidden; /* 防止模态框内部出现滚动条 */
}

.image-preview-content,
.video-preview-content {
  position: relative;
  max-width: 95%;
  max-height: 95vh;
  animation: zoomIn 0.3s ease-out;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.preview-image,
.preview-video {
  max-width: 100%;
  max-height: 80vh;
  object-fit: contain;
  border-radius: 8px;
}

/* 确保视频不会超出视口宽度 */
.preview-video {
  width: 100%; /* 让视频占满可用宽度 */
  height: auto; /* 保持视频比例 */
  max-width: 100vw; /* 确保不超出视口宽度 */
}

.close-button {
  position: absolute;
  top: -40px;
  right: -20px;
  background-color: #fff;
  border: none;
  border-radius: 50%;
  width: 40px;
  height: 40px;
  font-size: 24px;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: background-color 0.3s ease;
}

.close-button:hover {
  background-color: #f0f0f0;
}

.preview-caption {
  color: white;
  text-align: center;
  margin-top: 15px;
  font-size: 16px;
}

.cursor-pointer {
  cursor: pointer;
}

@keyframes fadeIn {
  from { opacity: 0; }
  to { opacity: 1; }
}

@keyframes zoomIn {
  from { transform: scale(0.8); opacity: 0; }
  to { transform: scale(1); opacity: 1; }
}

/* 顶部导航 - 增强版 */
.header {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  background-color: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(10px);
  box-shadow: var(--box-shadow-light);
  z-index: 1000;
  transition: all var(--transition-duration) ease-in-out;
  height: 80px;
}

/* 轮播组件样式 - 调整图片和视频显示尺寸 */
.hero-carousel {
  width: 100%;
  max-width: 600px;
}

.carousel-container {
  position: relative;
  width: 100%;
  overflow: hidden;
  border-radius: var(--border-radius);
  box-shadow: 0 20px 40px rgba(0, 0, 0, 0.15);
}

/* 轮播标题样式优化 */
.carousel-caption {
  position: relative;
  width: 100%;
  padding: 16px 24px;
  background-color: rgba(255, 255, 255, 0.9);
  color: var(--text-primary);
  font-size: 16px;
  font-weight: 500;
  text-align: center;
  line-height: 1.4;
  box-sizing: border-box;
  border-bottom-left-radius: var(--border-radius);
  border-bottom-right-radius: var(--border-radius);
  box-shadow: 0 -5px 15px rgba(0, 0, 0, 0.05);
}

.carousel-slide {
  width: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: #f8f9fa;
  overflow: hidden;
}

/* 缩略图容器样式 */
.thumbnail-container {
  width: 100%;
  position: relative;
  overflow: hidden;
  border-top-left-radius: var(--border-radius);
  border-top-right-radius: var(--border-radius);
}

/* 统一的缩略图样式 */
.carousel-thumbnail {
  width: 100%;
  height: auto;
  max-height: 400px;
  object-fit: contain;
  transition: transform 0.3s ease;
  background-color: #f0f0f0;
}

.thumbnail-container:hover .carousel-thumbnail {
  transform: scale(1.02);
}

/* 视频缩略图包装器 */
.video-thumbnail-wrapper {
  position: relative;
  width: 100%;
}

/* 视频播放覆盖层 */
.video-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: background-color 0.3s ease;
}

.video-overlay:hover {
  background-color: rgba(0, 0, 0, 0.2);
}

/* 播放图标 */
.play-icon {
  color: white;
  font-size: 48px;
  text-shadow: 0 2px 8px rgba(0, 0, 0, 0.5);
  transition: transform 0.3s ease;
  cursor: pointer;
}

.play-icon:hover {
  transform: scale(1.1);
}

.header.scrolled {
  height: 70px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.header.scrolled .header-content {
  height: 70px;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 80px;
  transition: height var(--transition-duration) ease-in-out;
}

.logo {
  display: flex;
  align-items: center;
  transition: transform var(--transition-duration);
}

.logo:hover {
  transform: scale(1.05);
}

.logo-image {
  width: 40px;
  height: 40px;
  margin-right: 10px;
  object-fit: contain;
}

.logo-text {
  color: var(--primary-color);
  font-size: 24px;
  font-weight: 600;
  margin: 0;
  background: linear-gradient(135deg, var(--primary-color) 0%, var(--primary-color-hover) 100%);
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
}

.navigation {
  display: flex;
  align-items: center;
  gap: 8px;
}

.nav-link {
  margin: 0 16px;
  color: var(--text-regular);
  text-decoration: none;
  font-weight: 500;
  transition: all var(--transition-duration);
  position: relative;
  padding: 8px 0;
}

.nav-link::after {
  content: '';
  position: absolute;
  bottom: 0;
  left: 50%;
  width: 0;
  height: 2px;
  background-color: var(--primary-color);
  transition: all var(--transition-duration);
  transform: translateX(-50%);
}

.nav-link:hover,
.nav-link.active {
  color: var(--primary-color);
}

.nav-link:hover::after,
.nav-link.active::after {
  width: 100%;
}

/* 按钮样式优化 */
.btn {
  padding: 10px 24px;
  border-radius: 25px;
  font-weight: 500;
  transition: all var(--transition-duration);
  border: none;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  text-decoration: none;
  position: relative;
  overflow: hidden;
}

.btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: all 0.5s;
}

.btn:hover::before {
  left: 100%;
}

.btn-primary {
  background-color: var(--primary-color);
  color: white;
}

.btn-primary:hover {
  background-color: var(--primary-color-hover);
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(64, 158, 255, 0.4);
}

.btn-outline {
  background-color: transparent;
  color: var(--primary-color);
  border: 2px solid var(--primary-color);
}

.btn-outline:hover {
  background-color: var(--primary-color-light);
  transform: translateY(-2px);
}

.btn-lg {
  padding: 12px 32px;
  font-size: 18px;
}

.mobile-menu-btn {
  display: none;
  cursor: pointer;
  padding: 8px;
  border-radius: 50%;
  transition: all var(--transition-duration);
}

.mobile-menu-btn:hover {
  background-color: var(--primary-color-light);
}

.mobile-menu {
  position: fixed;
  top: 80px;
  left: 0;
  right: 0;
  background-color: white;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  z-index: 999;
  animation: slideDown 0.3s ease-out;
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
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
  transition: all var(--transition-duration);
}

.mobile-nav-link:hover {
  color: var(--primary-color);
  padding-left: 8px;
}

.mobile-nav-link:last-child {
  border-bottom: none;
}

/* 英雄区域 - 增强版 */
.hero {
  padding: 140px 0 100px;
  background: linear-gradient(135deg, var(--primary-color-light) 0%, #f0f9ff 100%);
  margin-top: 80px;
  position: relative;
  overflow: hidden;
}

.hero::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-image: 
    radial-gradient(circle at 10% 20%, rgba(64, 158, 255, 0.1) 0%, transparent 20%),
    radial-gradient(circle at 80% 70%, rgba(64, 158, 255, 0.1) 0%, transparent 30%);
  z-index: 0;
}

.hero-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  position: relative;
  z-index: 1;
  gap: 40px;
}

.hero-text {
  flex: 1;
  max-width: 550px;
}

.hero-text h2 {
  font-size: 48px;
  font-weight: 700;
  color: var(--text-primary);
  margin-bottom: 24px;
  line-height: 1.2;
  letter-spacing: -0.5px;
}

.hero-text p {
  font-size: 18px;
  color: var(--text-regular);
  margin-bottom: 36px;
  line-height: 1.6;
}

.hero-actions {
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
}

.hero-image {
  flex: 1;
  display: flex;
  justify-content: flex-end;
  perspective: 1000px;
}

.hero-img {
  max-width: 100%;
  height: auto;
  border-radius: var(--border-radius);
  box-shadow: 0 20px 40px rgba(0, 0, 0, 0.15);
  transform: rotateY(5deg) rotateZ(2deg);
  transition: all 0.5s ease;
}

.hero-img:hover {
  transform: rotateY(0) rotateZ(0) scale(1.02);
  box-shadow: 0 25px 50px rgba(0, 0, 0, 0.2);
}

/* 特性展示 - 增强版 */
.features {
  padding: 100px 0;
  background-color: white;
  position: relative;
}

.section-header {
  text-align: center;
  margin-bottom: 80px;
}

.section-header h3 {
  font-size: 36px;
  font-weight: 700;
  color: var(--text-primary);
  margin-bottom: 16px;
  position: relative;
  display: inline-block;
}

.section-header h3::after {
  content: '';
  position: absolute;
  bottom: -10px;
  left: 50%;
  transform: translateX(-50%);
  width: 80px;
  height: 4px;
  background: linear-gradient(90deg, var(--primary-color) 0%, var(--primary-color-hover) 100%);
  border-radius: 2px;
}

.section-header p {
  font-size: 18px;
  color: var(--text-secondary);
  max-width: 700px;
  margin: 0 auto;
  line-height: 1.6;
}

.features-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 32px;
}

.feature-card {
  background-color: white;
  padding: 40px 32px;
  border-radius: var(--border-radius);
  box-shadow: var(--box-shadow-light);
  transition: all var(--transition-duration);
  text-align: center;
  position: relative;
  overflow: hidden;
  border: 1px solid transparent;
}

.feature-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: linear-gradient(90deg, var(--primary-color) 0%, var(--primary-color-hover) 100%);
  transform: scaleX(0);
  transition: transform var(--transition-duration);
}

.feature-card:hover {
  transform: translateY(-10px);
  box-shadow: var(--box-shadow-base);
  border-color: var(--primary-color-light);
}

.feature-card:hover::before {
  transform: scaleX(1);
}

.feature-icon {
  width: 80px;
  height: 80px;
  background-color: var(--primary-color-light);
  color: var(--primary-color);
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 20px;
  margin: 0 auto 32px;
  font-size: 32px;
  transition: all var(--transition-duration);
}

.feature-card:hover .feature-icon {
  background-color: var(--primary-color);
  color: white;
  transform: scale(1.1);
}

.feature-card h4 {
  font-size: 22px;
  font-weight: 600;
  color: var(--text-primary);
  margin-bottom: 16px;
  transition: color var(--transition-duration);
}

.feature-card:hover h4 {
  color: var(--primary-color);
}

.feature-card p {
  font-size: 16px;
  color: var(--text-regular);
  line-height: 1.6;
  margin: 0;
}

/* 数据统计 */
.stats {
  background-color: var(--primary-color-light);
  padding: 80px 0;
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
  font-size: 42px;
  font-weight: 700;
  color: var(--primary-color);
  margin-bottom: 12px;
  line-height: 1;
}

.stat-label {
  font-size: 16px;
  color: var(--text-regular);
  font-weight: 500;
}

/* 合作伙伴 - 增强版 */
.partners {
  padding: 100px 0;
  background-color: #fafafa;
}

.partners-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 32px;
  align-items: center;
  justify-items: center;
}

.partner-item {
  transition: transform var(--transition-duration);
  width: 100%;
  max-width: 180px;
}

.partner-item:hover {
  transform: scale(1.05);
}

.partner-logo {
  max-width: 100%;
  height: auto;
  opacity: 0.7;
  transition: opacity var(--transition-duration);
  filter: grayscale(30%);
}

.partner-item:hover .partner-logo {
  opacity: 1;
  filter: grayscale(0%);
}

/* 号召性用语 - 增强版 */
.cta {
  background: linear-gradient(135deg, var(--primary-color) 0%, var(--primary-color-hover) 100%);
  padding: 100px 0;
  color: white;
  position: relative;
  overflow: hidden;
}

.cta::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-image: 
    radial-gradient(circle at 10% 20%, rgba(255, 255, 255, 0.1) 0%, transparent 30%),
    radial-gradient(circle at 80% 70%, rgba(255, 255, 255, 0.1) 0%, transparent 30%);
}

.cta-content {
  text-align: center;
  max-width: 900px;
  margin: 0 auto;
  position: relative;
  z-index: 1;
}

.cta-content h3 {
  font-size: 40px;
  font-weight: 700;
  margin-bottom: 24px;
  line-height: 1.2;
}

.cta-content p {
  font-size: 20px;
  margin-bottom: 40px;
  opacity: 0.9;
  line-height: 1.6;
}

.cta-content .btn-primary {
  background-color: white;
  color: var(--primary-color);
  font-size: 20px;
  padding: 14px 40px;
  border-radius: 50px;
  transition: all var(--transition-duration);
}

.cta-content .btn-primary:hover {
  background-color: var(--primary-color-light);
  transform: translateY(-3px);
  box-shadow: 0 10px 25px rgba(255, 255, 255, 0.3);
}

/* 页脚 - 增强版 */
.footer {
  background: linear-gradient(135deg, #2c3e50 0%, #1a2530 100%);
  color: white;
  padding: 100px 0 40px;
  position: relative;
}

.footer::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 4px;
  background: linear-gradient(90deg, var(--primary-color) 0%, var(--primary-color-hover) 100%);
}

.footer-content {
  display: flex;
  justify-content: space-between;
  margin-bottom: 60px;
  flex-wrap: wrap;
  gap: 40px;
}

.footer-logo {
  flex: 0 0 250px;
}

.footer-logo h2 {
  font-size: 32px;
  font-weight: 700;
  margin-bottom: 16px;
  background: linear-gradient(135deg, white 0%, #e0e0e0 100%);
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
}

.footer-logo p {
  color: rgba(255, 255, 255, 0.7);
  font-size: 16px;
  line-height: 1.6;
}

.footer-links {
  display: flex;
  gap: 80px;
  flex-wrap: wrap;
}

.footer-column h4 {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 24px;
  position: relative;
  display: inline-block;
}

.footer-column h4::after {
  content: '';
  position: absolute;
  bottom: -8px;
  left: 0;
  width: 40px;
  height: 2px;
  background-color: var(--primary-color);
  border-radius: 1px;
}

.footer-column ul {
  list-style: none;
  padding: 0;
}

.footer-column li {
  margin-bottom: 16px;
}

.footer-column a {
  color: rgba(255, 255, 255, 0.7);
  text-decoration: none;
  transition: all var(--transition-duration);
  display: inline-block;
  position: relative;
  padding-left: 0;
}

.footer-column a:hover {
  color: white;
  transform: translateX(5px);
}

.footer-column a::before {
  content: '→';
  position: absolute;
  left: -20px;
  opacity: 0;
  transition: all var(--transition-duration);
  color: var(--primary-color);
}

.footer-column a:hover::before {
  left: -15px;
  opacity: 1;
}

.footer-bottom {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 40px;
  border-top: 1px solid rgba(255, 255, 255, 0.1);
  flex-wrap: wrap;
  gap: 20px;
}

.footer-bottom p {
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  margin: 0;
}

.social-links {
  display: flex;
  gap: 24px;
}

.social-link {
  color: rgba(255, 255, 255, 0.7);
  text-decoration: none;
  transition: all var(--transition-duration);
  font-size: 20px;
}

.social-link:hover {
  color: white;
  transform: translateY(-3px) scale(1.1);
}

/* 动画效果类 */
.animate-on-load {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.8s cubic-bezier(0.22, 1, 0.36, 1);
}

.animate-on-load.animated {
  opacity: 1;
  transform: translateY(0);
}

/* 延迟动画效果 */
.animate-on-load:nth-child(2) {
  transition-delay: 0.2s;
}

.animate-on-load:nth-child(3) {
  transition-delay: 0.4s;
}

.animate-on-load:nth-child(4) {
  transition-delay: 0.6s;
}

/* 滚动触发动画 */
@media (prefers-reduced-motion: no-preference) {
  .animate-on-scroll {
    opacity: 0;
    transform: translateY(30px);
    transition: all 0.8s cubic-bezier(0.22, 1, 0.36, 1);
  }
  
  .animate-on-scroll.animated {
    opacity: 1;
    transform: translateY(0);
  }
}

/* 响应式设计增强版 */
@media (max-width: 1200px) {
  .features-grid {
    grid-template-columns: repeat(2, 1fr);
  }
  
  .stats-grid {
    grid-template-columns: repeat(2, 1fr);
    gap: 40px;
  }
  
  .partners-grid {
    grid-template-columns: repeat(3, 1fr);
  }
  
  .hero-text h2 {
    font-size: 42px;
  }
  
  .section-header h3 {
    font-size: 32px;
  }
  
  /* 轮播组件响应式 - 中等桌面屏幕 */
.hero-carousel {
  max-width: 500px;
}

.carousel-thumbnail {
  max-height: 350px;
}
}

@media (max-width: 992px) {
  .hero-content {
    flex-direction: column;
    text-align: center;
  }
  
  .hero-image {
    justify-content: center;
    margin-top: 40px;
  }
  
  .partners-grid {
    grid-template-columns: repeat(2, 1fr);
  }
  
  .footer-links {
    gap: 40px;
  }
  
  /* 轮播组件响应式 - 平板屏幕 */
.hero-carousel {
  max-width: 450px;
}

.carousel-thumbnail {
  max-height: 300px;
}
}

@media (max-width: 768px) {
  .navigation {
    display: none;
  }
  
  .mobile-menu-btn {
    display: block;
  }
  
  .features-grid {
    grid-template-columns: 1fr;
    gap: 24px;
  }
  
  .partners-grid {
    grid-template-columns: 1fr;
    gap: 24px;
  }
  
  .footer-content {
    flex-direction: column;
    gap: 40px;
  }
  
  .footer-links {
    margin-top: 0;
    flex-direction: column;
    gap: 40px;
  }
  
  .footer-bottom {
    flex-direction: column;
    text-align: center;
    gap: 20px;
  }
  
  .hero-text h2 {
    font-size: 36px;
  }
  
  .section-header h3 {
    font-size: 28px;
  }
  
  .cta-content h3 {
    font-size: 32px;
  }
  
  /* 轮播组件响应式 - 小平板/大手机 */
.hero-carousel {
  max-width: 100%;
}

.carousel-thumbnail {
  max-height: 250px;
}
}

@media (max-width: 480px) {
  .header-content {
    padding: 0 16px;
  }
  
  .logo-image {
    width: 32px;
    height: 32px;
  }
  
  .logo-text {
    font-size: 20px;
  }
  
  .hero {
    padding: 120px 0 80px;
  }
  
  .hero-text h2 {
    font-size: 32px;
  }
  
  .hero-text p {
    font-size: 16px;
  }
  
  .hero-actions {
    flex-direction: column;
    gap: 12px;
  }
  
  .section-header h3 {
    font-size: 24px;
  }
  
  .cta-content h3 {
    font-size: 28px;
  }
  
  .cta-content p {
    font-size: 16px;
  }
  
  .footer {
    padding: 80px 0 40px;
  }
  
  .footer-logo h2 {
    font-size: 24px;
  }
  
  /* 轮播组件响应式 - 手机屏幕 */
.carousel-thumbnail {
  max-height: 200px;
}
}

/* 高分辨率屏幕优化 */
@media (-webkit-device-pixel-ratio: 2), (resolution: 192dpi) {
  .btn {
    border: 0.5px solid rgba(0, 0, 0, 0.1);
  }
}

/* 打印样式 */
@media print {
  .header,
  .mobile-menu-btn,
  .mobile-menu,
  .cta {
    display: none;
  }
  
  .hero {
    margin-top: 0;
    padding: 40px 0;
  }
  
  .home-view {
    font-size: 12pt;
  }
}
</style>