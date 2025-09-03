import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import path from 'path'

export default defineConfig(({ mode }) => {
  const isProduction = mode === 'production'
  
  return {
    plugins: [
      vue()
    ],
    build: {
      chunkSizeWarningLimit: 1000, // 调整chunk大小警告限制为1000kB
      minify: isProduction ? 'esbuild' : false,
      sourcemap: !isProduction, // 开发版本生成sourcemap便于调试
      outDir: isProduction ? 'dist' : 'dist-dev',
      assetsDir: 'assets',
      assetsInlineLimit: 4096, // 4kb以下的资源内联
      cssCodeSplit: true,
      rollupOptions: {
        output: {
          // 手动配置chunk分割
          manualChunks(id) {
            // 将大型依赖分离到单独的chunk中
            if (id.includes('node_modules')) {
              if (id.includes('element-plus')) {
                return 'element-plus'
              } else if (id.includes('axios')) {
                return 'axios'
              } else if (id.includes('lodash')) {
                return 'lodash'
              } else if (id.includes('@vueuse')) {
                return 'vueuse'
              } else if (id.includes('pinia')) {
                return 'pinia'
              } else if (id.includes('@vue')) {
                return 'vue'
              }
              // 其他第三方库统一放在vendors chunk中
              return 'vendors'
            }
          }
        }
      }
    },
    resolve: {
      alias: {
        '@': path.resolve(__dirname, './src')
      },
      extensions: ['.js', '.vue', '.json', '.css', '.scss', '.ts']
    },
    server: {
      port: 3000,
      host: true,
      headers: {
        'Strict-Transport-Security': 'max-age=63072000; includeSubDomains; preload',
        'X-Frame-Options': 'DENY',
        'X-Content-Type-Options': 'nosniff',
        'X-XSS-Protection': '1; mode=block',
        'Content-Security-Policy': "default-src 'self'; script-src 'self' 'unsafe-inline' 'unsafe-eval'; style-src 'self' 'unsafe-inline'; img-src 'self' data:;"
      }
    }
  };
});