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
        <el-divider direction="vertical" />
        <el-switch 
          v-model="showAdvanced" 
          active-text="高级设置" 
          size="small"
          @change="onAdvancedToggle"
        />
      </div>
    </div>
    
    <div class="style-manager-content">
      <!-- 常用设置 -->
      <div class="settings-section">
        <h4>常用设置</h4>
        <div class="settings-grid">
          <div class="setting-item">
            <label>字体大小</label>
            <el-input-number 
              v-model="localStyle.fontSize" 
              :min="1" 
              :max="100" 
              size="small"
              controls-position="right"
              @change="updateStyle"
            />
            <span class="unit">px</span>
          </div>
          
          <div class="setting-item">
            <label>字体颜色</label>
            <el-color-picker 
              v-model="localStyle.color" 
              size="small" 
              show-alpha
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>字体粗细</label>
            <el-select 
              v-model="localStyle.fontWeight" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="正常" value="normal" />
              <el-option label="粗体" value="bold" />
              <el-option label="100" value="100" />
              <el-option label="200" value="200" />
              <el-option label="300" value="300" />
              <el-option label="400" value="400" />
              <el-option label="500" value="500" />
              <el-option label="600" value="600" />
              <el-option label="700" value="700" />
              <el-option label="800" value="800" />
              <el-option label="900" value="900" />
            </el-select>
          </div>
          
          <div class="setting-item">
            <label>字体样式</label>
            <el-select 
              v-model="localStyle.fontStyle" 
              size="small"
              @change="updateStyle"
            >
              <el-option label="正常" value="normal" />
              <el-option label="斜体" value="italic" />
              <el-option label="倾斜" value="oblique" />
            </el-select>
          </div>
          
          <div class="setting-item">
            <label>文本对齐</label>
            <div class="button-group">
              <el-button 
                size="small" 
                :type="localStyle.textAlign === 'left' ? 'primary' : 'default'"
                @click="localStyle.textAlign = 'left'; updateStyle()"
                title="左对齐"
              >
                <el-icon><ArrowLeft /></el-icon>
              </el-button>
              <el-button 
                size="small" 
                :type="localStyle.textAlign === 'center' ? 'primary' : 'default'"
                @click="localStyle.textAlign = 'center'; updateStyle()"
                title="居中"
              >
                <el-icon><Close /></el-icon>
              </el-button>
              <el-button 
                size="small" 
                :type="localStyle.textAlign === 'right' ? 'primary' : 'default'"
                @click="localStyle.textAlign = 'right'; updateStyle()"
                title="右对齐"
              >
                <el-icon><ArrowRight /></el-icon>
              </el-button>
              <el-button 
                size="small" 
                :type="localStyle.textAlign === 'justify' ? 'primary' : 'default'"
                @click="localStyle.textAlign = 'justify'; updateStyle()"
                title="两端对齐"
              >
                <el-icon><ScaleToOriginal /></el-icon>
              </el-button>
            </div>
          </div>
          
          <div class="setting-item">
            <label>行高</label>
            <el-slider 
              v-model="localStyle.lineHeight" 
              :min="0" 
              :max="5" 
              :step="0.1"
              size="small"
              show-input
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>背景颜色</label>
            <el-color-picker 
              v-model="localStyle.backgroundColor" 
              size="small" 
              show-alpha
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>透明度</label>
            <el-slider 
              v-model="localStyle.opacity" 
              :min="0" 
              :max="1" 
              :step="0.01"
              size="small"
              show-input
              @change="updateStyle"
            />
          </div>
          
          <div class="setting-item">
            <label>文字旋转</label>
            <el-slider 
              v-model="localStyle.textRotation" 
              :min="0" 
              :max="360" 
              size="small"
              @change="updateStyle"
            />
            <span class="value-display">{{ localStyle.textRotation }}°</span>
          </div>
          
          <div class="setting-item">
            <label>缩放比例</label>
            <el-slider 
              v-model="localStyle.scale" 
              :min="0.1" 
              :max="5" 
              :step="0.1"
              size="small"
              @change="updateStyle"
            />
            <span class="value-display">{{ localStyle.scale || 1 }}</span>
          </div>
        </div>
      </div>
      
      <!-- 高级设置 -->
      <div v-if="showAdvanced" class="settings-section advanced-section">
        <h4>高级设置</h4>
        
        <!-- 盒模型 -->
        <div class="settings-subsection">
          <h5>盒模型</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>内边距</label>
              <el-input-number 
                v-model="localStyle.padding" 
                :min="0" 
                :max="100" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item">
              <label>外边距</label>
              <el-input-number 
                v-model="localStyle.margin" 
                :min="0" 
                :max="100" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item">
              <label>宽度</label>
              <el-input 
                v-model="localStyle.width" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>高度</label>
              <el-input 
                v-model="localStyle.height" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>最大宽度</label>
              <el-input 
                v-model="localStyle.maxWidth" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>最大高度</label>
              <el-input 
                v-model="localStyle.maxHeight" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>最小宽度</label>
              <el-input 
                v-model="localStyle.minWidth" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>最小高度</label>
              <el-input 
                v-model="localStyle.minHeight" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>盒模型</label>
              <el-select 
                v-model="localStyle.boxSizing" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="" />
                <el-option label="内容盒" value="content-box" />
                <el-option label="边框盒" value="border-box" />
              </el-select>
            </div>
          </div>
        </div>
        
        <!-- 边框 -->
        <div class="settings-subsection">
          <h5>边框</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>边框</label>
              <el-select 
                v-model="localStyle.borderStyle" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="无" value="none" />
                <el-option label="实线" value="solid" />
                <el-option label="虚线" value="dashed" />
                <el-option label="点线" value="dotted" />
                <el-option label="双线" value="double" />
                <el-option label="凹槽" value="groove" />
                <el-option label=" ridge" value="ridge" />
                <el-option label="插入" value="inset" />
                <el-option label="突出" value="outset" />
              </el-select>
            </div>
            
            <div class="setting-item" v-if="localStyle.borderStyle !== 'none'">
              <label>边框宽度</label>
              <el-input-number 
                v-model="localStyle.borderWidth" 
                :min="0" 
                :max="20" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item" v-if="localStyle.borderStyle !== 'none'">
              <label>边框颜色</label>
              <el-color-picker 
                v-model="localStyle.borderColor" 
                size="small" 
                show-alpha
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>圆角</label>
              <el-input-number 
                v-model="localStyle.borderRadius" 
                :min="0" 
                :max="50" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
          </div>
        </div>
        
        <!-- 布局 -->
        <div class="settings-subsection">
          <h5>布局</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>显示类型</label>
              <el-select 
                v-model="localStyle.display" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="" />
                <el-option label="块级" value="block" />
                <el-option label="行内" value="inline" />
                <el-option label="行内块" value="inline-block" />
                <el-option label="弹性" value="flex" />
                <el-option label="网格" value="grid" />
                <el-option label="表格" value="table" />
                <el-option label="列表项" value="list-item" />
                <el-option label="隐藏" value="none" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>定位</label>
              <el-select 
                v-model="localStyle.position" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="静态" value="static" />
                <el-option label="相对" value="relative" />
                <el-option label="绝对" value="absolute" />
                <el-option label="固定" value="fixed" />
                <el-option label="粘性" value="sticky" />
              </el-select>
            </div>
            
            <div class="setting-item" v-if="localStyle.position !== 'static'">
              <label>左边距</label>
              <el-input 
                v-model="localStyle.left" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item" v-if="localStyle.position !== 'static'">
              <label>上边距</label>
              <el-input 
                v-model="localStyle.top" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item" v-if="localStyle.position !== 'static'">
              <label>右边距</label>
              <el-input 
                v-model="localStyle.right" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item" v-if="localStyle.position !== 'static'">
              <label>下边距</label>
              <el-input 
                v-model="localStyle.bottom" 
                size="small"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>浮动</label>
              <el-select 
                v-model="localStyle.float" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="无" value="none" />
                <el-option label="左浮动" value="left" />
                <el-option label="右浮动" value="right" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>清除浮动</label>
              <el-select 
                v-model="localStyle.clear" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="无" value="none" />
                <el-option label="左清除" value="left" />
                <el-option label="右清除" value="right" />
                <el-option label="两侧清除" value="both" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>z-index</label>
              <el-input-number 
                v-model="localStyle.zIndex" 
                :min="-100" 
                :max="100" 
                size="small"
                controls-position="right"
                @change="updateStyle"
              />
            </div>
            
            <!-- Flexbox相关属性 -->
            <div class="setting-item" v-if="localStyle.display === 'flex'">
              <label>主轴方向</label>
              <el-select 
                v-model="localStyle.flexDirection" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="行" value="row" />
                <el-option label="行反转" value="row-reverse" />
                <el-option label="列" value="column" />
                <el-option label="列反转" value="column-reverse" />
              </el-select>
            </div>
            
            <div class="setting-item" v-if="localStyle.display === 'flex'">
              <label>换行</label>
              <el-select 
                v-model="localStyle.flexWrap" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="不换行" value="nowrap" />
                <el-option label="换行" value="wrap" />
                <el-option label="反转换行" value="wrap-reverse" />
              </el-select>
            </div>
            
            <div class="setting-item" v-if="localStyle.display === 'flex'">
              <label>主轴对齐</label>
              <el-select 
                v-model="localStyle.justifyContent" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="起始对齐" value="flex-start" />
                <el-option label="结束对齐" value="flex-end" />
                <el-option label="居中" value="center" />
                <el-option label="两端对齐" value="space-between" />
                <el-option label="均匀分布" value="space-around" />
                <el-option label="完全均匀分布" value="space-evenly" />
              </el-select>
            </div>
            
            <div class="setting-item" v-if="localStyle.display === 'flex'">
              <label>交叉轴对齐</label>
              <el-select 
                v-model="localStyle.alignItems" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="拉伸" value="stretch" />
                <el-option label="起始对齐" value="flex-start" />
                <el-option label="结束对齐" value="flex-end" />
                <el-option label="居中" value="center" />
                <el-option label="基线对齐" value="baseline" />
              </el-select>
            </div>
            
            <!-- Grid相关属性 -->
            <div class="setting-item" v-if="localStyle.display === 'grid'">
              <label>网格模板列</label>
              <el-input 
                v-model="localStyle.gridTemplateColumns" 
                size="small"
                placeholder="例: 1fr 1fr 1fr"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item" v-if="localStyle.display === 'grid'">
              <label>网格模板行</label>
              <el-input 
                v-model="localStyle.gridTemplateRows" 
                size="small"
                placeholder="例: auto 1fr auto"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item" v-if="localStyle.display === 'grid'">
              <label>网格列间隙</label>
              <el-input-number 
                v-model="localStyle.gridColumnGap" 
                :min="0" 
                :max="100" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item" v-if="localStyle.display === 'grid'">
              <label>网格行间隙</label>
              <el-input-number 
                v-model="localStyle.gridRowGap" 
                :min="0" 
                :max="100" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
          </div>
        </div>
        
        <!-- 背景 -->
        <div class="settings-subsection">
          <h5>背景</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>背景图片</label>
              <el-input 
                v-model="localStyle.backgroundImage" 
                size="small"
                placeholder="例: url(image.jpg)"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>背景重复</label>
              <el-select 
                v-model="localStyle.backgroundRepeat" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="重复" value="repeat" />
                <el-option label="不重复" value="no-repeat" />
                <el-option label="X轴重复" value="repeat-x" />
                <el-option label="Y轴重复" value="repeat-y" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>背景大小</label>
              <el-select 
                v-model="localStyle.backgroundSize" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="自动" value="auto" />
                <el-option label="覆盖" value="cover" />
                <el-option label="包含" value="contain" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>背景位置</label>
              <el-input 
                v-model="localStyle.backgroundPosition" 
                size="small"
                placeholder="例: center center"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>背景附着</label>
              <el-select 
                v-model="localStyle.backgroundAttachment" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="滚动" value="scroll" />
                <el-option label="固定" value="fixed" />
                <el-option label="局部" value="local" />
              </el-select>
            </div>
          </div>
        </div>
        
        <!-- 文本 -->
        <div class="settings-subsection">
          <h5>文本</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>字体族</label>
              <el-input 
                v-model="localStyle.fontFamily" 
                size="small"
                placeholder="例: Arial, sans-serif"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>字体大小调整</label>
              <el-switch 
                v-model="localStyle.fontVariant" 
                active-value="small-caps"
                inactive-value="normal"
                @change="updateStyle"
              />
              <span class="switch-label">小型大写字母</span>
            </div>
            
            <div class="setting-item">
              <label>文字装饰</label>
              <el-select 
                v-model="localStyle.textDecoration" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="无" value="none" />
                <el-option label="下划线" value="underline" />
                <el-option label="上划线" value="overline" />
                <el-option label="删除线" value="line-through" />
                <el-option label="下划线+删除线" value="underline line-through" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>文字转换</label>
              <el-select 
                v-model="localStyle.textTransform" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="无" value="none" />
                <el-option label="大写" value="uppercase" />
                <el-option label="小写" value="lowercase" />
                <el-option label="首字母大写" value="capitalize" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>垂直对齐</label>
              <el-select 
                v-model="localStyle.verticalAlign" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="基线" value="baseline" />
                <el-option label="顶部" value="top" />
                <el-option label="中间" value="middle" />
                <el-option label="底部" value="bottom" />
                <el-option label="文本顶部" value="text-top" />
                <el-option label="文本底部" value="text-bottom" />
                <el-option label="上标" value="super" />
                <el-option label="下标" value="sub" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>字母间距</label>
              <el-input-number 
                v-model="localStyle.letterSpacing" 
                :min="-5" 
                :max="10" 
                :step="0.1"
                size="small"
                controls-position="right"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item">
              <label>词间距</label>
              <el-input-number 
                v-model="localStyle.wordSpacing" 
                :min="-5" 
                :max="20" 
                :step="0.1"
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item">
              <label>文本缩进</label>
              <el-input-number 
                v-model="localStyle.textIndent" 
                :min="0" 
                :max="100" 
                size="small"
                @change="updateStyle"
              />
              <span class="unit">px</span>
            </div>
            
            <div class="setting-item">
              <label>文本阴影</label>
              <el-input 
                v-model="localStyle.textShadow" 
                size="small"
                placeholder="例: 1px 1px 2px #000"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>文本溢出</label>
              <el-select 
                v-model="localStyle.textOverflow" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="" />
                <el-option label="省略号" value="ellipsis" />
                <el-option label="裁剪" value="clip" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>单词断行</label>
              <el-select 
                v-model="localStyle.wordBreak" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="normal" />
                <el-option label="保持所有" value="keep-all" />
                <el-option label="断开所有" value="break-all" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>换行处理</label>
              <el-select 
                v-model="localStyle.wordWrap" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="normal" />
                <el-option label="强制换行" value="break-word" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>空白处理</label>
              <el-select 
                v-model="localStyle.whiteSpace" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="normal" />
                <el-option label="不换行" value="nowrap" />
                <el-option label="预格式化" value="pre" />
                <el-option label="预格式化+换行" value="pre-wrap" />
                <el-option label="预格式化+不换行" value="pre-line" />
              </el-select>
            </div>
          </div>
        </div>
        
        <!-- 列表 -->
        <div class="settings-subsection">
          <h5>列表</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>列表样式类型</label>
              <el-select 
                v-model="localStyle.listStyleType" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="无" value="none" />
                <el-option label="圆点" value="disc" />
                <el-option label="圆圈" value="circle" />
                <el-option label="方块" value="square" />
                <el-option label="数字" value="decimal" />
                <el-option label="小写罗马数字" value="lower-roman" />
                <el-option label="大写罗马数字" value="upper-roman" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>列表样式位置</label>
              <el-select 
                v-model="localStyle.listStylePosition" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="内部" value="inside" />
                <el-option label="外部" value="outside" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>列表样式图片</label>
              <el-input 
                v-model="localStyle.listStyleImage" 
                size="small"
                placeholder="例: url(image.png)"
                @change="updateStyle"
              />
            </div>
          </div>
        </div>
        
        <!-- 交互 -->
        <div class="settings-subsection">
          <h5>交互</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>光标</label>
              <el-select 
                v-model="localStyle.cursor" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="default" />
                <el-option label="指针" value="pointer" />
                <el-option label="等待" value="wait" />
                <el-option label="帮助" value="help" />
                <el-option label="文本" value="text" />
                <el-option label="移动" value="move" />
                <el-option label="不允许" value="not-allowed" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>可见性</label>
              <el-select 
                v-model="localStyle.visibility" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="可见" value="visible" />
                <el-option label="隐藏" value="hidden" />
                <el-option label="折叠" value="collapse" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>用户选择</label>
              <el-select 
                v-model="localStyle.userSelect" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="" />
                <el-option label="可选择" value="text" />
                <el-option label="不可选择" value="none" />
                <el-option label="所有元素" value="all" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>溢出</label>
              <el-select 
                v-model="localStyle.overflow" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="可见" value="visible" />
                <el-option label="隐藏" value="hidden" />
                <el-option label="滚动" value="scroll" />
                <el-option label="自动" value="auto" />
              </el-select>
            </div>
          </div>
        </div>
        
        <!-- 效果 -->
        <div class="settings-subsection">
          <h5>效果</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>阴影</label>
              <el-input 
                v-model="localStyle.boxShadow" 
                size="small"
                placeholder="例: 0 2px 4px rgba(0,0,0,0.1)"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>变换</label>
              <el-input 
                v-model="localStyle.transform" 
                size="small"
                placeholder="例: rotate(45deg) scale(1.2)"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>滤镜</label>
              <el-input 
                v-model="localStyle.filter" 
                size="small"
                placeholder="例: blur(2px)"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>轮廓</label>
              <el-input 
                v-model="localStyle.outline" 
                size="small"
                placeholder="例: 2px solid #000"
                @change="updateStyle"
              />
            </div>
          </div>
        </div>
        
        <!-- 动画 -->
        <div class="settings-subsection">
          <h5>动画</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>过渡属性</label>
              <el-input 
                v-model="localStyle.transition" 
                size="small"
                placeholder="例: all 0.3s ease"
                @change="updateStyle"
              />
            </div>
            
            <div class="setting-item">
              <label>动画</label>
              <el-input 
                v-model="localStyle.animation" 
                size="small"
                placeholder="例: slide 1s ease-in-out"
                @change="updateStyle"
              />
            </div>
          </div>
        </div>
        
        <!-- 方向 -->
        <div class="settings-subsection">
          <h5>方向</h5>
          <div class="settings-grid">
            <div class="setting-item">
              <label>文本方向</label>
              <el-select 
                v-model="localStyle.direction" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="默认" value="ltr" />
                <el-option label="从右到左" value="rtl" />
              </el-select>
            </div>
            
            <div class="setting-item">
              <label>书写模式</label>
              <el-select 
                v-model="localStyle.writingMode" 
                size="small"
                @change="updateStyle"
              >
                <el-option label="水平" value="horizontal-tb" />
                <el-option label="垂直" value="vertical-rl" />
                <el-option label="垂直(左到右)" value="vertical-lr" />
              </el-select>
            </div>
          </div>
        </div>
      </div>
    </div>
    
    <div class="style-manager-footer">
      <el-button @click="resetStyle" size="small">重置</el-button>
      <el-button type="primary" @click="applyStyle" size="small">应用</el-button>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted } from 'vue'
import { 
  CopyDocument, 
  DocumentCopy, 
  ArrowLeft, 
  ArrowRight, 
  Close, 
  ScaleToOriginal 
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

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
const showAdvanced = ref(false)
const localStyle = reactive({
  // 常用设置
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
  
  // 高级设置
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
  
  // Flexbox相关属性
  flexDirection: 'row',
  flexWrap: 'nowrap',
  justifyContent: 'flex-start',
  alignItems: 'stretch',
  
  // Grid相关属性
  gridTemplateColumns: '',
  gridTemplateRows: '',
  gridColumnGap: 0,
  gridRowGap: 0,
  
  // 背景相关属性
  backgroundImage: '',
  backgroundRepeat: 'repeat',
  backgroundSize: 'auto',
  backgroundPosition: '',
  backgroundAttachment: 'scroll',
  
  // 文本相关属性
  textIndent: 0,
  textShadow: '',
  textOverflow: '',
  wordBreak: 'normal',
  wordWrap: 'normal',
  
  // 列表样式属性
  listStyleType: 'none',
  listStylePosition: 'outside',
  listStyleImage: '',
  
  // 过渡和动画属性
  transition: '',
  animation: '',
  
  // 滤镜属性
  filter: '',
  
  // outline属性
  outline: '',
  
  // white-space属性
  whiteSpace: 'normal',
  
  // 用户选择属性
  userSelect: '',
  
  // 盒模型属性
  boxSizing: '',
  
  // 方向属性
  direction: 'ltr',
  writingMode: 'horizontal-tb'
})

// 初始化数据
onMounted(() => {
  // 合并传入的样式和默认样式
  Object.assign(localStyle, props.modelValue)
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
  Object.assign(localStyle, {
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
    
    // Flexbox相关属性
    flexDirection: 'row',
    flexWrap: 'nowrap',
    justifyContent: 'flex-start',
    alignItems: 'stretch',
    
    // Grid相关属性
    gridTemplateColumns: '',
    gridTemplateRows: '',
    gridColumnGap: 0,
    gridRowGap: 0,
    
    // 背景相关属性
    backgroundImage: '',
    backgroundRepeat: 'repeat',
    backgroundSize: 'auto',
    backgroundPosition: '',
    backgroundAttachment: 'scroll',
    
    // 文本相关属性
    textIndent: 0,
    textShadow: '',
    textOverflow: '',
    wordBreak: 'normal',
    wordWrap: 'normal',
    
    // 列表样式属性
    listStyleType: 'none',
    listStylePosition: 'outside',
    listStyleImage: '',
    
    // 过渡和动画属性
    transition: '',
    animation: '',
    
    // 滤镜属性
    filter: '',
    
    // outline属性
    outline: '',
    
    // white-space属性
    whiteSpace: 'normal',
    
    // 用户选择属性
    userSelect: '',
    
    // 盒模型属性
    boxSizing: '',
    
    // 方向属性
    direction: 'ltr',
    writingMode: 'horizontal-tb'
  })
  updateStyle()
}

// 高级设置切换
const onAdvancedToggle = () => {
  // 切换时触发更新
  updateStyle()
}

// 转换为CSS对象
const toCSS = () => {
  const css = {}
  
  // 常用设置
  if (localStyle.fontSize) css['font-size'] = `${localStyle.fontSize}px`
  if (localStyle.color) css['color'] = localStyle.color
  if (localStyle.fontWeight && localStyle.fontWeight !== 'normal') css['font-weight'] = localStyle.fontWeight
  if (localStyle.fontStyle && localStyle.fontStyle !== 'normal') css['font-style'] = localStyle.fontStyle
  if (localStyle.textAlign && localStyle.textAlign !== 'left') css['text-align'] = localStyle.textAlign
  if (localStyle.lineHeight && localStyle.lineHeight !== 1.2) css['line-height'] = localStyle.lineHeight
  if (localStyle.backgroundColor) css['background-color'] = localStyle.backgroundColor
  if (localStyle.opacity !== 1) css['opacity'] = localStyle.opacity
  
  // 处理旋转和缩放
  if (localStyle.textRotation !== undefined || localStyle.scale !== undefined) {
    const rotation = localStyle.textRotation || 0;
    const scale = localStyle.scale || 1;
    css['transform'] = `rotate(${rotation}deg) scale(${scale})`;
  } else if (localStyle.transform) {
    css['transform'] = localStyle.transform;
  }
  
  // 高级设置
  if (localStyle.padding !== undefined) css['padding'] = `${localStyle.padding}px`
  if (localStyle.margin !== undefined) css['margin'] = `${localStyle.margin}px`
  
  // 边框设置
  if (localStyle.borderStyle && localStyle.borderStyle !== 'none') {
    css['border-width'] = `${localStyle.borderWidth}px`
    css['border-style'] = localStyle.borderStyle
    css['border-color'] = localStyle.borderColor
  } else if (localStyle.borderStyle === 'none') {
    css['border'] = 'none'
  }
  
  if (localStyle.borderRadius !== undefined && localStyle.borderRadius !== 0) css['border-radius'] = `${localStyle.borderRadius}px`
  if (localStyle.width) css['width'] = localStyle.width
  if (localStyle.height) css['height'] = localStyle.height
  if (localStyle.maxWidth) css['max-width'] = localStyle.maxWidth
  if (localStyle.maxHeight) css['max-height'] = localStyle.maxHeight
  if (localStyle.minWidth) css['min-width'] = localStyle.minWidth
  if (localStyle.minHeight) css['min-height'] = localStyle.minHeight
  if (localStyle.display) css['display'] = localStyle.display
  if (localStyle.position && localStyle.position !== 'static') {
    css['position'] = localStyle.position
    if (localStyle.left) css['left'] = localStyle.left
    if (localStyle.top) css['top'] = localStyle.top
    if (localStyle.right) css['right'] = localStyle.right
    if (localStyle.bottom) css['bottom'] = localStyle.bottom
  }
  if (localStyle.boxShadow) css['box-shadow'] = localStyle.boxShadow
  if (localStyle.textDecoration && localStyle.textDecoration !== 'none') css['text-decoration'] = localStyle.textDecoration
  if (localStyle.textTransform && localStyle.textTransform !== 'none') css['text-transform'] = localStyle.textTransform
  if (localStyle.letterSpacing) css['letter-spacing'] = `${localStyle.letterSpacing}px`
  if (localStyle.wordSpacing) css['word-spacing'] = `${localStyle.wordSpacing}px`
  if (localStyle.fontFamily) css['font-family'] = localStyle.fontFamily
  if (localStyle.fontVariant && localStyle.fontVariant !== 'normal') css['font-variant'] = localStyle.fontVariant
  if (localStyle.verticalAlign && localStyle.verticalAlign !== 'baseline') css['vertical-align'] = localStyle.verticalAlign
  if (localStyle.overflow && localStyle.overflow !== 'visible') css['overflow'] = localStyle.overflow
  if (localStyle.cursor && localStyle.cursor !== 'default') css['cursor'] = localStyle.cursor
  if (localStyle.visibility && localStyle.visibility !== 'visible') css['visibility'] = localStyle.visibility
  if (localStyle.zIndex && localStyle.zIndex !== 0) css['z-index'] = localStyle.zIndex
  if (localStyle.float && localStyle.float !== 'none') css['float'] = localStyle.float
  if (localStyle.clear && localStyle.clear !== 'none') css['clear'] = localStyle.clear
  
  // Flexbox相关属性
  if (localStyle.display === 'flex') {
    if (localStyle.flexDirection && localStyle.flexDirection !== 'row') css['flex-direction'] = localStyle.flexDirection
    if (localStyle.flexWrap && localStyle.flexWrap !== 'nowrap') css['flex-wrap'] = localStyle.flexWrap
    if (localStyle.justifyContent && localStyle.justifyContent !== 'flex-start') css['justify-content'] = localStyle.justifyContent
    if (localStyle.alignItems && localStyle.alignItems !== 'stretch') css['align-items'] = localStyle.alignItems
  }
  
  // Grid相关属性
  if (localStyle.display === 'grid') {
    if (localStyle.gridTemplateColumns) css['grid-template-columns'] = localStyle.gridTemplateColumns
    if (localStyle.gridTemplateRows) css['grid-template-rows'] = localStyle.gridTemplateRows
    if (localStyle.gridColumnGap) css['grid-column-gap'] = `${localStyle.gridColumnGap}px`
    if (localStyle.gridRowGap) css['grid-row-gap'] = `${localStyle.gridRowGap}px`
  }
  
  // 背景相关属性
  if (localStyle.backgroundImage) css['background-image'] = localStyle.backgroundImage
  if (localStyle.backgroundRepeat && localStyle.backgroundRepeat !== 'repeat') css['background-repeat'] = localStyle.backgroundRepeat
  if (localStyle.backgroundSize && localStyle.backgroundSize !== 'auto') css['background-size'] = localStyle.backgroundSize
  if (localStyle.backgroundPosition) css['background-position'] = localStyle.backgroundPosition
  if (localStyle.backgroundAttachment && localStyle.backgroundAttachment !== 'scroll') css['background-attachment'] = localStyle.backgroundAttachment
  
  // 文本相关属性
  if (localStyle.textIndent) css['text-indent'] = `${localStyle.textIndent}px`
  if (localStyle.textShadow) css['text-shadow'] = localStyle.textShadow
  if (localStyle.textOverflow) css['text-overflow'] = localStyle.textOverflow
  if (localStyle.wordBreak && localStyle.wordBreak !== 'normal') css['word-break'] = localStyle.wordBreak
  if (localStyle.wordWrap && localStyle.wordWrap !== 'normal') css['word-wrap'] = localStyle.wordWrap
  
  // 列表样式属性
  if (localStyle.listStyleType && localStyle.listStyleType !== 'none') css['list-style-type'] = localStyle.listStyleType
  if (localStyle.listStylePosition && localStyle.listStylePosition !== 'outside') css['list-style-position'] = localStyle.listStylePosition
  if (localStyle.listStyleImage) css['list-style-image'] = localStyle.listStyleImage
  
  // 过渡和动画属性
  if (localStyle.transition) css['transition'] = localStyle.transition
  if (localStyle.animation) css['animation'] = localStyle.animation
  
  // 滤镜属性
  if (localStyle.filter) css['filter'] = localStyle.filter
  
  // outline属性
  if (localStyle.outline) css['outline'] = localStyle.outline
  
  // white-space属性
  if (localStyle.whiteSpace && localStyle.whiteSpace !== 'normal') css['white-space'] = localStyle.whiteSpace
  
  // 用户选择属性
  if (localStyle.userSelect) css['user-select'] = localStyle.userSelect
  
  // 盒模型属性
  if (localStyle.boxSizing) css['box-sizing'] = localStyle.boxSizing
  
  // 方向属性
  if (localStyle.direction && localStyle.direction !== 'ltr') css['direction'] = localStyle.direction
  if (localStyle.writingMode && localStyle.writingMode !== 'horizontal-tb') css['writing-mode'] = localStyle.writingMode
  
  return css
}

// 复制样式
const copyStyle = () => {
  const styleText = JSON.stringify(localStyle, null, 2)
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

// 暴露方法
defineExpose({
  toCSS
})
</script>

<style scoped>
.style-manager {
  background-color: #fff;
  border-radius: 8px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  padding: 16px;
  max-width: 100%;
}

.style-manager-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
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
</style>