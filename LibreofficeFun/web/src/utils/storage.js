// utils/storage.js

class StorageUtil {
  static getItem(key) {
    if (!key) throw new Error('Key cannot be empty');
    return localStorage.getItem(key);
  }

  static setItem(key, value) {
    if (!key) throw new Error('Key cannot be empty');
    if (value === undefined || value === null) throw new Error('Value cannot be empty');
    localStorage.setItem(key, value);
  }

  static removeItem(key) {
    if (!key) throw new Error('Key cannot be empty');
    localStorage.removeItem(key);
  }

  static clear() {
    localStorage.clear();
  }
}

export default StorageUtil;