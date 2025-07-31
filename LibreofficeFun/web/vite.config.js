import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import path from 'path'

export default defineConfig({
  // 添加 define 配置
  define: {
    __VUE_OPTIONS_API__: true,
    __VUE_PROD_DEVTOOLS__: false,
    process: {
      env: {}
    }
  },
  plugins: [
    vue()
  ],
  // 明确指定构建选项
  build: {
    rollupOptions: {
      external: [],
      // 添加详细的输入配置
      input: {
        main: path.resolve(__dirname, './index.html')
      }
    }
  },
  // 配置别名
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
      '@components': path.resolve(__dirname, './src/components'),
      '@pages': path.resolve(__dirname, './src/pages'),
      '@utils': path.resolve(__dirname, './src/utils')
    },
    // 确保正确解析.vue文件
    extensions: ['.js', '.vue', '.json', '.css', '.scss','.ts']
  },
  // 配置服务器选项
  server: {
    open: true,
    port: 3000,
    strictPort: false,
    host: true,
    watch: {
      usePolling: true
    }
  }
})
