// jest.setup.js
import '@testing-library/jest-dom';

// 添加全局样式
import './tests/unit/global.css';

// 添加全局变量
global.ResizeObserver = require('resize-observer-polyfill');

// 添加全局错误处理
console.error = jest.fn();