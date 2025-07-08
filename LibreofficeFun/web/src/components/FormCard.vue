<template>
  <el-card class="form-card" shadow="hover">
    <el-form :model="form" label-width="90px">
      <el-form-item label="Title" class="form-item-flex">
        <el-input v-model="form.title" placeholder="Please enter the main title" :disabled="!editable" class="form-input" />
        <el-checkbox v-model="form.showTitle" :disabled="!editable" class="form-checkbox">Show</el-checkbox>
        <el-select v-model="form.titleFontSize" :disabled="!editable" class="form-fontsize" placeholder="Font Size">
          <el-option v-for="size in fontSizes" :key="size" :label="size + 'px'" :value="size" />
        </el-select>
        <el-color-picker v-model="form.titleColor" :disabled="!editable" class="form-color" />
      </el-form-item>
      <el-form-item label="Value" class="form-item-flex">
        <el-input v-model="form.value" placeholder="Please enter the value" :disabled="!editable" class="form-input" />
        <el-checkbox v-model="form.showValue" :disabled="!editable" class="form-checkbox">Show</el-checkbox>
        <el-select v-model="form.valueFontSize" :disabled="!editable" class="form-fontsize" placeholder="Font Size">
          <el-option v-for="size in fontSizes" :key="size" :label="size + 'px'" :value="size" />
        </el-select>
        <el-color-picker v-model="form.valueColor" :disabled="!editable" class="form-color" />
      </el-form-item>
      <el-form-item label="Remark" class="form-item-flex">
        <el-input v-model="form.remark" type="textarea" placeholder="Please enter remarks" :disabled="!editable" class="form-input" />
        <el-checkbox v-model="form.showRemark" :disabled="!editable" class="form-checkbox">Show</el-checkbox>
        <el-select v-model="form.remarkFontSize" :disabled="!editable" class="form-fontsize" placeholder="Font Size">
          <el-option v-for="size in fontSizes" :key="size" :label="size + 'px'" :value="size" />
        </el-select>
        <el-color-picker v-model="form.remarkColor" :disabled="!editable" class="form-color" />
      </el-form-item>
      <el-form-item label="Media" class="form-item-flex">
        <media-uploader v-model="form.media" :disabled="!editable" class="form-input" />
        <el-checkbox v-model="form.showMedia" :disabled="!editable" class="form-checkbox">Show</el-checkbox>
      </el-form-item>
      <el-form-item>
        <el-button type="primary" @click="$emit('save', form)" :disabled="!editable">Save</el-button>
        <el-button @click="$emit('delete')" type="danger" :disabled="!editable">Delete</el-button>
      </el-form-item>
    </el-form>
  </el-card>
</template>

<script setup>
import { reactive, watch } from 'vue'
import MediaUploader from './MediaUploader.vue'
import { ElColorPicker, ElSelect, ElOption } from 'element-plus'
const fontSizes = [12, 14, 16, 18, 20, 24, 28, 32]
const props = defineProps({
  modelValue: Object,
  editable: Boolean
})
const emit = defineEmits(['update:modelValue', 'save', 'delete'])
const form = reactive({
  showTitle: true,
  showValue: true,
  showRemark: false,
  showMedia: false,
  titleFontSize: 16,
  valueFontSize: 16,
  remarkFontSize: 14,
  titleColor: '#333',
  valueColor: '#333',
  remarkColor: '#666',
  ...props.modelValue
})
watch(form, v => emit('update:modelValue', v), { deep: true })
</script>

<style scoped>
.form-card {
  margin: 8px;
  width: 600px;
  display: inline-block;
  vertical-align: top;
}
.form-item-flex {
  display: flex;
  align-items: center;
}
.form-input {
  flex: 1 1 auto;
  min-width: 0;
}
.form-checkbox {
  margin-left: 12px;
  min-width: 60px;
  text-align: right;
}
.form-fontsize {
  margin-left: 12px;
  width: 90px;
}
.form-color {
  margin-left: 8px;
}
</style>
