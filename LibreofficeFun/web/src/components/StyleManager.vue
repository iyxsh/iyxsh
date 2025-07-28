<template>
  <div class="style-manager">
    <div class="style-manager-header">
      <h3>样式管理器</h3>
      <div class="header-actions">
        <el-button 
          size="small" 
          @click="copyStyle"
          title="复制当前样式"
        >
          <el-icon><CopyDocument /></el-icon>
        </el-button>
        <el-button 
          size="small" 
          @click="pasteStyle"
          title="粘贴样式"
        >
          <el-icon><DocumentCopy /></el-icon>
        </el-button>
      </div>
    </div>
    <div>
      <h5 style="color: red;">"操作说明：单双击"设置样式 "Ctrl + 单击"切换样式可见性</h5>
    </div>
    
    <div class="style-manager-content">
      <!-- 样式工具栏 -->
      <div class="style-toolbar">
        <div 
          v-for="category in allStylesTree" 
          :key="category.id"
          class="toolbar-category"
        >
          <div class="category-header">
            <h4>{{ category.label }}</h4>
          </div>
          <div class="toolbar-icons">
            <div
              v-for="item in category.children"
              :key="item.id"
              class="toolbar-item"
              :class="{ active: isVisibleStyle(item.key) }"
              @click.stop="handleToolbarItemClick($event, item)"
              @dblclick.stop="openStyleSettings(item)"
            >
              <div class="icon-wrapper">
                <el-icon v-if="getStyleIcon(item.key)">
                  <component :is="getStyleIcon(item.key)" />
                </el-icon>
                <div v-else class="default-icon">{{ item.label.charAt(0) }}</div>
              </div>
              <span class="icon-label">{{ item.label }}</span>
            </div>
          </div>
        </div>
      </div>
      
      <!-- 样式设置区域 -->
      <div class="settings-section advanced-section">
        <div class="settings-grid">
          <!-- 盒模型 -->
          <div 
            v-for="item in visibleBoxModelSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'number'">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                size="small"
                @change="updateStyle"
              />
              <span v-if="item.unit" class="unit">{{ item.unit }}</span>
            </template>
            
            <template v-else-if="item.type === 'input'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
          
          <!-- 字体族特殊处理 -->
          <div 
            v-for="item in visibleTextSettings" 
            :key="item.key" 
            class="setting-item"
            v-show="item.key === 'fontFamily'"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="handleFontFamilyChange"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
            
            <!-- 当选择自定义字体时显示输入框 -->
            <template v-if="localStyle.fontFamily === 'custom'">
              <el-input 
                v-model="customFontFamily" 
                size="small"
                placeholder="请输入字体族，如：Arial, sans-serif"
                @change="updateCustomFontFamily"
              />
              <div class="font-family-tip">
                <p>提示：可以输入多个字体名称，用逗号分隔，如：Arial, Helvetica, sans-serif</p>
                <p>系统会按顺序查找，使用第一个找到的字体</p>
              </div>
            </template>
          </div>
          
          <!-- 文本设置（除字体族外） -->
          <div 
            v-for="item in visibleTextSettings" 
            :key="item.key" 
            class="setting-item"
            v-show="item.key !== 'fontFamily'"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'number'">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                :step="item.step"
                size="small"
                controls-position="right"
                @change="updateStyle"
              />
              <span v-if="item.unit" class="unit">{{ item.unit }}</span>
            </template>
            
            <template v-else-if="item.type === 'color-picker'">
              <el-color-picker 
                v-model="localStyle[item.key]" 
                size="small" 
                :show-alpha="item.showAlpha"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
          
          <!-- 常用设置（除字体族外） -->
          <div 
            v-for="item in visibleBasicSettings" 
            :key="item.key" 
            class="setting-item"
            v-show="item.key !== 'fontFamily'"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'number'">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                :step="item.step"
                size="small"
                controls-position="right"
                @change="updateStyle"
              />
              <span v-if="item.unit" class="unit">{{ item.unit }}</span>
            </template>
            
            <template v-else-if="item.type === 'color-picker'">
              <el-color-picker 
                v-model="localStyle[item.key]" 
                size="small" 
                :show-alpha="item.showAlpha"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
            
            <template v-else-if="item.type === 'switch'">
              <el-switch
                v-model="localStyle[item.key]"
                :active-value="item.activeValue"
                :inactive-value="item.inactiveValue"
                @change="updateStyle"
              />
              <span v-if="item.switchLabel" class="switch-label">{{ item.switchLabel }}</span>
            </template>
          </div>
          
          <!-- 边框 -->
          <div 
            v-for="item in visibleBorderSettings" 
            :key="item.key" 
            class="setting-item"
            :class="item.type === 'color-picker' ? 'color-picker-item' : ''"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'number' && (!item.condition || localStyle[item.condition] !== 'none')">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                size="small"
                @change="updateStyle"
              />
              <span v-if="item.unit" class="unit">{{ item.unit }}</span>
            </template>
            
            <template v-else-if="item.type === 'color-picker' && (!item.condition || localStyle[item.condition] !== 'none')">
              <el-color-picker 
                v-model="localStyle[item.key]" 
                size="small" 
                :show-alpha="item.showAlpha"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
          
          <!-- 布局 -->
          <div 
            v-for="item in visibleLayoutSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'number' && (!item.condition || localStyle[item.condition] !== 'static')">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                size="small"
                controls-position="right"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'input' && (!item.condition || localStyle[item.condition] !== 'static')">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
            
            <!-- Flexbox相关属性 -->
            <template v-else-if="item.type === 'flex-select' && localStyle.display === 'flex'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
            
            <!-- Grid相关属性 -->
            <template v-else-if="item.type === 'grid-input' && localStyle.display === 'grid'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                :placeholder="item.placeholder"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'grid-number' && localStyle.display === 'grid'">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                size="small"
                @change="updateStyle"
              />
              <span v-if="item.unit" class="unit">{{ item.unit }}</span>
            </template>
          </div>
          
          <!-- 背景 -->
          <div 
            v-for="item in visibleBackgroundSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'input'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                :placeholder="item.placeholder"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
          
          <!-- 文本 -->
          <div 
            v-for="item in visibleTextSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'input'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                :placeholder="item.placeholder"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'number'">
              <el-input-number 
                v-model="localStyle[item.key]" 
                :min="item.min" 
                :max="item.max" 
                :step="item.step"
                size="small"
                :controls-position="item.controlsPosition"
                @change="updateStyle"
              />
              <span v-if="item.unit" class="unit">{{ item.unit }}</span>
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
            
            <template v-else-if="item.type === 'switch'">
              <el-switch
                v-model="localStyle[item.key]"
                :active-value="item.activeValue"
                :inactive-value="item.inactiveValue"
                @change="updateStyle"
              />
              <span v-if="item.switchLabel" class="switch-label">{{ item.switchLabel }}</span>
            </template>
          </div>
          
          <!-- 列表 -->
          <div 
            v-for="item in visibleListSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'input'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                :placeholder="item.placeholder"
                @change="updateStyle"
              />
            </template>
            
            <template v-else-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
          
          <!-- 交互 -->
          <div 
            v-for="item in visibleInteractionSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
          
          <!-- 效果 -->
          <div 
            v-for="item in visibleEffectSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'input'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                :placeholder="item.placeholder"
                @change="updateStyle"
              />
            </template>
          </div>
          
          <!-- 动画 -->
          <div 
            v-for="item in visibleAnimationSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'input'">
              <el-input 
                v-model="localStyle[item.key]" 
                size="small"
                :placeholder="item.placeholder"
                @change="updateStyle"
              />
            </template>
          </div>
          
          <!-- 方向 -->
          <div 
            v-for="item in visibleDirectionSettings" 
            :key="item.key" 
            class="setting-item"
          >
            <label>{{ item.label }}</label>
            <template v-if="item.type === 'select'">
              <el-select 
                v-model="localStyle[item.key]" 
                size="small"
                @change="updateStyle"
              >
                <el-option 
                  v-for="option in item.options" 
                  :key="option.value" 
                  :label="option.label" 
                  :value="option.value" 
                />
              </el-select>
            </template>
          </div>
        </div>
      </div>
    </div>
    
    <div class="style-manager-footer">
      <el-button @click="resetStyle" size="small">重置</el-button>
      <el-button type="primary" @click="applyStyle" size="small">应用</el-button>
    </div>
    
    <!-- 样式设置对话框 -->
    <el-dialog
      v-model="styleSettingsDialogVisible"
      :title="currentSettingItem?.label"
      width="400px"
    >
      <div v-if="currentSettingItem" class="setting-dialog-content">
        <template v-if="currentSettingItem.type === 'number'">
          <el-input-number 
            v-model="localStyle[currentSettingItem.key]" 
            :min="currentSettingItem.min" 
            :max="currentSettingItem.max" 
            :step="currentSettingItem.step"
            size="small"
            controls-position="right"
            @change="updateStyle"
          />
          <span v-if="currentSettingItem.unit" class="unit">{{ currentSettingItem.unit }}</span>
        </template>
        
        <template v-else-if="currentSettingItem.type === 'slider'">
          <el-slider 
            v-model="localStyle[currentSettingItem.key]" 
            :min="currentSettingItem.min" 
            :max="currentSettingItem.max" 
            :step="currentSettingItem.step"
            size="small"
            :show-input="currentSettingItem.showInput"
            @change="updateStyle"
          />
          <span v-if="currentSettingItem.showValue" class="value-display">{{ localStyle[currentSettingItem.key] }}{{ currentSettingItem.unit || '' }}</span>
        </template>
        
        <template v-else-if="currentSettingItem.type === 'color-picker'">
          <el-color-picker 
            v-model="localStyle[currentSettingItem.key]" 
            size="small" 
            :show-alpha="currentSettingItem.showAlpha"
            @change="updateStyle"
          />
        </template>
        
        <template v-else-if="currentSettingItem.type === 'select'">
          <el-select 
            v-model="localStyle[currentSettingItem.key]" 
            size="small"
            @change="updateStyle"
          >
            <el-option 
              v-for="option in currentSettingItem.options" 
              :key="option.value" 
              :label="option.label" 
              :value="option.value" 
            />
          </el-select>
        </template>
        
        <template v-else-if="currentSettingItem.type === 'button-group'">
          <div class="button-group">
            <el-button 
              v-for="btn in currentSettingItem.buttons" 
              :key="btn.value"
              size="small" 
              :type="localStyle[currentSettingItem.key] === btn.value ? 'primary' : 'default'"
              @click="localStyle[currentSettingItem.key] = btn.value; updateStyle()"
              :title="btn.title"
            >
              <el-icon v-if="btn.icon"><component :is="btn.icon" /></el-icon>
              <span v-else>{{ btn.label }}</span>
            </el-button>
          </div>
        </template>
        
        <template v-else-if="currentSettingItem.type === 'input'">
          <el-input 
            v-model="localStyle[currentSettingItem.key]" 
            size="small"
            @change="updateStyle"
          />
        </template>
        
        <template v-else-if="currentSettingItem.type === 'switch'">
          <el-switch
            v-model="localStyle[currentSettingItem.key]"
            :active-value="currentSettingItem.activeValue"
            :inactive-value="currentSettingItem.inactiveValue"
            @change="updateStyle"
          />
          <span v-if="currentSettingItem.switchLabel" class="switch-label">{{ currentSettingItem.switchLabel }}</span>
        </template>
      </div>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="styleSettingsDialogVisible = false" size="small">关闭</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted, computed } from 'vue'
import { 
  CopyDocument, 
  DocumentCopy, 
  ArrowLeft, 
  ArrowRight, 
  Close, 
  ScaleToOriginal,
  Brush,
  Picture,
  Box,
  Crop,
  Position,
  Link,
  Monitor,
  MagicStick
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

// 定义props
const props = defineProps({
  modelValue: {
    type: Object,
    default: () => ({})
  },
  predefineColors: {
    type: Array,
    default: () => [
      '#ffffff', '#000000', '#ff0000', '#00ff00', '#0000ff',
      '#ffff00', '#00ffff', '#ff00ff', '#f5f5f5', '#e0e0e0'
    ]
  }
})

// 定义emits
const emit = defineEmits(['update:modelValue', 'change', 'apply'])

// 数据
const localStyle = reactive({}) // 全部默认值
const localTextStyle = reactive({}) // 文本相关默认值
const localContainerStyle = reactive({}) // 容器相关默认值
const visibleStyles = ref(new Set()) // 存储可见样式属性的集合
const customFontFamily = ref('') // 存储自定义字体族

// 对话框相关
const styleSettingsDialogVisible = ref(false)
const currentSettingItem = ref(null)

// 所有样式定义
const allStylesDefinition = computed(() => {
  return {
    // 常用设置
    basic: [
      { key: 'fontSize', label: '字体大小', type: 'number', min: 1, max: 100, unit: 'px' },
      { key: 'color', label: '字体颜色', type: 'color-picker', showAlpha: true },
      { key: 'fontWeight', label: '字体粗细', type: 'select', options: [
        { label: '正常', value: 'normal' },
        { label: '粗体', value: 'bold' },
        { label: '100', value: '100' },
        { label: '200', value: '200' },
        { label: '300', value: '300' },
        { label: '400', value: '400' },
        { label: '500', value: '500' },
        { label: '600', value: '600' },
        { label: '700', value: '700' },
        { label: '800', value: '800' },
        { label: '900', value: '900' }
      ]},
      { key: 'fontStyle', label: '字体样式', type: 'select', options: [
        { label: '正常', value: 'normal' },
        { label: '斜体', value: 'italic' },
        { label: '倾斜', value: 'oblique' }
      ]},
      { key: 'textAlign', label: '文本对齐', type: 'button-group', buttons: [
        { value: 'left', icon: ArrowLeft, title: '左对齐' },
        { value: 'center', icon: Close, title: '居中' },
        { value: 'right', icon: ArrowRight, title: '右对齐' },
        { value: 'justify', icon: ScaleToOriginal, title: '两端对齐' }
      ]},
      { key: 'lineHeight', label: '行高', type: 'slider', min: 0, max: 5, step: 0.1, showInput: true },
      { key: 'backgroundColor', label: '背景颜色', type: 'color-picker', showAlpha: true },
      { key: 'opacity', label: '透明度', type: 'slider', min: 0, max: 1, step: 0.01, showInput: true },
      { key: 'textRotation', label: '文字旋转', type: 'slider', min: 0, max: 360, showValue: true, unit: '°' },
      { key: 'scale', label: '缩放比例', type: 'slider', min: 0.1, max: 5, step: 0.1, showValue: true }
    ],
    
    // 盒模型设置
    boxModel: [
      { key: 'padding', label: '内边距', type: 'number', min: 0, max: 100, unit: 'px' },
      { key: 'margin', label: '外边距', type: 'number', min: 0, max: 100, unit: 'px' },
      { key: 'width', label: '宽度', type: 'input' },
      { key: 'height', label: '高度', type: 'input' },
      { key: 'maxWidth', label: '最大宽度', type: 'input' },
      { key: 'maxHeight', label: '最大高度', type: 'input' },
      { key: 'minWidth', label: '最小宽度', type: 'input' },
      { key: 'minHeight', label: '最小高度', type: 'input' },
      { key: 'boxSizing', label: '盒模型', type: 'select', options: [
        { label: '默认', value: '' },
        { label: '内容盒', value: 'content-box' },
        { label: '边框盒', value: 'border-box' }
      ]}
    ],
    
    // 边框设置
    border: [
      { key: 'borderStyle', label: '边框', type: 'select', options: [
        { label: '无', value: 'none' },
        { label: '实线', value: 'solid' },
        { label: '虚线', value: 'dashed' },
        { label: '点线', value: 'dotted' },
        { label: '双线', value: 'double' },
        { label: '凹槽', value: 'groove' },
        { label: 'ridge', value: 'ridge' },
        { label: '插入', value: 'inset' },
        { label: '突出', value: 'outset' }
      ]},
      { key: 'borderWidth', label: '边框宽度', type: 'number', min: 0, max: 20, unit: 'px', condition: 'borderStyle' },
      { key: 'borderColor', label: '边框颜色', type: 'color-picker', showAlpha: true, condition: 'borderStyle' },
      { key: 'borderRadius', label: '圆角', type: 'number', min: 0, max: 50, unit: 'px' }
    ],
    
    // 布局设置
    layout: [
      { key: 'display', label: '显示类型', type: 'select', options: [
        { label: '默认', value: '' },
        { label: '块级', value: 'block' },
        { label: '行内', value: 'inline' },
        { label: '行内块', value: 'inline-block' },
        { label: '弹性', value: 'flex' },
        { label: '网格', value: 'grid' },
        { label: '表格', value: 'table' },
        { label: '列表项', value: 'list-item' },
        { label: '隐藏', value: 'none' }
      ]},
      { key: 'position', label: '定位', type: 'select', options: [
        { label: '静态', value: 'static' },
        { label: '相对', value: 'relative' },
        { label: '绝对', value: 'absolute' },
        { label: '固定', value: 'fixed' },
        { label: '粘性', value: 'sticky' }
      ]},
      { key: 'left', label: '左边距', type: 'input', condition: 'position' },
      { key: 'top', label: '上边距', type: 'input', condition: 'position' },
      { key: 'right', label: '右边距', type: 'input', condition: 'position' },
      { key: 'bottom', label: '下边距', type: 'input', condition: 'position' },
      { key: 'float', label: '浮动', type: 'select', options: [
        { label: '无', value: 'none' },
        { label: '左浮动', value: 'left' },
        { label: '右浮动', value: 'right' }
      ]},
      { key: 'clear', label: '清除浮动', type: 'select', options: [
        { label: '无', value: 'none' },
        { label: '左清除', value: 'left' },
        { label: '右清除', value: 'right' },
        { label: '两侧清除', value: 'both' }
      ]},
      { key: 'zIndex', label: 'z-index', type: 'number', min: -100, max: 100 },
      
      // Flexbox相关属性
      { key: 'flexDirection', label: '主轴方向', type: 'flex-select', options: [
        { label: '行', value: 'row' },
        { label: '行反转', value: 'row-reverse' },
        { label: '列', value: 'column' },
        { label: '列反转', value: 'column-reverse' }
      ]},
      { key: 'flexWrap', label: '换行', type: 'flex-select', options: [
        { label: '不换行', value: 'nowrap' },
        { label: '换行', value: 'wrap' },
        { label: '反转换行', value: 'wrap-reverse' }
      ]},
      { key: 'justifyContent', label: '主轴对齐', type: 'flex-select', options: [
        { label: '起始对齐', value: 'flex-start' },
        { label: '结束对齐', value: 'flex-end' },
        { label: '居中', value: 'center' },
        { label: '两端对齐', value: 'space-between' },
        { label: '均匀分布', value: 'space-around' },
        { label: '完全均匀分布', value: 'space-evenly' }
      ]},
      { key: 'alignItems', label: '交叉轴对齐', type: 'flex-select', options: [
        { label: '拉伸', value: 'stretch' },
        { label: '起始对齐', value: 'flex-start' },
        { label: '结束对齐', value: 'flex-end' },
        { label: '居中', value: 'center' },
        { label: '基线对齐', value: 'baseline' }
      ]},
      
      // Grid相关属性
      { key: 'gridTemplateColumns', label: '网格模板列', type: 'grid-input', placeholder: '例: 1fr 1fr 1fr' },
      { key: 'gridTemplateRows', label: '网格模板行', type: 'grid-input', placeholder: '例: auto 1fr auto' },
      { key: 'gridColumnGap', label: '网格列间隙', type: 'grid-number', min: 0, max: 100, unit: 'px' },
      { key: 'gridRowGap', label: '网格行间隙', type: 'grid-number', min: 0, max: 100, unit: 'px' }
    ],
    
    // 背景设置
    background: [
      { key: 'backgroundImage', label: '背景图片', type: 'input', placeholder: '例: url(image.jpg)' },
      { key: 'backgroundRepeat', label: '背景重复', type: 'select', options: [
        { label: '重复', value: 'repeat' },
        { label: '不重复', value: 'no-repeat' },
        { label: 'X轴重复', value: 'repeat-x' },
        { label: 'Y轴重复', value: 'repeat-y' }
      ]},
      { key: 'backgroundSize', label: '背景大小', type: 'select', options: [
        { label: '自动', value: 'auto' },
        { label: '覆盖', value: 'cover' },
        { label: '包含', value: 'contain' }
      ]},
      { key: 'backgroundPosition', label: '背景位置', type: 'input', placeholder: '例: center center' },
      { key: 'backgroundAttachment', label: '背景附着', type: 'select', options: [
        { label: '滚动', value: 'scroll' },
        { label: '固定', value: 'fixed' },
        { label: '局部', value: 'local' }
      ]}
    ],
    
    // 文本设置
    text: [
      { key: 'fontFamily', label: '字体族', type: 'select', options: [
        { label: '默认', value: '' },
        { label: '宋体/SimSun', value: 'SimSun, STSong, serif' },
        { label: '黑体/SimHei', value: 'SimHei, STHeiti, sans-serif' },
        { label: '微软雅黑/Microsoft YaHei', value: 'Microsoft YaHei, STHeiti, sans-serif' },
        { label: '楷体/KaiTi', value: 'KaiTi, STKaiti, serif' },
        { label: '仿宋/FangSong', value: 'FangSong, STFangsong, serif' },
        { label: '华文细黑/STXihei', value: 'STXihei, SimHei, sans-serif' },
        { label: '华文宋体/STSong', value: 'STSong, SimSun, serif' },
        { label: '华文仿宋/STFangsong', value: 'STFangsong, FangSong, serif' },
        { label: '华文楷体/STKaiti', value: 'STKaiti, KaiTi, serif' },
        { label: '思源黑体/Source Han Sans', value: 'Source Han Sans SC, Source Han Sans, Noto Sans CJK SC, sans-serif' },
        { label: '思源宋体/Source Han Serif', value: 'Source Han Serif SC, Source Han Serif, Noto Serif CJK SC, serif' },
        { label: '苹方/PingFang SC', value: 'PingFang SC, Helvetica Neue, Helvetica, Arial, sans-serif' },
        { label: '苹方粗体/PingFang SC Bold', value: 'PingFang SC Bold, PingFang SC, Helvetica Neue, Helvetica, Arial, sans-serif' },
        { label: '兰亭黑/Lantinghei SC', value: 'Lantinghei SC, SimHei, sans-serif' },
        { label: '汉仪旗黑/HanYiQiHei', value: 'HanYiQiHei, SimHei, sans-serif' },
        { label: '汉仪楷体/HanYiKaiti', value: 'HanYiKaiti, KaiTi, serif' },
        { label: '方正兰亭黑/FZLanTingHei', value: 'FZLanTingHei, SimHei, sans-serif' },
        { label: '方正楷体/FZKaiTi', value: 'FZKaiTi, KaiTi, serif' },
        { label: '方正书宋/FZShuSong', value: 'FZShuSong, SimSun, serif' },
        { label: 'Arial', value: 'Arial, Helvetica, sans-serif' },
        { label: 'Arial Black', value: 'Arial Black, Arial Bold, Gadget, sans-serif' },
        { label: 'Helvetica', value: 'Helvetica, Arial, sans-serif' },
        { label: 'Times New Roman', value: '"Times New Roman", Times, serif' },
        { label: 'Times', value: 'Times, "Times New Roman", serif' },
        { label: 'Verdana', value: 'Verdana, Geneva, sans-serif' },
        { label: 'Tahoma', value: 'Tahoma, Geneva, sans-serif' },
        { label: 'Trebuchet MS', value: '"Trebuchet MS", Helvetica, sans-serif' },
        { label: 'Georgia', value: 'Georgia, serif' },
        { label: 'Courier New', value: '"Courier New", Courier, monospace' },
        { label: 'Brush Script MT', value: '"Brush Script MT", cursive' },
        { label: 'Impact', value: 'Impact, Charcoal, sans-serif' },
        { label: 'Comic Sans MS', value: '"Comic Sans MS", cursive, sans-serif' },
        { label: 'Open Sans', value: 'Open Sans, sans-serif' },
        { label: 'Roboto', value: 'Roboto, sans-serif' },
        { label: 'Lato', value: 'Lato, sans-serif' },
        { label: 'Montserrat', value: 'Montserrat, sans-serif' },
        { label: 'Oswald', value: 'Oswald, sans-serif' },
        { label: 'Raleway', value: 'Raleway, sans-serif' },
        { label: 'Ubuntu', value: 'Ubuntu, sans-serif' },
        { label: '自定义', value: 'custom' }
      ]},
      { key: 'fontVariant', label: '字体大小调整', type: 'switch', activeValue: 'small-caps', inactiveValue: 'normal', switchLabel: '小型大写字母' },
      { key: 'textDecoration', label: '文字装饰', type: 'select', options: [
        { label: '无', value: 'none' },
        { label: '下划线', value: 'underline' },
        { label: '上划线', value: 'overline' },
        { label: '删除线', value: 'line-through' },
        { label: '下划线+删除线', value: 'underline line-through' }
      ]},
      { key: 'textTransform', label: '文字转换', type: 'select', options: [
        { label: '无', value: 'none' },
        { label: '大写', value: 'uppercase' },
        { label: '小写', value: 'lowercase' },
        { label: '首字母大写', value: 'capitalize' }
      ]},
      { key: 'verticalAlign', label: '垂直对齐', type: 'select', options: [
        { label: '基线', value: 'baseline' },
        { label: '顶部', value: 'top' },
        { label: '中间', value: 'middle' },
        { label: '底部', value: 'bottom' },
        { label: '文本顶部', value: 'text-top' },
        { label: '文本底部', value: 'text-bottom' },
        { label: '上标', value: 'super' },
        { label: '下标', value: 'sub' }
      ]},
      { key: 'letterSpacing', label: '字母间距', type: 'number', min: -5, max: 10, step: 0.1, controlsPosition: 'right', unit: 'px' },
      { key: 'wordSpacing', label: '词间距', type: 'number', min: -5, max: 20, step: 0.1, controlsPosition: 'right', unit: 'px' },
      { key: 'textIndent', label: '文本缩进', type: 'number', min: 0, max: 100, unit: 'px' },
      { key: 'textShadow', label: '文本阴影', type: 'input', placeholder: '例: 1px 1px 2px #000' },
      { key: 'textOverflow', label: '文本溢出', type: 'select', options: [
        { label: '默认', value: '' },
        { label: '省略号', value: 'ellipsis' },
        { label: '裁剪', value: 'clip' }
      ]},
      { key: 'wordBreak', label: '单词断行', type: 'select', options: [
        { label: '默认', value: 'normal' },
        { label: '保持所有', value: 'keep-all' },
        { label: '断开所有', value: 'break-all' }
      ]},
      { key: 'wordWrap', label: '换行处理', type: 'select', options: [
        { label: '默认', value: 'normal' },
        { label: '强制换行', value: 'break-word' }
      ]},
      { key: 'whiteSpace', label: '空白处理', type: 'select', options: [
        { label: '默认', value: 'normal' },
        { label: '不换行', value: 'nowrap' },
        { label: '预格式化', value: 'pre' },
        { label: '预格式化+换行', value: 'pre-wrap' },
        { label: '预格式化+不换行', value: 'pre-line' }
      ]}
    ],
    
    // 列表设置
    list: [
      { key: 'listStyleType', label: '列表样式类型', type: 'select', options: [
        { label: '无', value: 'none' },
        { label: '圆点', value: 'disc' },
        { label: '圆圈', value: 'circle' },
        { label: '方块', value: 'square' },
        { label: '数字', value: 'decimal' },
        { label: '小写罗马数字', value: 'lower-roman' },
        { label: '大写罗马数字', value: 'upper-roman' }
      ]},
      { key: 'listStylePosition', label: '列表样式位置', type: 'select', options: [
        { label: '内部', value: 'inside' },
        { label: '外部', value: 'outside' }
      ]},
      { key: 'listStyleImage', label: '列表样式图片', type: 'input', placeholder: '例: url(image.png)' }
    ],
    
    // 交互设置
    interaction: [
      { key: 'cursor', label: '光标', type: 'select', options: [
        { label: '默认', value: 'default' },
        { label: '指针', value: 'pointer' },
        { label: '等待', value: 'wait' },
        { label: '帮助', value: 'help' },
        { label: '文本', value: 'text' },
        { label: '移动', value: 'move' },
        { label: '不允许', value: 'not-allowed' }
      ]},
      { key: 'visibility', label: '可见性', type: 'select', options: [
        { label: '可见', value: 'visible' },
        { label: '隐藏', value: 'hidden' },
        { label: '折叠', value: 'collapse' }
      ]},
      { key: 'userSelect', label: '用户选择', type: 'select', options: [
        { label: '默认', value: '' },
        { label: '可选择', value: 'text' },
        { label: '不可选择', value: 'none' },
        { label: '所有元素', value: 'all' }
      ]},
      { key: 'overflow', label: '溢出', type: 'select', options: [
        { label: '可见', value: 'visible' },
        { label: '隐藏', value: 'hidden' },
        { label: '滚动', value: 'scroll' },
        { label: '自动', value: 'auto' }
      ]}
    ],
    
    // 效果设置
    effect: [
      { key: 'boxShadow', label: '阴影', type: 'input', placeholder: '例: 0 2px 4px rgba(0,0,0,0.1)' },
      { key: 'transform', label: '变换', type: 'input', placeholder: '例: rotate(45deg) scale(1.2)' },
      { key: 'filter', label: '滤镜', type: 'input', placeholder: '例: blur(2px)' },
      { key: 'outline', label: '轮廓', type: 'input', placeholder: '例: 2px solid #000' }
    ],
    
    // 动画设置
    animation: [
      { key: 'transition', label: '过渡属性', type: 'input', placeholder: '例: all 0.3s ease' },
      { key: 'animation', label: '动画', type: 'input', placeholder: '例: slide 1s ease-in-out' }
    ],
    
    // 方向设置
    direction: [
      { key: 'direction', label: '文本方向', type: 'select', options: [
        { label: '默认', value: 'ltr' },
        { label: '从右到左', value: 'rtl' }
      ]},
      { key: 'writingMode', label: '书写模式', type: 'select', options: [
        { label: '水平', value: 'horizontal-tb' },
        { label: '垂直', value: 'vertical-rl' },
        { label: '垂直(左到右)', value: 'vertical-lr' }
      ]}
    ]
  }
})

// 所有样式树形结构
const allStylesTree = computed(() => {
  return [
    {
      id: 'basic',
      label: '常用设置',
      children: allStylesDefinition.value.basic.map(item => ({
        id: `basic-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'boxModel',
      label: '盒模型',
      children: allStylesDefinition.value.boxModel.map(item => ({
        id: `boxModel-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'border',
      label: '边框',
      children: allStylesDefinition.value.border.map(item => ({
        id: `border-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'layout',
      label: '布局',
      children: allStylesDefinition.value.layout.map(item => ({
        id: `layout-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'background',
      label: '背景',
      children: allStylesDefinition.value.background.map(item => ({
        id: `background-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'text',
      label: '文本',
      children: allStylesDefinition.value.text.map(item => ({
        id: `text-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'list',
      label: '列表',
      children: allStylesDefinition.value.list.map(item => ({
        id: `list-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'interaction',
      label: '交互',
      children: allStylesDefinition.value.interaction.map(item => ({
        id: `interaction-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'effect',
      label: '效果',
      children: allStylesDefinition.value.effect.map(item => ({
        id: `effect-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'animation',
      label: '动画',
      children: allStylesDefinition.value.animation.map(item => ({
        id: `animation-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    },
    {
      id: 'direction',
      label: '方向',
      children: allStylesDefinition.value.direction.map(item => ({
        id: `direction-${item.key}`,
        label: item.label,
        key: item.key,
        type: 'style'
      }))
    }
  ]
})

// 可见的基本设置
const visibleBasicSettings = computed(() => {
  return allStylesDefinition.value.basic.filter(item => visibleStyles.value.has(item.key))
})

// 可见的盒模型设置
const visibleBoxModelSettings = computed(() => {
  return allStylesDefinition.value.boxModel.filter(item => visibleStyles.value.has(item.key))
})

// 可见的边框设置
const visibleBorderSettings = computed(() => {
  return allStylesDefinition.value.border.filter(item => visibleStyles.value.has(item.key))
})

// 可见的布局设置
const visibleLayoutSettings = computed(() => {
  return allStylesDefinition.value.layout.filter(item => visibleStyles.value.has(item.key))
})

// 可见的背景设置
const visibleBackgroundSettings = computed(() => {
  return allStylesDefinition.value.background.filter(item => visibleStyles.value.has(item.key))
})

// 可见的文本设置
const visibleTextSettings = computed(() => {
  return allStylesDefinition.value.text.filter(item => visibleStyles.value.has(item.key))
})

// 可见的列表设置
const visibleListSettings = computed(() => {
  return allStylesDefinition.value.list.filter(item => visibleStyles.value.has(item.key))
})

// 可见的交互设置
const visibleInteractionSettings = computed(() => {
  return allStylesDefinition.value.interaction.filter(item => visibleStyles.value.has(item.key))
})

// 可见的效果设置
const visibleEffectSettings = computed(() => {
  return allStylesDefinition.value.effect.filter(item => visibleStyles.value.has(item.key))
})

// 可见的动画设置
const visibleAnimationSettings = computed(() => {
  return allStylesDefinition.value.animation.filter(item => visibleStyles.value.has(item.key))
})

// 可见的方向设置
const visibleDirectionSettings = computed(() => {
  return allStylesDefinition.value.direction.filter(item => visibleStyles.value.has(item.key))
})

// 检查样式是否可见
const isVisibleStyle = (key) => {
  return visibleStyles.value.has(key)
}

// 获取样式图标
const getStyleIcon = (key) => {
  const iconMap = {
    color: Brush,
    backgroundColor: MagicStick,
    backgroundImage: Picture,
    width: Box,
    height: Box,
    padding: Crop,
    margin: Crop,
    borderStyle: Box,
    position: Position,
    display: Monitor,
    textAlign: Close
  }
  
  return iconMap[key] || null
}

// 初始化数据
const initializeLocalStyle = () => {
  // 初始化所有样式属性
  const allStyleKeys = Object.values(allStylesDefinition.value).flat().map(item => item.key)
  
  // 设置默认值
  allStyleKeys.forEach(key => {
    //全部默认值
    localStyle[key] = getDefaultValue(key)
  })
  
  // 初始化文本相关默认值
  Object.assign(localTextStyle, getTextStyleDefaults())
  
  // 初始化容器相关默认值
  Object.assign(localContainerStyle, getContainerStyleDefaults())
  
  // 合并传入的样式
  if (props.modelValue) {
    Object.assign(localStyle, props.modelValue)
    
    // 处理自定义字体族
    initCustomFontFamily(localStyle.fontFamily)
  }
  
  // 初始化可见样式集合（默认显示常用设置）
  allStylesDefinition.value.basic.forEach(item => {
    visibleStyles.value.add(item.key)
  })
}

// 检查字体族是否为预设值
const isPredefinedFontFamily = (fontFamily) => {
  const predefinedFonts = allStylesDefinition.value.text
    .find(item => item.key === 'fontFamily')
    ?.options
    .filter(option => option.value !== 'custom')
    .map(option => option.value) || []
  
  return predefinedFonts.includes(fontFamily)
}

// 获取默认值
onMounted(() => {
  // 初始化全部默认值（合并文本和容器样式默认值）
  const textStyleDefaults = getTextStyleDefaults()
  const containerStyleDefaults = getContainerStyleDefaults()
  Object.assign(localStyle, textStyleDefaults, containerStyleDefaults)
  
  // 初始化文本相关默认值
  Object.assign(localTextStyle, textStyleDefaults)
  
  // 初始化容器相关默认值
  Object.assign(localContainerStyle, containerStyleDefaults)
  
  // 合并传入的样式
  if (props.modelValue) {
    Object.assign(localStyle, props.modelValue)
    
    // 处理自定义字体族
    initCustomFontFamily(localStyle.fontFamily)
  }
  
  // 初始化可见样式集合（默认显示常用设置）
  allStylesDefinition.value.basic.forEach(item => {
    visibleStyles.value.add(item.key)
  })
  
  // 同时初始化盒模型相关样式为可见（确保padding等样式默认可见）
  allStylesDefinition.value.boxModel.forEach(item => {
    visibleStyles.value.add(item.key)
  })
  
  // 强制触发响应式更新
  visibleStyles.value = new Set(visibleStyles.value)
})

const getDefaultValue = (key) => {
  const defaults = {
    fontSize: 16,
    color: '#000000',
    fontWeight: 'normal',
    fontStyle: 'normal',
    textAlign: 'left',
    lineHeight: 1.2,
    backgroundColor: '#ffffff',
    opacity: 1,
    textRotation: 0,
    scale: 1,
    fontFamily: '', // 默认不设置字体族
    padding: 0,
    margin: 0,
    borderStyle: 'none',
    borderWidth: 1,
    borderColor: '#000000',
    borderRadius: 0,
    width: '',
    height: '',
    maxWidth: '',
    maxHeight: '',
    minWidth: '',
    minHeight: '',
    display: '',
    position: 'static',
    left: '',
    top: '',
    right: '',
    bottom: '',
    boxShadow: '',
    transform: '',
    textDecoration: 'none',
    textTransform: 'none',
    letterSpacing: 0,
    wordSpacing: 0,
    fontFamily: '',
    fontVariant: 'normal',
    verticalAlign: 'baseline',
    overflow: 'visible',
    cursor: 'default',
    visibility: 'visible',
    zIndex: 0,
    float: 'none',
    clear: 'none',
    flexDirection: 'row',
    flexWrap: 'nowrap',
    justifyContent: 'flex-start',
    alignItems: 'stretch',
    gridTemplateColumns: '',
    gridTemplateRows: '',
    gridColumnGap: 0,
    gridRowGap: 0,
    backgroundImage: '',
    backgroundRepeat: 'repeat',
    backgroundSize: 'auto',
    backgroundPosition: '',
    backgroundAttachment: 'scroll',
    textIndent: 0,
    textShadow: '',
    textOverflow: '',
    wordBreak: 'normal',
    wordWrap: 'normal',
    listStyleType: 'none',
    listStylePosition: 'outside',
    listStyleImage: '',
    transition: '',
    animation: '',
    filter: '',
    outline: '',
    whiteSpace: 'normal',
    userSelect: '',
    boxSizing: '',
    direction: 'ltr',
    writingMode: 'horizontal-tb'
  }
  
  return defaults[key]
}

onMounted(() => {
  initializeLocalStyle()
})

// 监听modelValue变化
watch(() => props.modelValue, (newVal) => {
  if (newVal) {
    Object.assign(localStyle, newVal)
  }
}, { deep: true })

// 更新样式
const updateStyle = () => {
  // 发出更新事件
  emit('update:modelValue', { ...localStyle })
  emit('change', { ...localStyle })
}

// 应用样式
const applyStyle = () => {
  emit('apply', { ...localStyle })
}

// 重置样式
const resetStyle = () => {
  const allStyleKeys = Object.values(allStylesDefinition.value).flat().map(item => item.key)
  allStyleKeys.forEach(key => {
    localStyle[key] = getDefaultValue(key)
  })
  updateStyle()
}

// 转换为CSS对象
const toCSS = () => {
  const css = {}
  
  // 只处理可见的样式属性
  const visibleStyleKeys = Array.from(visibleStyles.value)
  
  // 常用设置
  if (visibleStyles.value.has('fontSize') && localStyle.fontSize) css['font-size'] = `${localStyle.fontSize}px`
  if (visibleStyles.value.has('color') && localStyle.color) css['color'] = localStyle.color
  if (visibleStyles.value.has('fontFamily') && localStyle.fontFamily) {
    // 处理字体族，如果是自定义则使用customFontFamily的值
    const fontFamilyValue = localStyle.fontFamily === 'custom' ? customFontFamily.value : localStyle.fontFamily
    if (fontFamilyValue) css['font-family'] = fontFamilyValue
  }
  if (visibleStyles.value.has('fontWeight') && localStyle.fontWeight && localStyle.fontWeight !== 'normal') css['font-weight'] = localStyle.fontWeight
  if (visibleStyles.value.has('fontStyle') && localStyle.fontStyle && localStyle.fontStyle !== 'normal') css['font-style'] = localStyle.fontStyle
  if (visibleStyles.value.has('textAlign') && localStyle.textAlign && localStyle.textAlign !== 'left') css['text-align'] = localStyle.textAlign
  if (visibleStyles.value.has('lineHeight') && localStyle.lineHeight && localStyle.lineHeight !== 1.2) css['line-height'] = localStyle.lineHeight
  if (visibleStyles.value.has('backgroundColor') && localStyle.backgroundColor) css['background-color'] = localStyle.backgroundColor
  if (visibleStyles.value.has('opacity') && localStyle.opacity !== 1) css['opacity'] = localStyle.opacity
  
  // 处理旋转和缩放
  if (visibleStyles.value.has('textRotation') || visibleStyles.value.has('scale')) {
    const rotation = localStyle.textRotation || 0;
    const scale = localStyle.scale || 1;
    css['transform'] = `rotate(${rotation}deg) scale(${scale})`;
  } else if (visibleStyles.value.has('transform') && localStyle.transform) {
    css['transform'] = localStyle.transform;
  }
  
  // 高级设置
  if (visibleStyles.value.has('padding') && localStyle.padding !== undefined) css['padding'] = `${localStyle.padding}px`
  if (visibleStyles.value.has('margin') && localStyle.margin !== undefined) css['margin'] = `${localStyle.margin}px`
  
  // 边框设置
  if (visibleStyles.value.has('borderStyle') && localStyle.borderStyle && localStyle.borderStyle !== 'none') {
    if (visibleStyles.value.has('borderWidth')) css['border-width'] = `${localStyle.borderWidth}px`
    css['border-style'] = localStyle.borderStyle
    if (visibleStyles.value.has('borderColor') && localStyle.borderColor) css['border-color'] = localStyle.borderColor
  } else if (visibleStyles.value.has('borderStyle') && localStyle.borderStyle === 'none') {
    css['border'] = 'none'
  }
  
  if (visibleStyles.value.has('borderRadius') && localStyle.borderRadius !== undefined && localStyle.borderRadius !== 0) css['border-radius'] = `${localStyle.borderRadius}px`
  if (visibleStyles.value.has('width') && localStyle.width) css['width'] = localStyle.width
  if (visibleStyles.value.has('height') && localStyle.height) css['height'] = localStyle.height
  if (visibleStyles.value.has('maxWidth') && localStyle.maxWidth) css['max-width'] = localStyle.maxWidth
  if (visibleStyles.value.has('maxHeight') && localStyle.maxHeight) css['max-height'] = localStyle.maxHeight
  if (visibleStyles.value.has('minWidth') && localStyle.minWidth) css['min-width'] = localStyle.minWidth
  if (visibleStyles.value.has('minHeight') && localStyle.minHeight) css['min-height'] = localStyle.minHeight
  if (visibleStyles.value.has('display') && localStyle.display) css['display'] = localStyle.display
  if (visibleStyles.value.has('position') && localStyle.position && localStyle.position !== 'static') {
    css['position'] = localStyle.position
    if (visibleStyles.value.has('left') && localStyle.left) css['left'] = localStyle.left
    if (visibleStyles.value.has('top') && localStyle.top) css['top'] = localStyle.top
    if (visibleStyles.value.has('right') && localStyle.right) css['right'] = localStyle.right
    if (visibleStyles.value.has('bottom') && localStyle.bottom) css['bottom'] = localStyle.bottom
  }
  if (visibleStyles.value.has('boxShadow') && localStyle.boxShadow) css['box-shadow'] = localStyle.boxShadow
  if (visibleStyles.value.has('textDecoration') && localStyle.textDecoration && localStyle.textDecoration !== 'none') css['text-decoration'] = localStyle.textDecoration
  if (visibleStyles.value.has('textTransform') && localStyle.textTransform && localStyle.textTransform !== 'none') css['text-transform'] = localStyle.textTransform
  if (visibleStyles.value.has('letterSpacing') && localStyle.letterSpacing) css['letter-spacing'] = `${localStyle.letterSpacing}px`
  if (visibleStyles.value.has('wordSpacing') && localStyle.wordSpacing) css['word-spacing'] = `${localStyle.wordSpacing}px`
  if (visibleStyles.value.has('fontFamily') && localStyle.fontFamily) css['font-family'] = localStyle.fontFamily
  if (visibleStyles.value.has('fontVariant') && localStyle.fontVariant && localStyle.fontVariant !== 'normal') css['font-variant'] = localStyle.fontVariant
  if (visibleStyles.value.has('verticalAlign') && localStyle.verticalAlign && localStyle.verticalAlign !== 'baseline') css['vertical-align'] = localStyle.verticalAlign
  if (visibleStyles.value.has('overflow') && localStyle.overflow && localStyle.overflow !== 'visible') css['overflow'] = localStyle.overflow
  if (visibleStyles.value.has('cursor') && localStyle.cursor && localStyle.cursor !== 'default') css['cursor'] = localStyle.cursor
  if (visibleStyles.value.has('visibility') && localStyle.visibility && localStyle.visibility !== 'visible') css['visibility'] = localStyle.visibility
  if (visibleStyles.value.has('zIndex') && localStyle.zIndex && localStyle.zIndex !== 0) css['z-index'] = localStyle.zIndex
  if (visibleStyles.value.has('float') && localStyle.float && localStyle.float !== 'none') css['float'] = localStyle.float
  if (visibleStyles.value.has('clear') && localStyle.clear && localStyle.clear !== 'none') css['clear'] = localStyle.clear
  
  // Flexbox相关属性
  if (visibleStyles.value.has('display') && localStyle.display === 'flex') {
    if (visibleStyles.value.has('flexDirection') && localStyle.flexDirection && localStyle.flexDirection !== 'row') css['flex-direction'] = localStyle.flexDirection
    if (visibleStyles.value.has('flexWrap') && localStyle.flexWrap && localStyle.flexWrap !== 'nowrap') css['flex-wrap'] = localStyle.flexWrap
    if (visibleStyles.value.has('justifyContent') && localStyle.justifyContent && localStyle.justifyContent !== 'flex-start') css['justify-content'] = localStyle.justifyContent
    if (visibleStyles.value.has('alignItems') && localStyle.alignItems && localStyle.alignItems !== 'stretch') css['align-items'] = localStyle.alignItems
  }
  
  // Grid相关属性
  if (visibleStyles.value.has('display') && localStyle.display === 'grid') {
    if (visibleStyles.value.has('gridTemplateColumns') && localStyle.gridTemplateColumns) css['grid-template-columns'] = localStyle.gridTemplateColumns
    if (visibleStyles.value.has('gridTemplateRows') && localStyle.gridTemplateRows) css['grid-template-rows'] = localStyle.gridTemplateRows
    if (visibleStyles.value.has('gridColumnGap') && localStyle.gridColumnGap) css['grid-column-gap'] = `${localStyle.gridColumnGap}px`
    if (visibleStyles.value.has('gridRowGap') && localStyle.gridRowGap) css['grid-row-gap'] = `${localStyle.gridRowGap}px`
  }
  
  // 背景相关属性
  if (visibleStyles.value.has('backgroundImage') && localStyle.backgroundImage) css['background-image'] = localStyle.backgroundImage
  if (visibleStyles.value.has('backgroundRepeat') && localStyle.backgroundRepeat && localStyle.backgroundRepeat !== 'repeat') css['background-repeat'] = localStyle.backgroundRepeat
  if (visibleStyles.value.has('backgroundSize') && localStyle.backgroundSize && localStyle.backgroundSize !== 'auto') css['background-size'] = localStyle.backgroundSize
  if (visibleStyles.value.has('backgroundPosition') && localStyle.backgroundPosition) css['background-position'] = localStyle.backgroundPosition
  if (visibleStyles.value.has('backgroundAttachment') && localStyle.backgroundAttachment && localStyle.backgroundAttachment !== 'scroll') css['background-attachment'] = localStyle.backgroundAttachment
  
  // 文本相关属性
  if (visibleStyles.value.has('textIndent') && localStyle.textIndent) css['text-indent'] = `${localStyle.textIndent}px`
  if (visibleStyles.value.has('textShadow') && localStyle.textShadow) css['text-shadow'] = localStyle.textShadow
  if (visibleStyles.value.has('textOverflow') && localStyle.textOverflow) css['text-overflow'] = localStyle.textOverflow
  if (visibleStyles.value.has('wordBreak') && localStyle.wordBreak && localStyle.wordBreak !== 'normal') css['word-break'] = localStyle.wordBreak
  if (visibleStyles.value.has('wordWrap') && localStyle.wordWrap && localStyle.wordWrap !== 'normal') css['word-wrap'] = localStyle.wordWrap
  if (visibleStyles.value.has('whiteSpace') && localStyle.whiteSpace && localStyle.whiteSpace !== 'normal') css['white-space'] = localStyle.whiteSpace
  
  // 列表样式属性
  if (visibleStyles.value.has('listStyleType') && localStyle.listStyleType && localStyle.listStyleType !== 'none') css['list-style-type'] = localStyle.listStyleType
  if (visibleStyles.value.has('listStylePosition') && localStyle.listStylePosition && localStyle.listStylePosition !== 'outside') css['list-style-position'] = localStyle.listStylePosition
  if (visibleStyles.value.has('listStyleImage') && localStyle.listStyleImage) css['list-style-image'] = localStyle.listStyleImage
  
  // 过渡和动画属性
  if (visibleStyles.value.has('transition') && localStyle.transition) css['transition'] = localStyle.transition
  if (visibleStyles.value.has('animation') && localStyle.animation) css['animation'] = localStyle.animation
  
  // 滤镜属性
  if (visibleStyles.value.has('filter') && localStyle.filter) css['filter'] = localStyle.filter
  
  // outline属性
  if (visibleStyles.value.has('outline') && localStyle.outline) css['outline'] = localStyle.outline
  
  // 用户选择属性
  if (visibleStyles.value.has('userSelect') && localStyle.userSelect) css['user-select'] = localStyle.userSelect
  
  // 盒模型属性
  if (visibleStyles.value.has('boxSizing') && localStyle.boxSizing) css['box-sizing'] = localStyle.boxSizing
  
  // 方向属性
  if (visibleStyles.value.has('direction') && localStyle.direction && localStyle.direction !== 'ltr') css['direction'] = localStyle.direction
  if (visibleStyles.value.has('writingMode') && localStyle.writingMode && localStyle.writingMode !== 'horizontal-tb') css['writing-mode'] = localStyle.writingMode
  
  return css
}

// 处理字体族变化
const handleFontFamilyChange = (value) => {
  if (value === 'custom') {
    // 如果选择自定义，使用当前自定义值或空字符串
    localStyle.fontFamily = customFontFamily.value || ''
  } else {
    // 如果选择预设值，直接使用
    localStyle.fontFamily = value
  }
  updateStyle()
}

// 更新自定义字体族
const updateCustomFontFamily = () => {
  // 只有当字体族设置为自定义时才更新
  if (localStyle.fontFamily === 'custom') {
    localStyle.fontFamily = customFontFamily.value || ''
    updateStyle()
  }
}

// 检查字体族是否为预设值
const isPresetFontFamily = (fontFamily) => {
  if (!fontFamily) return false
  
  // 获取所有预设字体族选项
  const textSettings = allStylesDefinition.value.text
  const fontFamilySetting = textSettings.find(setting => setting.key === 'fontFamily')
  
  if (!fontFamilySetting) return false
  
  // 检查是否为预设选项（除了"自定义"）
  return fontFamilySetting.options.some(option => 
    option.value === fontFamily && option.value !== 'custom'
  )
}

// 初始化自定义字体族
const initCustomFontFamily = (fontFamily) => {
  // 如果是预设字体族，则清空自定义字体
  if (isPresetFontFamily(fontFamily)) {
    customFontFamily.value = ''
  } else if (fontFamily) {
    // 如果不是预设字体族，则设置为自定义字体
    customFontFamily.value = fontFamily
    // 如果当前字体族不是"自定义"，则设置为"自定义"
    if (localStyle.fontFamily !== 'custom') {
      localStyle.fontFamily = 'custom'
    }
  }
}

// 复制样式
const copyStyle = () => {
  // 创建一个副本用于复制，处理自定义字体族
  const styleToCopy = { ...localStyle }
  if (styleToCopy.fontFamily === 'custom') {
    styleToCopy.fontFamily = customFontFamily.value || ''
  }
  
  const styleText = JSON.stringify(styleToCopy, null, 2)
  navigator.clipboard.writeText(styleText).then(() => {
    ElMessage.success('样式已复制到剪贴板')
  }).catch(() => {
    ElMessage.error('复制失败')
  })
}

// 粘贴样式
const pasteStyle = () => {
  navigator.clipboard.readText().then(text => {
    try {
      const parsedStyle = JSON.parse(text)
      
      // 处理字体族
      if (parsedStyle.fontFamily) {
        if (!isPresetFontFamily(parsedStyle.fontFamily)) {
          // 如果不是预设字体族，设置为自定义
          customFontFamily.value = parsedStyle.fontFamily
          parsedStyle.fontFamily = 'custom'
        }
      }
      
      Object.assign(localStyle, parsedStyle)
      updateStyle()
      ElMessage.success('样式已粘贴')
    } catch (e) {
      ElMessage.error('粘贴的样式格式不正确')
    }
  }).catch(() => {
    ElMessage.error('粘贴失败')
  })
}

// 切换样式可见性
const toggleStyleVisibility = (data) => {
  if (data && data.type === 'style' && data.key) {
    if (visibleStyles.value.has(data.key)) {
      visibleStyles.value.delete(data.key)
      ElMessage.success(`已隐藏样式: ${data.label}`)
    } else {
      visibleStyles.value.add(data.key)
      ElMessage.success(`已显示样式: ${data.label}`)
    }
    
    // 强制更新视图
    visibleStyles.value = new Set(visibleStyles.value)
  }
}

// 处理工具栏项目点击事件
const handleToolbarItemClick = (event, item) => {
  // Ctrl+单击切换样式可见性
  if (event.ctrlKey) {
    toggleStyleVisibility(item)
  } else {
    // 普通单击打开样式设置
    openStyleSettings(item)
  }
}

// 打开样式设置对话框
const openStyleSettings = (data) => {
  if (data && data.type === 'style' && data.key) {
    // 查找对应的设置项
    const allStyles = Object.values(allStylesDefinition.value).flat()
    const settingItem = allStyles.find(item => item.key === data.key)
    
    if (settingItem) {
      // 检查样式是否可见，如果不可见则提示并设置为可见
      if (!visibleStyles.value.has(data.key)) {
        visibleStyles.value.add(data.key)
        ElMessage.info(`已显示样式: ${data.label}，现在可以进行设置了`)
      }
      
      currentSettingItem.value = settingItem
      styleSettingsDialogVisible.value = true
    }
  }
}

// 获取文本相关样式的默认值
const getTextStyleDefaults = () => {
  const textStyleDefaults = {}
  
  // 文本相关样式分类
  const textStyleCategories = ['basic', 'text']
  
  textStyleCategories.forEach(category => {
    if (allStylesDefinition.value[category]) {
      allStylesDefinition.value[category].forEach(item => {
        textStyleDefaults[item.key] = getDefaultValue(item.key)
      })
    }
  })
  
  // 额外的文本相关属性
  const additionalTextKeys = [
    'textDecoration', 'textTransform', 'letterSpacing', 'wordSpacing',
    'fontVariant', 'verticalAlign', 'textIndent', 'textShadow',
    'textOverflow', 'wordBreak', 'wordWrap', 'whiteSpace',
    'direction', 'writingMode'
  ]
  
  additionalTextKeys.forEach(key => {
    if (!textStyleDefaults.hasOwnProperty(key)) {
      textStyleDefaults[key] = getDefaultValue(key)
    }
  })
  
  return textStyleDefaults
}

// 获取容器相关样式的默认值
const getContainerStyleDefaults = () => {
  const containerStyleDefaults = {}
  
  // 容器相关样式分类
  const containerStyleCategories = ['boxModel', 'border', 'layout', 'background', 'effect', 'animation', 'list', 'interaction']
  
  containerStyleCategories.forEach(category => {
    if (allStylesDefinition.value[category]) {
      allStylesDefinition.value[category].forEach(item => {
        containerStyleDefaults[item.key] = getDefaultValue(item.key)
      })
    }
  })
  
  // 额外的容器相关属性
  const additionalContainerKeys = [
    'display', 'position', 'left', 'top', 'right', 'bottom',
    'overflow', 'cursor', 'visibility', 'zIndex', 'float', 'clear',
    'flexDirection', 'flexWrap', 'justifyContent', 'alignItems',
    'gridTemplateColumns', 'gridTemplateRows', 'gridColumnGap', 'gridRowGap',
    'backgroundImage', 'backgroundRepeat', 'backgroundSize', 'backgroundPosition', 'backgroundAttachment',
    'listStyleType', 'listStylePosition', 'listStyleImage',
    'transition', 'animation', 'filter', 'outline',
    'userSelect', 'boxSizing'
  ]
  
  additionalContainerKeys.forEach(key => {
    if (!containerStyleDefaults.hasOwnProperty(key)) {
      containerStyleDefaults[key] = getDefaultValue(key)
    }
  })
  
  return containerStyleDefaults
}

// 暴露方法
defineExpose({
  toCSS
})
</script>

<style scoped>
.setting-item {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 12px;
}

.control-label {
  margin-right: 8px;
  font-size: 14px;
  color: #606266;
  white-space: nowrap;
  flex-shrink: 0;
}

.unit {
  margin-left: 4px;
  font-size: 12px;
  color: #909399;
}

.color-picker-wrapper {
  display: flex;
  align-items: center;
  gap: 8px;
}

.color-picker-trigger {
  width: 32px;
  height: 32px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  cursor: pointer;
}

.color-picker-trigger:hover {
  border-color: #409eff;
}

.style-manager {
  width: 100%;
}

.style-manager-header {
  padding: 10px;
  border-bottom: 1px solid #e0e0e0;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.style-manager-content {
  flex: 1;
  overflow-y: auto;
  padding: 10px;
}

.style-manager-footer {
  padding: 10px;
  border-top: 1px solid #e0e0e0;
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

/* 字体族提示样式 */
.font-family-tip {
  margin-top: 5px;
  padding: 8px;
  background-color: #f5f5f5;
  border-radius: 4px;
  font-size: 12px;
  color: #666;
}

.font-family-tip p {
  margin: 3px 0;
}

.setting-tree {
  :deep(.el-tree-node__content) {
    height: 32px;
    font-size: 12px;
  }
}

.style-manager-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.style-toolbar {
  margin-bottom: 20px;
  border-bottom: 1px solid #eee;
  padding-bottom: 20px;
}

.toolbar-category {
  margin-bottom: 15px;
}

.category-header h4 {
  margin: 0 0 10px 0;
  font-weight: 500;
  color: #333;
  font-size: 14px;
}

.toolbar-icons {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.toolbar-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 8px;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.3s;
  width: 60px;
}

.toolbar-item:hover {
  background-color: #f5f7fa;
}

.toolbar-item.active {
  background-color: #ecf5ff;
  border: 1px solid #409eff;
}

.icon-wrapper {
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 4px;
}

.icon-wrapper .el-icon {
  font-size: 18px;
  color: #666;
}

.toolbar-item.active .icon-wrapper .el-icon {
  color: #409eff;
}

.default-icon {
  width: 24px;
  height: 24px;
  border-radius: 4px;
  background-color: #f0f2f5;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  font-weight: bold;
  color: #666;
}

.toolbar-item.active .default-icon {
  background-color: #409eff;
  color: white;
}

.icon-label {
  font-size: 12px;
  color: #666;
  text-align: center;
  line-height: 1.2;
}

.toolbar-item.active .icon-label {
  color: #409eff;
  font-weight: 500;
}

.settings-section {
  margin-bottom: 20px;
}

.settings-subsection {
  margin-bottom: 15px;
}

.settings-section h4,
.settings-subsection h5 {
  margin: 0 0 12px 0;
  font-weight: 500;
  color: #333;
  border-bottom: 1px solid #eee;
  padding-bottom: 6px;
}

.settings-section h4 {
  font-size: 14px;
}

.settings-subsection h5 {
  font-size: 13px;
}

.settings-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
  gap: 12px;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.setting-item.label-top {
  flex-direction: column;
}

.setting-item.color-picker-item {
  justify-content: center;
}

.setting-item label {
  font-size: 12px;
  color: #666;
}

.button-group {
  display: flex;
  gap: 4px;
}

.button-group .el-button {
  padding: 8px;
  min-width: auto;
}

.setting-item :deep(.el-input__wrapper),
.setting-item :deep(.el-input-number) {
  width: 100%;
}

.setting-item :deep(.el-slider) {
  margin-top: 8px;
}

.setting-item :deep(.el-slider__input) {
  width: 60px;
}

.unit,
.value-display,
.switch-label {
  font-size: 12px;
  color: #999;
  margin-left: 4px;
}

.switch-label {
  margin-left: 0;
  margin-top: 4px;
}

.style-manager-footer {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  margin-top: 16px;
  padding-top: 16px;
  border-top: 1px solid #eee;
}

.setting-dialog-content {
  display: flex;
  flex-direction: column;
  gap: 12px;
}
</style>