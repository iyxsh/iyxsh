module.exports = {
  testEnvironment: 'jsdom',
  transform: {
    '^.+\.vue$': 'vue3-jest',
    '^.+\.js$': 'babel-jest'
  },
  moduleFileExtensions: [
    'js',
    'vue',
    'json'
  ],
  moduleNameMapper: {
    '^@/(.*)$': '<rootDir>/src/$1'
  },
  setupFilesAfterEnv: [
    '@testing-library/jest-dom/extend-expect',
    './jest.setup.js'
  ],
  testMatch: [
    '**/tests/unit/**/*.spec.js'
  ],
  collectCoverage: true,
  collectCoverageFrom: [
    'src/**/*.{js,vue}',
    '!src/main.js',
    '!src/router/index.js'
  ],
  coverageDirectory: 'coverage',
  coverageReporters: [
    'json',
    'text',
    'html'
  ]
};