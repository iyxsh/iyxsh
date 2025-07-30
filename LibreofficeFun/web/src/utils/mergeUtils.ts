import type { CardGroup, CardStyle } from '@/types/cardTypes';

export interface DeepMergeConfig {
  inheritanceValidation?: (style: CardStyle) => void;
  cacheKeyStrategy?: (group: CardGroup) => string;
  stylePriority?: ('parent' | 'global' | 'local')[];
}

export function deepMerge<T extends Record<string, any>>(
  target: T,
  ...sources: Partial<T>[]
): T {
  const merged = { ...target };
  
  sources.forEach(source => {
    if (!source || typeof source !== 'object') return;
    
    Object.keys(source).forEach(key => {
      const targetValue = merged[key];
      const sourceValue = source[key];

      if (Array.isArray(sourceValue)) {
        merged[key] = [...(Array.isArray(targetValue) ? targetValue : []), ...sourceValue];
      } else if (typeof sourceValue === 'object' && sourceValue !== null) {
        merged[key] = deepMerge(targetValue || {}, sourceValue);
      } else {
        merged[key] = sourceValue;
      }
    });
  });

  return merged;
}

export function createStyleCache() {
  const cache = new Map<string, CardStyle>();
  let version = 0;

  return {
    get(key: string) {
      return cache.get(key);
    },
    set(key: string, value: CardStyle) {
      cache.set(key, value);
    },
    clear() {
      version++;
      cache.clear();
    },
    getVersion() {
      return version;
    }
  };
}