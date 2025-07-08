import { createRouter, createWebHistory } from 'vue-router'
import PageManager from '../pages/PageManager.vue'
const routes = [
  { path: '/', component: PageManager }
]
const router = createRouter({
  history: createWebHistory(),
  routes
})
export default router
