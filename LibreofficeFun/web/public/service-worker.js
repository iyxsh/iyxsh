// PWA Service Worker，支持缓存静态资源和API数据，后台同步

const CACHE_NAME = 'libreofficefun-cache-v1';
const API_CACHE_NAME = 'libreofficefun-api-cache-v1';
const STATIC_ASSETS = [
  '/',
  '/index.html',
  '/src/main.js',
  '/src/App.vue',
  '/src/style.css',
  '/logo.svg',
  '/favicon.ico',
  '/manifest.json'
];

// 修复语法错误和重复的install事件监听器
self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(async cache => {
      // 预缓存静态资源
      await cache.addAll(STATIC_ASSETS);
      return;
    })
  );
  self.skipWaiting();
});

self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(keys => {
      return Promise.all(
        keys.filter(key => key !== CACHE_NAME && key !== API_CACHE_NAME)
          .map(key => caches.delete(key))
      );
    })
  );
  self.clients.claim();
});

// 拦截fetch请求，静态资源优先缓存，API数据支持后台同步
// 为SSL环境优化fetch处理
self.addEventListener('fetch', event => {
  const url = new URL(event.request.url);
  
  // 确保只处理本域请求，避免跨域SSL问题
  if (url.origin !== self.origin) {
    return;
  }
  
  // 为开发环境添加宽松的安全策略
  const request = event.request;
  
  if (STATIC_ASSETS.includes(url.pathname)) {
    event.respondWith(
      caches.match(request).then(response => {
        return response || fetch(request).then(res => {
          return caches.open(CACHE_NAME).then(cache => {
            cache.put(request, res.clone());
            return res;
          });
        });
      })
    );
  } else if (url.pathname.startsWith('/api/')) {
    event.respondWith(
      fetch(request, {
        credentials: 'same-origin',
        redirect: 'follow'
      })
        .then(res => {
          // API数据缓存
          const clonedRes = res.clone();
          caches.open(API_CACHE_NAME).then(cache => {
            cache.put(request, clonedRes);
          });
          return res;
        })
        .catch(() => {
          // 离线时返回缓存API数据
          return caches.match(request);
        })
    );
  }
});

// 后台同步（示例：同步离线操作队列）
self.addEventListener('sync', event => {
  if (event.tag === 'sync-api-queue') {
    event.waitUntil(syncApiQueue());
  }
});

// IndexedDB 操作工具
function openDb() {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open('libreofficefun-sync-db', 1);
    request.onupgradeneeded = event => {
      const db = event.target.result;
      if (!db.objectStoreNames.contains('apiQueue')) {
        db.createObjectStore('apiQueue', { keyPath: 'id', autoIncrement: true });
      }
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
  });
}

async function getApiQueue() {
  const db = await openDb();
  return new Promise((resolve, reject) => {
    const tx = db.transaction('apiQueue', 'readonly');
    const store = tx.objectStore('apiQueue');
    const req = store.getAll();
    req.onsuccess = () => resolve(req.result);
    req.onerror = () => reject(req.error);
  });
}

async function clearApiQueue() {
  const db = await openDb();
  return new Promise((resolve, reject) => {
    const tx = db.transaction('apiQueue', 'readwrite');
    const store = tx.objectStore('apiQueue');
    const req = store.clear();
    req.onsuccess = () => resolve();
    req.onerror = () => reject(req.error);
  });
}

// 后台同步队列
async function syncApiQueue() {
  const queue = await getApiQueue();
  for (const item of queue) {
    try {
      await fetch(item.url, {
        method: item.method,
        headers: item.headers,
        body: item.body,
        credentials: 'same-origin',
        redirect: 'follow'
      });
    } catch (e) {
      // 后台同步失败，保留队列
      return;
    }
  }
  await clearApiQueue();
}

// PWA 安装提示事件（可用于前端监听）
self.addEventListener('beforeinstallprompt', event => {
  event.preventDefault();
  self.installPromptEvent = event;
});
