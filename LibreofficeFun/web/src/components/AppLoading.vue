<template>
  <section class="app-loading" role="status" aria-live="polite">
    <div v-if="!hasError" class="loading-container">
      <div class="progress-container">
        <div class="progress-text">{{ t('appLoading.loading') }} {{ progress }}%</div>
        <div class="progress-bar">
          <div :style="{ width: progress + '%' }" class="progress"></div>
        </div>
      </div>

      <div v-if="progress >= 100" class="loading-failed">
        {{ t('appLoading.failed') }}
        <button @click="$emit('reload')">{{ t('appLoading.retry') }}</button>
      </div>
    </div>

    <div v-else class="error-container">
      <p>{{ errorMessage }}</p>
      <button @click="$emit('reload')">{{ t('appLoading.retry') }}</button>
    </div>
  </section>
</template>

<script setup>
import { t } from '../utils/i18n'

const props = defineProps({
  progress: {
    type: Number,
    default: 0
  },
  hasError: {
    type: Boolean,
    default: false
  },
  errorMessage: {
    type: String,
    default: t('appLoading.defaultErrorMessage')
  }
})

const emit = defineEmits(['reload'])
</script>

<style scoped>
.app-loading {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  width: 100%;
}

.loading-container {
  text-align: center;
  padding: 20px;
  font-size: 18px;
  color: #333;
  width: 100%;
  max-width: 600px;
}

.progress-container {
  margin: 20px 0;
  width: 100%;
}

.progress-text {
  margin-bottom: 10px;
  font-size: 16px;
  color: #606266;
}

.progress-bar {
  height: 8px;
  background-color: #ebeef5;
  border-radius: 4px;
  overflow: hidden;
}

.progress {
  height: 100%;
  background-color: #42b883;
  border-radius: 4px;
  transition: width 0.3s ease;
}

.loading-failed {
  margin-top: 20px;
  color: #f56c6c;
}

.error-container {
  text-align: center;
  padding: 20px;
  color: #e74c3c;
  max-width: 600px;
}

button {
  margin-top: 10px;
  padding: 8px 16px;
  background-color: #42b883;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;
}

button:hover {
  background-color: #369767;
}

.error-container button {
  background-color: #e74c3c;
}

.error-container button:hover {
  background-color: #c0392b;
}
</style>
