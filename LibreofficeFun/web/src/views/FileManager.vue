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
                <el-button 
                  type="primary" 
                  icon="Edit" 
                  size="small" 
                  circle 
                  @click.stop="useInPageManager(scope.row)"
                />
              </el-tooltip>
              <el-tooltip content="加载" placement="top">
                <el-button 
                  type="success" 
                  icon="Download" 
                  size="small" 
                  circle 
                  @click.stop="loadFileContent(scope.row)"
                />
              </el-tooltip>
              <el-tooltip content="删除" placement="top">
                <el-button 
                  type="danger" 
                  icon="Delete" 
                  size="small" 
                  circle 
                  @click.stop="deleteFile(scope.row)"
                />
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
  </div>
</template>

<script>
import { ref, onMounted } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage, ElMessageBox } from 'element-plus';
import ApiServiceManager from '@/components/ApiServiceManager.vue';

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
          await apiServiceManager.value.newFile(newFileData);

          // 显示修改成功后的信息
          ElMessage.success(`修文 "${newFileName.value}" 添加成功`);
        }
        else {
          // 调用ApiServiceManager组件的新功能创建新修文
          await apiServiceManager.value.newFile();

          // 显示修改成功后的信息
          ElMessage.success(`修文添加成功`);
        }

        addFileDialogVisible.value = false;
        newFileName.value = '';

        // 无论是否输入文件名，都重新加载修文列表
        // 使用ApiServiceManager组件的新功能获取修文列表
        const response = await apiServiceManager.value.getFileList();
        fileList.value = Array.isArray(response) ? response.map((item, index) => ({
          id: index,
          name: (item.filename ? removeFileExtension(item.filename) : `修文${index + 1}`),
          status: item.status || '未知',
          modified: parseDate(item.lastModified) || new Date(),
          type: 'ods'
        })) : [];
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
        fileList.value = Array.isArray(response) ? response.map((item, index) => ({
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
        await apiServiceManager.value.deleteFile(file.name);
        
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
    const useInPageManager = (file) => {
      // 如果传入了文件参数，直接使用该文件
      if (file) {
        router.push({
          name: 'page',
          query: {
            fileName: file.name
          }
        });
      } 
      // 否则使用已加载的文件内容
      else if (fileContent.value) {
        // 将修文内容传递给PageManager组件
        router.push({
          name: 'page',
          query: {
            fileData: JSON.stringify(fileContent.value),
            fileName: selectedFile.value.name
          }
        });
      }
      // 如果没有文件内容但有选中的文件，只传递文件名
      else if (selectedFile.value) {
        router.push({
          name: 'page',
          query: {
            fileName: selectedFile.value.name
          }
        });
      }
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