<template>
  <div class="file-manager">
    <h1>修文管理器</h1>

    <!-- 导航区域 -->
    <div class="navigation">
      <router-link to="/" class="home-link">返回首页</router-link>
    </div>

    <!-- 修文列表区域 -->
    <div class="file-list-container">
      <el-card class="file-list-card">
        <template #header>
          <div class="card-header">
            <span>修文列表</span>
            <div>
              <el-button type="primary" @click="showAddFileDialog" size="small">
                添加修文
              </el-button>
              <el-button type="primary" @click="loadFileList" :loading="loading" size="small">
                刷新
              </el-button>
            </div>
          </div>
        </template>

        <el-table :data="fileList" v-loading="loading" highlight-current-row @row-click="handleFileSelect"
          style="width: 100%">
          <el-table-column prop="name" label="修文名" width="200"></el-table-column>
          <el-table-column prop="status" label="状态" width="120"></el-table-column>
          <el-table-column prop="modified" label="修改时间" width="200">
            <template #default="scope">
              {{ formatDate(scope.row.modified) }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="120">
            <template #default="scope">
              <el-tooltip content="编辑" placement="top">
                <el-button type="primary" icon="Edit" size="small" circle @click.stop="useInPageManager(scope.row)" />
              </el-tooltip>
              <el-tooltip content="加载" placement="top">
                <el-button type="success" icon="Download" size="small" circle
                  @click.stop="loadFileContent(scope.row)" />
              </el-tooltip>
              <el-tooltip content="删除" placement="top">
                <el-button type="danger" icon="Delete" size="small" circle @click.stop="deleteFile(scope.row)" />
              </el-tooltip>
            </template>
          </el-table-column>
        </el-table>
      </el-card>
    </div>

    <!-- 修文内容显示区域 -->
    <div class="file-content-container" v-if="selectedFile">
      <el-card class="file-content-card">
        <template #header>
          <div class="card-header">
            <span>修文内容: {{ selectedFile.name }}</span>
            <div>
              <el-button type="success" @click="useInPageManager" size="small">
                在页面管理器中使用
              </el-button>
              <el-button @click="clearFileContent" size="small">清除</el-button>
            </div>
          </div>
        </template>

        <div class="file-content">
          <pre v-if="fileContent">{{ JSON.stringify(fileContent, null, 2) }}</pre>
          <div v-else class="no-content">
            未加载修文内容
          </div>
        </div>
      </el-card>
    </div>

    <!-- 添加修文对话框 -->
    <el-dialog v-model="addFileDialogVisible" title="添加修文" width="500px">
      <el-form>
        <el-form-item label="修文名称">
          <el-input v-model="newFileName" placeholder="请输入修文名称"></el-input>
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="addFileDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="addNewFile">确认</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- API服务管理组件 -->
    <ApiServiceManager ref="apiServiceManager" />

    <!-- 添加 PageManager 容器 -->
    <div id="page-manager-container" style="margin-top: 20px;"></div>

  </div>
</template>

<script>
import { ref, onMounted } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage, ElMessageBox } from 'element-plus';
import ApiServiceManager from '@/components/ApiServiceManager.vue';
import { createApp } from 'vue';

export default {
  name: 'FileManager',
  components: {
    ApiServiceManager
  },
  setup() {
    const router = useRouter();
    const apiServiceManager = ref(null);

    // 响应式数据
    const fileList = ref([]);
    const loading = ref(false);
    const selectedFile = ref(null);
    const fileContent = ref(null);

    // 添加修文相关
    const addFileDialogVisible = ref(false);
    const newFileName = ref('');

    // 格式化修文大小
    const formatFileSize = (size) => {
      if (size < 1024) return size + ' B';
      if (size < 1024 * 1024) return (size / 1024).toFixed(2) + ' KB';
      if (size < 1024 * 1024 * 1024) return (size / (1024 * 1024)).toFixed(2) + ' MB';
      return (size / (1024 * 1024 * 1024)).toFixed(2) + ' GB';
    };

    // 添加日期解析函数
    const parseDate = (date) => {
      if (!date) return null;

      // 如果已经是Date对象
      if (date instanceof Date) {
        return isNaN(date.getTime()) ? null : date;
      }

      // 如果是数字（时间戳）
      if (typeof date === 'number') {
        // 检查是否是秒级时间戳（小于1e10），需要乘以1000转换为毫秒
        let timestamp = date;
        if (date < 1e10) {
          timestamp = date * 1000;
        }
        const d = new Date(timestamp);
        return isNaN(d.getTime()) ? null : d;
      }

      // 如果是字符串
      if (typeof date === 'string') {
        // 尝试直接解析
        let d = new Date(date);
        if (!isNaN(d.getTime())) {
          return d;
        }

        // 尝试解析时间戳字符串
        const timestamp = Number(date);
        if (!isNaN(timestamp)) {
          // 检查是否是秒级时间戳（小于1e10），需要乘以1000转换为毫秒
          let ts = timestamp;
          if (timestamp < 1e10) {
            ts = timestamp * 1000;
          }
          d = new Date(ts);
          if (!isNaN(d.getTime())) {
            return d;
          }
        }

        // 尝试解析其他常见格式
        // ISO格式: 2023-01-01T10:00:00Z 或 2023-01-01T10:00:00.000Z
        // 日期格式: 2023-01-01 10:00:00 或 2023/01/01 10:00:00
        const formats = [
          date.replace(' ', 'T') + 'Z',  // 尝试转换为空格分隔的ISO格式
          date.replace(/\//g, '-')       // 尝试将/替换为-的格式
        ];

        for (const format of formats) {
          d = new Date(format);
          if (!isNaN(d.getTime())) {
            return d;
          }
        }
      }

      return null;
    };

    // 格式化日期
    const formatDate = (date) => {
      // 检查日期是否有效
      if (!date) return '-';

      // 使用parseDate函数处理日期
      const parsedDate = parseDate(date);
      if (!parsedDate) return '-';

      return parsedDate.toLocaleString();
    };

    // 移除文件名后缀
    const removeFileExtension = (filename) => {
      if (!filename) return '';
      const lastDotIndex = filename.lastIndexOf('.');
      return lastDotIndex > 0 ? filename.substring(0, lastDotIndex) : filename;
    };

    // 显示添加修文对话框
    const showAddFileDialog = () => {
      newFileName.value = '';
      addFileDialogVisible.value = true;
    };

    // 添加新修文
    const addNewFile = async () => {
      try {
        loading.value = true;
        // 如果输入了文件名，则创建新文件
        if (newFileName.value.trim()) {
          // 使用新添加的后台API接口创建新修文
          const newFileData = {
            filename: removeFileExtension(newFileName.value),
            status: 'ready',
            modified: new Date(),
            type: 'ods'
          };

          // 调用ApiServiceManager组件的新功能创建新修文
          const response = await apiServiceManager.value.newFile(newFileData);
          console.log('[FileManager] 创建新修文响应:', response);
          if (response.errorCode !== 1000 || response.errorMessage !== "Success") {
            console.error('创建新修文失败:', response);
            ElMessage.error('创建新修文失败: ' + (response.errorMessage || '未知错误'));
            return;
          }

          // 显示修改成功后的信息
          ElMessage.success(`修文 "${newFileName.value}" 添加成功`);
        }
        else {
          // 调用ApiServiceManager组件的新功能创建新修文
          const response = await apiServiceManager.value.newFile();
          console.log('[FileManager] 创建新修文响应:', response);
          if (response.errorCode !== 1000 || response.errorMessage !== "Success") {
            console.error('创建新修文失败:', response);
            ElMessage.error('创建新修文失败: ' + (response.errorMessage || '未知错误'));
            return;
          }

          // 显示修改成功后的信息
          ElMessage.success(`修文添加成功`);
        }

        addFileDialogVisible.value = false;
        newFileName.value = '';

        // 无论是否输入文件名，都重新加载修文列表
        // 使用ApiServiceManager组件的新功能获取修文列表
        const response = await apiServiceManager.value.getFileList();
        console.log('获取修文列表:', response);
        if (response.errorCode === 1000 && response.errorMessage === "Success") {
          fileList.value = Array.isArray(response.files) ? response.files.map((item, index) => ({
            id: index,
            name: (item.filename ? removeFileExtension(item.filename) : `修文${index + 1}`),
            status: item.filestatus || '未知',
            modified: parseDate(item.lastModified) || new Date(),
            type: 'ods',
            size: item.size || 0
          })) : [];
        }
        else {
          ElMessage.error('获取修文列表失败: ' + (response.errorMessage || '未知错误'));
        }
      } catch (error) {
        console.error('操作失败:', error);
        ElMessage.error('操作失败: ' + (error.message || '未知错误'));
      } finally {
        loading.value = false;
      }
    };

    // 加载修文列表
    const loadFileList = async () => {
      try {
        loading.value = true;
        // 使用ApiServiceManager组件的新功能获取修文列表
        const response = await apiServiceManager.value.getFileList();
        console.log('获取修文列表:', response);
        if (response.errorCode !== 1000 || response.errorMessage !== "Success") {
          console.error('加载修文列表失败:', response);
          ElMessage.error('加载修文列表失败: ' + (response.errorMessage || '未知错误'));
          return;
        }
        fileList.value = Array.isArray(response.files) ? response.files.map((item, index) => ({
          id: index,
          name: (item.filename ? removeFileExtension(item.filename) : `修文${index + 1}`),
          status: item.status || '未知',
          modified: parseDate(item.lastModified) || new Date(),
          type: 'ods'
        })) : [];
      } catch (error) {
        console.error('加载修文列表失败:', error);
        fileList.value = [];
        ElMessage.error('加载修文列表失败: ' + (error.message || '未知错误'));
      } finally {
        loading.value = false;
      }
    };

    // 处理修文选择
    const handleFileSelect = (row) => {
      selectedFile.value = row;
    };

    // 删除修文
    const deleteFile = async (file) => {
      try {
        await ElMessageBox.confirm(
          `确认删除修文 "${file.name}" 吗？此操作不可恢复。`,
          '确认删除',
          {
            confirmButtonText: '确认',
            cancelButtonText: '取消',
            type: 'warning'
          }
        );

        // 调用ApiServiceManager组件的删除文件功能
        const response = await apiServiceManager.value.deleteFile({ filename: file.name });
        if (response.errorCode !== 1000 || response.errorMessage !== "Success") {
          console.error('[ApiServiceManager] 删除文件失败:', response);
          ElMessage.error('删除文件失败: ' + (response.errorMessage || '未知错误'));
          return;
        }

        // 重新加载文件列表
        await loadFileList();

        ElMessage.success('修文删除成功');
      } catch (error) {
        if (error !== 'cancel') {
          console.error('删除修文失败:', error);
          ElMessage.error('删除修文失败: ' + (error.message || '未知错误'));
        }
      }
    };

    // 加载修文内容
    const loadFileContent = async (file) => {
      try {
        loading.value = true;
        selectedFile.value = file;
        // 调用ApiServiceManager组件获取指定修文的内容
        const response = await apiServiceManager.value.getFileList();
        if (response.errorCode !== 1000 || response.errorMessage !== "Success") {
          console.error('加载修文内容失败:', response);
          ElMessage.error('加载修文内容失败: ' + (response.errorMessage || '未知错误'));
          fileContent.value = null;
          return;
        }
        if (!Array.isArray(response.files) || !response.files.find((f) => f.filename === file.name)) {
          console.error('加载修文内容失败: 修文不存在');
          ElMessage.error('加载修文内容失败: 修文不存在');
          fileContent.value = null;
          return;
        }
        selectedFile.value = response.files.find((f) => f.filename === file.name);
        fileContent.value = response;
        ElMessage.success(`修文 "${file.name}" 加载成功`);
      } catch (error) {
        console.error('加载修文内容失败:', error);
        fileContent.value = null;
        ElMessage.error('加载修文内容失败: ' + (error.message || '未知错误'));
      } finally {
        loading.value = false;
      }
    };

    // 在页面管理器中使用
    const useInPageManager = async (file) => {
        if (!file || !file.name) {
            console.warn('[FileManager] 文件名为空，无法跳转');
            ElMessage.warning('文件名为空，无法跳转');
            return;
        }

        try {
            // 使用 Vue Router 跳转到 PageManager 页面
            await router.push({
                path: '/page', // 假设 PageManager 的路径为 /page
                query: { fileName: file.name } // 通过查询参数传递文件名
            });

            ElMessage.success(`已跳转到页面管理器，编辑文件: ${file.name}`);
        } catch (error) {
            console.error('[FileManager] 路由跳转失败:', error);
            ElMessage.error('跳转到页面管理器失败: ' + (error.message || '未知错误'));
        }
    };

    // 动态加载 PageManager 组件
    const showPageManager = (fileName) => {
      // 检查 DOM 容器是否存在
      const container = document.getElementById('page-manager-container');
      if (!container) {
          console.error('[FileManager] 无法找到挂载容器 #page-manager-container');
          ElMessage.error('页面初始化失败，请刷新后重试');
          return;
      }

      // 清空容器内容，避免重复挂载
      container.innerHTML = '';

      // 动态加载 PageManager 组件
      import('@/views/PageManager.vue')
          .then((module) => {
              const PageManager = module.default;

              // 创建 PageManager 实例并传递参数
              const pageManagerInstance = createApp(PageManager, {
                  fileName // 传递文件名作为属性
              });

              // 挂载到 DOM
              pageManagerInstance.mount('#page-manager-container');
          })
          .catch((error) => {
              console.error('[FileManager] 加载 PageManager 组件失败:', error);
              ElMessage.error('加载页面管理器失败，请稍后重试');
          });
    };

    // 清除修文内容
    const clearFileContent = () => {
      selectedFile.value = null;
      fileContent.value = null;
    };

    // 组件挂载时加载修文列表
    onMounted(() => {
      loadFileList();
    });

    return {
      fileList,
      loading,
      selectedFile,
      fileContent,
      formatFileSize,
      formatDate,
      loadFileList,
      handleFileSelect,
      deleteFile,
      loadFileContent,
      useInPageManager,
      clearFileContent,
      // 添加修文相关
      addFileDialogVisible,
      newFileName,
      showAddFileDialog,
      addNewFile,
      // API服务管理
      apiServiceManager
    };
  }
};
</script>

<style scoped>
.file-manager {
  padding: 20px;
  max-width: 1200px;
  margin: 0 auto;
}

.navigation {
  margin-bottom: 20px;
}

.home-link {
  display: inline-block;
  padding: 8px 16px;
  background-color: #409eff;
  color: white;
  text-decoration: none;
  border-radius: 4px;
  transition: background-color 0.3s;
}

.home-link:hover {
  background-color: #66b1ff;
}

.file-list-container,
.file-content-container {
  margin-bottom: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.file-content {
  min-height: 200px;
  max-height: 500px;
  overflow-y: auto;
  background-color: #f5f5f5;
  padding: 10px;
  border-radius: 4px;
}

.file-content pre {
  margin: 0;
  white-space: pre-wrap;
  word-wrap: break-word;
}

.no-content {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 200px;
  color: #909399;
}

.dialog-footer {
  text-align: right;
}
</style>